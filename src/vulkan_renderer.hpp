#ifndef VULKAN_RENDERER_HPP
#define VULKAN_RENDERER_HPP

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>

class VulkanRenderer {
public:
    VulkanRenderer();
    ~VulkanRenderer();
    VulkanRenderer(const VulkanRenderer&) = delete;
    VulkanRenderer& operator=(const VulkanRenderer&) = delete;

    bool init(uint32_t width, uint32_t height, const char* title);
    void run();
    void shutdown();

    GLFWwindow* window() const { return m_window; }
    uint32_t width() const { return m_width; }
    uint32_t height() const { return m_height; }

    // Vulkan handles — public for ImGui integration
    VkInstance m_instance = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkDevice m_device = VK_NULL_HANDLE;
    VkQueue m_graphicsQueue = VK_NULL_HANDLE;
    VkQueue m_presentQueue = VK_NULL_HANDLE;
    uint32_t m_graphicsQueueFamily = UINT32_MAX;

    VkSurfaceKHR m_surface = VK_NULL_HANDLE;
    VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
    VkFormat m_swapchainFormat;
    VkExtent2D m_swapchainExtent;
    std::vector<VkImage> m_swapchainImages;
    std::vector<VkImageView> m_swapchainImageViews;
    std::vector<VkFramebuffer> m_framebuffers;

    VkRenderPass m_renderPass = VK_NULL_HANDLE;
    VkCommandPool m_commandPool = VK_NULL_HANDLE;
    VkCommandBuffer m_commandBuffer = VK_NULL_HANDLE;

    VkSemaphore m_imageSemaphore = VK_NULL_HANDLE;
    VkSemaphore m_renderSemaphore = VK_NULL_HANDLE;
    VkFence m_fence = VK_NULL_HANDLE;

    VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
    uint32_t m_width = 1280;
    uint32_t m_height = 800;

    void cleanupSwapchain();
    void recreateSwapchain();
    GLFWwindow* m_window = nullptr;

    bool createInstance();
    bool createSurface();
    bool pickPhysicalDevice();
    bool createDevice();
    bool createSwapchain();
    bool createRenderPass();
    bool createCommandPool();
    bool createFramebuffers();
    bool createSyncObjects();
    bool createDescriptorPool();
};

#endif
