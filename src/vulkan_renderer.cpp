#include "vulkan_renderer.hpp"
#include <iostream>
#include <set>
#include <cstring>

// ============================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================
VulkanRenderer::VulkanRenderer() {}
VulkanRenderer::~VulkanRenderer() { shutdown(); }

void VulkanRenderer::shutdown() {
    if (m_device == VK_NULL_HANDLE) return;
    vkDeviceWaitIdle(m_device);

    vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);
    vkDestroySemaphore(m_device, m_renderSemaphore, nullptr);
    vkDestroySemaphore(m_device, m_imageSemaphore, nullptr);
    vkDestroyFence(m_device, m_fence, nullptr);
    vkDestroyCommandPool(m_device, m_commandPool, nullptr);
    cleanupSwapchain();
    vkDestroyRenderPass(m_device, m_renderPass, nullptr);
    vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
    vkDestroyDevice(m_device, nullptr);
    vkDestroyInstance(m_instance, nullptr);
    if (m_window) {
        glfwDestroyWindow(m_window);
        glfwTerminate();
    }
    m_device = VK_NULL_HANDLE;
}

// ============================================================
// INIT
// ============================================================
bool VulkanRenderer::init(uint32_t width, uint32_t height, const char* title) {
    m_width = width;
    m_height = height;

    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    m_window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!m_window) { std::cerr << "GLFW window creation failed\n"; return false; }

    if (!createInstance()) return false;
    if (!createSurface()) return false;
    if (!pickPhysicalDevice()) return false;
    if (!createDevice()) return false;
    if (!createSwapchain()) return false;
    if (!createRenderPass()) return false;
    if (!createCommandPool()) return false;
    if (!createFramebuffers()) return false;
    if (!createSyncObjects()) return false;
    if (!createDescriptorPool()) return false;
    return true;
}

// ============================================================
// MAIN LOOP
// ============================================================
void VulkanRenderer::run() {
    while (!glfwWindowShouldClose(m_window)) {
        glfwPollEvents();
    }
}

// ============================================================
// VULKAN SETUP
// ============================================================
bool VulkanRenderer::createInstance() {
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "VulkanCalc";
    appInfo.applicationVersion = VK_MAKE_VERSION(1,0,0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1,0,0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    uint32_t glfwCount;
    const char** glfwExts = glfwGetRequiredInstanceExtensions(&glfwCount);
    std::vector<const char*> exts(glfwExts, glfwExts + glfwCount);
    exts.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    // Portability enumeration for dzn/MoltenVK
    uint32_t instExtCount;
    vkEnumerateInstanceExtensionProperties(nullptr, &instExtCount, nullptr);
    std::vector<VkExtensionProperties> instExts(instExtCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &instExtCount, instExts.data());
    bool hasPort = false;
    for (auto& e : instExts) {
        if (strcmp(e.extensionName, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME) == 0) {
            hasPort = true;
            break;
        }
    }
    if (hasPort) {
        exts.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    }

    VkInstanceCreateInfo ci{};
    ci.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    ci.pApplicationInfo = &appInfo;
    ci.flags = hasPort ? VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR : 0;
    ci.enabledExtensionCount = (uint32_t)exts.size();
    ci.ppEnabledExtensionNames = exts.data();

    VkDebugUtilsMessengerCreateInfoEXT debug{};
    debug.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debug.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debug.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debug.pfnUserCallback = [](VkDebugUtilsMessageSeverityFlagBitsEXT,
                                VkDebugUtilsMessageTypeFlagsEXT,
                                const VkDebugUtilsMessengerCallbackDataEXT* data,
                                void*) -> VkBool32 {
        if (data && data->pMessage) std::cerr << "[VK] " << data->pMessage << "\n";
        return VK_FALSE;
    };
    ci.pNext = &debug;

    return vkCreateInstance(&ci, nullptr, &m_instance) == VK_SUCCESS;
}

bool VulkanRenderer::createSurface() {
    return glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface) == VK_SUCCESS;
}

bool VulkanRenderer::pickPhysicalDevice() {
    uint32_t count;
    vkEnumeratePhysicalDevices(m_instance, &count, nullptr);
    if (count == 0) return false;
    std::vector<VkPhysicalDevice> devices(count);
    vkEnumeratePhysicalDevices(m_instance, &count, devices.data());

    for (auto& dev : devices) {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(dev, &props);
        if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            m_physicalDevice = dev;
            return true;
        }
    }
    m_physicalDevice = devices[0];
    return true;
}

bool VulkanRenderer::createDevice() {
    uint32_t count;
    vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &count, nullptr);
    std::vector<VkQueueFamilyProperties> families(count);
    vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &count, families.data());

    int gfxIdx = -1, presentIdx = -1;
    for (uint32_t i = 0; i < count; i++) {
        if (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) gfxIdx = i;
        VkBool32 support;
        vkGetPhysicalDeviceSurfaceSupportKHR(m_physicalDevice, i, m_surface, &support);
        if (support) presentIdx = i;
    }
    if (gfxIdx < 0 || presentIdx < 0) return false;

    float priority = 1.0f;
    std::set<uint32_t> unique = {(uint32_t)gfxIdx, (uint32_t)presentIdx};
    std::vector<VkDeviceQueueCreateInfo> qInfos;
    for (auto idx : unique) {
        VkDeviceQueueCreateInfo q{};
        q.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        q.queueFamilyIndex = idx;
        q.queueCount = 1;
        q.pQueuePriorities = &priority;
        qInfos.push_back(q);
    }

    // Check for portability subset
    uint32_t deCount;
    vkEnumerateDeviceExtensionProperties(m_physicalDevice, nullptr, &deCount, nullptr);
    std::vector<VkExtensionProperties> devExts(deCount);
    vkEnumerateDeviceExtensionProperties(m_physicalDevice, nullptr, &deCount, devExts.data());
    std::vector<const char*> enabledExts = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    for (auto& e : devExts) {
        if (strcmp(e.extensionName, "VK_KHR_portability_subset") == 0) {
            enabledExts.push_back("VK_KHR_portability_subset");
            break;
        }
    }

    VkDeviceCreateInfo dci{};
    dci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    dci.queueCreateInfoCount = (uint32_t)qInfos.size();
    dci.pQueueCreateInfos = qInfos.data();
    dci.enabledExtensionCount = (uint32_t)enabledExts.size();
    dci.ppEnabledExtensionNames = enabledExts.data();

    if (vkCreateDevice(m_physicalDevice, &dci, nullptr, &m_device) != VK_SUCCESS)
        return false;

    m_graphicsQueueFamily = gfxIdx;
    vkGetDeviceQueue(m_device, gfxIdx, 0, &m_graphicsQueue);
    vkGetDeviceQueue(m_device, presentIdx, 0, &m_presentQueue);
    return true;
}

bool VulkanRenderer::createSwapchain() {
    VkSurfaceCapabilitiesKHR caps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physicalDevice, m_surface, &caps);

    uint32_t fmtCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface, &fmtCount, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(fmtCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface, &fmtCount, formats.data());

    VkSurfaceFormatKHR fmt = formats[0];
    for (auto& f : formats) {
        if (f.format == VK_FORMAT_B8G8R8A8_SRGB && f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            fmt = f; break;
        }
    }

    uint32_t pmCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, m_surface, &pmCount, nullptr);
    std::vector<VkPresentModeKHR> modes(pmCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, m_surface, &pmCount, modes.data());

    VkPresentModeKHR mode = VK_PRESENT_MODE_FIFO_KHR;
    for (auto m : modes) {
        if (m == VK_PRESENT_MODE_MAILBOX_KHR) { mode = m; break; }
        if (m == VK_PRESENT_MODE_IMMEDIATE_KHR) mode = m;
    }

    m_swapchainExtent = caps.currentExtent;
    if (m_swapchainExtent.width == UINT32_MAX) {
        m_swapchainExtent = {m_width, m_height};
    }

    VkSwapchainCreateInfoKHR sci{};
    sci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    sci.surface = m_surface;
    sci.minImageCount = std::min(caps.minImageCount + 1, 3u);
    sci.imageFormat = fmt.format;
    sci.imageColorSpace = fmt.colorSpace;
    sci.imageExtent = m_swapchainExtent;
    sci.imageArrayLayers = 1;
    sci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    sci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    sci.preTransform = caps.currentTransform;
    sci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    sci.presentMode = mode;
    sci.clipped = VK_TRUE;

    if (vkCreateSwapchainKHR(m_device, &sci, nullptr, &m_swapchain) != VK_SUCCESS)
        return false;

    m_swapchainFormat = fmt.format;
    uint32_t imgCount;
    vkGetSwapchainImagesKHR(m_device, m_swapchain, &imgCount, nullptr);
    m_swapchainImages.resize(imgCount);
    vkGetSwapchainImagesKHR(m_device, m_swapchain, &imgCount, m_swapchainImages.data());

    m_swapchainImageViews.resize(imgCount);
    for (size_t i = 0; i < imgCount; i++) {
        VkImageViewCreateInfo vi{};
        vi.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        vi.image = m_swapchainImages[i];
        vi.viewType = VK_IMAGE_VIEW_TYPE_2D;
        vi.format = m_swapchainFormat;
        vi.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        vi.subresourceRange.levelCount = 1;
        vi.subresourceRange.layerCount = 1;
        vkCreateImageView(m_device, &vi, nullptr, &m_swapchainImageViews[i]);
    }
    return true;
}

bool VulkanRenderer::createRenderPass() {
    VkAttachmentDescription ca{};
    ca.format = m_swapchainFormat;
    ca.samples = VK_SAMPLE_COUNT_1_BIT;
    ca.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    ca.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    ca.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    ca.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference cr{};
    cr.attachment = 0;
    cr.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription sp{};
    sp.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    sp.colorAttachmentCount = 1;
    sp.pColorAttachments = &cr;

    VkSubpassDependency dep{};
    dep.srcSubpass = VK_SUBPASS_EXTERNAL;
    dep.dstSubpass = 0;
    dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo rp{};
    rp.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rp.attachmentCount = 1;
    rp.pAttachments = &ca;
    rp.subpassCount = 1;
    rp.pSubpasses = &sp;
    rp.dependencyCount = 1;
    rp.pDependencies = &dep;

    return vkCreateRenderPass(m_device, &rp, nullptr, &m_renderPass) == VK_SUCCESS;
}

bool VulkanRenderer::createCommandPool() {
    VkCommandPoolCreateInfo ci{};
    ci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    ci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    ci.queueFamilyIndex = m_graphicsQueueFamily;
    vkCreateCommandPool(m_device, &ci, nullptr, &m_commandPool);

    VkCommandBufferAllocateInfo ai{};
    ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    ai.commandPool = m_commandPool;
    ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    ai.commandBufferCount = 1;
    vkAllocateCommandBuffers(m_device, &ai, &m_commandBuffer);
    return true;
}

bool VulkanRenderer::createFramebuffers() {
    m_framebuffers.resize(m_swapchainImageViews.size());
    for (size_t i = 0; i < m_swapchainImageViews.size(); i++) {
        VkFramebufferCreateInfo fci{};
        fci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fci.renderPass = m_renderPass;
        fci.attachmentCount = 1;
        fci.pAttachments = &m_swapchainImageViews[i];
        fci.width = m_swapchainExtent.width;
        fci.height = m_swapchainExtent.height;
        fci.layers = 1;
        vkCreateFramebuffer(m_device, &fci, nullptr, &m_framebuffers[i]);
    }
    return true;
}

bool VulkanRenderer::createSyncObjects() {
    VkSemaphoreCreateInfo si{};
    si.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VkFenceCreateInfo fi{};
    fi.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fi.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    return vkCreateSemaphore(m_device, &si, nullptr, &m_imageSemaphore) == VK_SUCCESS
        && vkCreateSemaphore(m_device, &si, nullptr, &m_renderSemaphore) == VK_SUCCESS
        && vkCreateFence(m_device, &fi, nullptr, &m_fence) == VK_SUCCESS;
}

bool VulkanRenderer::createDescriptorPool() {
    VkDescriptorPoolSize ps[] = {
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10},
    };
    VkDescriptorPoolCreateInfo ci{};
    ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    ci.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    ci.maxSets = 32;
    ci.poolSizeCount = 3;
    ci.pPoolSizes = ps;
    return vkCreateDescriptorPool(m_device, &ci, nullptr, &m_descriptorPool) == VK_SUCCESS;
}

void VulkanRenderer::cleanupSwapchain() {
    for (auto& fb : m_framebuffers) vkDestroyFramebuffer(m_device, fb, nullptr);
    m_framebuffers.clear();
    for (auto& iv : m_swapchainImageViews) vkDestroyImageView(m_device, iv, nullptr);
    m_swapchainImageViews.clear();
    m_swapchainImages.clear();
    if (m_swapchain) vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
    m_swapchain = VK_NULL_HANDLE;
}

void VulkanRenderer::recreateSwapchain() {
    vkDeviceWaitIdle(m_device);
    cleanupSwapchain();
    createSwapchain();
    createFramebuffers();
}
