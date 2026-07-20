#include "vulkan_renderer.hpp"
#include "math_engine.hpp"
#include "state.hpp"
#include "locale.hpp"
#include "calc_logic.hpp"
#include "ui.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include <iostream>
#include <cstring>

// Global state instance
CalcState g_state;

// Vulkan debug callback
static void check_vk_result(VkResult err) {
    if (err != VK_SUCCESS) {
        std::cerr << "VkResult " << err << "\n";
        if (err < 0) abort();
    }
}

int main() {
    // --- Detect language ---
    g_state.lang = detectLang();

    // --- Init Vulkan ---
    VulkanRenderer renderer;
    if (!renderer.init(480, 700, "VulkanCalc")) {
        std::cerr << "Failed to init Vulkan renderer\n";
        return 1;
    }

    // Make window resizable with minimum size
    glfwSetWindowAttrib(renderer.window(), GLFW_RESIZABLE, GLFW_TRUE);
    glfwSetWindowSizeLimits(renderer.window(), 400, 600, GLFW_DONT_CARE, GLFW_DONT_CARE);

    GLFWwindow* w = renderer.window();

    // --- Init ImGui ---
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = "imgui.ini";

    // --- Font setup ---
    // Font sizes for LCD display
    ImFontConfig cfg;

    // Font 0: Default (10px)
    ImFont* f0 = io.Fonts->AddFontDefault();

    // Font 1: Large bold for result (28px)
    ImFont* f1 = nullptr;
    // Font 2: Medium for expression (18px)
    ImFont* f2 = nullptr;
    // Font 3: Small for history/status (13px)
    ImFont* f3 = nullptr;

    // Try CJK font first
    const char* cjkPath = "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc";
    FILE* testFile = fopen(cjkPath, "rb");
    if (testFile) {
        fclose(testFile);
        // Load CJK with different sizes
        f1 = io.Fonts->AddFontFromFileTTF(cjkPath, 28.0f, nullptr,
            io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
        f2 = io.Fonts->AddFontFromFileTTF(cjkPath, 16.0f, nullptr,
            io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
        f3 = io.Fonts->AddFontFromFileTTF(cjkPath, 12.0f, nullptr,
            io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
    }

    // Fallback to default
    if (!f1) f1 = io.Fonts->AddFontDefault(); // will be scaled via PushFont
    if (!f2) f2 = io.Fonts->AddFontDefault();
    if (!f3) f3 = io.Fonts->AddFontDefault();

    // Set default to the 16px CJK or default
    io.FontDefault = f2;

    // --- Style ---
    ImGui::StyleColorsDark();
    auto& st = ImGui::GetStyle();
    st.WindowRounding = 0;
    st.FrameRounding = 2.0f;
    st.WindowBorderSize = 0;
    st.FrameBorderSize = 0;
    st.ChildBorderSize = 0;
    st.WindowPadding = ImVec2(6, 4);
    st.FramePadding = ImVec2(4, 2);
    st.ItemSpacing = ImVec2(4, 2);

    // Dark theme colors
    auto setCol = [&](ImGuiCol idx, float r, float g, float b, float a = 1) {
        st.Colors[idx] = ImVec4(r, g, b, a);
    };
    setCol(ImGuiCol_WindowBg, 0.06f, 0.06f, 0.10f, 0.95f);
    setCol(ImGuiCol_ChildBg, 0.04f, 0.04f, 0.08f, 0.90f);
    setCol(ImGuiCol_PopupBg, 0.10f, 0.10f, 0.16f, 0.95f);
    setCol(ImGuiCol_Text, 0.90f, 0.90f, 0.95f, 1);
    setCol(ImGuiCol_Border, 0.12f, 0.12f, 0.20f, 0.5f);
    setCol(ImGuiCol_FrameBg, 0.08f, 0.08f, 0.12f, 0.8f);
    setCol(ImGuiCol_FrameBgHovered, 0.12f, 0.12f, 0.18f, 0.85f);
    setCol(ImGuiCol_FrameBgActive, 0.15f, 0.15f, 0.22f, 0.9f);
    setCol(ImGuiCol_Button, 0.12f, 0.12f, 0.18f, 0.9f);
    setCol(ImGuiCol_ButtonHovered, 0.18f, 0.18f, 0.26f, 0.95f);
    setCol(ImGuiCol_ButtonActive, 0.25f, 0.25f, 0.35f, 1);
    setCol(ImGuiCol_Header, 0.15f, 0.15f, 0.22f, 0.8f);
    setCol(ImGuiCol_Separator, 0.12f, 0.12f, 0.18f, 0.5f);
    setCol(ImGuiCol_TitleBg, 0.04f, 0.04f, 0.08f, 0.95f);
    setCol(ImGuiCol_TitleBgActive, 0.06f, 0.06f, 0.12f, 0.95f);

    // --- Init ImGui backends ---
    ImGui_ImplGlfw_InitForVulkan(w, true);
    ImGui_ImplVulkan_InitInfo vi{};
    vi.Instance = renderer.m_instance;
    vi.PhysicalDevice = renderer.m_physicalDevice;
    vi.Device = renderer.m_device;
    vi.QueueFamily = renderer.m_graphicsQueueFamily;
    vi.Queue = renderer.m_graphicsQueue;
    vi.DescriptorPool = renderer.m_descriptorPool;
    vi.RenderPass = renderer.m_renderPass;
    vi.Subpass = 0;
    vi.MinImageCount = 2;
    vi.ImageCount = (uint32_t)renderer.m_swapchainImages.size();
    vi.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    vi.CheckVkResultFn = check_vk_result;
    ImGui_ImplVulkan_Init(&vi);
    ImGui_ImplVulkan_CreateFontsTexture();

    // Keyboard callback
    glfwSetKeyCallback(w, [](GLFWwindow* window, int key, int scancode,
                              int action, int mods) {
        if (action == GLFW_PRESS || action == GLFW_REPEAT) {
            // Ignore modifier-only keys
            if (key == GLFW_KEY_LEFT_SHIFT || key == GLFW_KEY_RIGHT_SHIFT ||
                key == GLFW_KEY_LEFT_CONTROL || key == GLFW_KEY_RIGHT_CONTROL ||
                key == GLFW_KEY_LEFT_ALT || key == GLFW_KEY_RIGHT_ALT ||
                key == GLFW_KEY_CAPS_LOCK) return;
            calcKeyboardInput(key, mods);
        }
    });

    // ============================================================
    // MAIN LOOP
    // ============================================================
    while (!glfwWindowShouldClose(w)) {
        glfwPollEvents();

        int fbw, fbh;
        glfwGetFramebufferSize(w, &fbw, &fbh);
        if (fbw > 0 && fbh > 0 &&
            (fbw != (int)renderer.m_width || fbh != (int)renderer.m_height)) {
            renderer.m_width = fbw;
            renderer.m_height = fbh;
            renderer.recreateSwapchain();
        }

        uint32_t ii;
        auto rv = vkAcquireNextImageKHR(renderer.m_device,
                                         renderer.m_swapchain,
                                         UINT64_MAX,
                                         renderer.m_imageSemaphore,
                                         VK_NULL_HANDLE, &ii);
        if (rv == VK_ERROR_OUT_OF_DATE_KHR) {
            renderer.recreateSwapchain();
            continue;
        }

        vkWaitForFences(renderer.m_device, 1, &renderer.m_fence,
                        VK_TRUE, UINT64_MAX);
        vkResetFences(renderer.m_device, 1, &renderer.m_fence);

        // --- ImGui frame ---
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Render the calculator UI
        renderUI();

        // --- Vulkan render ---
        ImGui::Render();
        VkCommandBuffer cmd = renderer.m_commandBuffer;
        vkResetCommandBuffer(cmd, 0);

        VkCommandBufferBeginInfo bi{};
        bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(cmd, &bi);

        VkClearValue cl{};
        cl.color.float32[0] = 0.04f;
        cl.color.float32[1] = 0.04f;
        cl.color.float32[2] = 0.08f;
        cl.color.float32[3] = 1.0f;

        VkRenderPassBeginInfo rp{};
        rp.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        rp.renderPass = renderer.m_renderPass;
        rp.framebuffer = renderer.m_framebuffers[ii];
        rp.renderArea.offset = {0, 0};
        rp.renderArea.extent = renderer.m_swapchainExtent;
        rp.clearValueCount = 1;
        rp.pClearValues = &cl;
        vkCmdBeginRenderPass(cmd, &rp, VK_SUBPASS_CONTENTS_INLINE);
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
        vkCmdEndRenderPass(cmd);
        vkEndCommandBuffer(cmd);

        VkSubmitInfo si{};
        si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        VkSemaphore ws[] = {renderer.m_imageSemaphore};
        VkPipelineStageFlags wst[] = {
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        si.waitSemaphoreCount = 1;
        si.pWaitSemaphores = ws;
        si.pWaitDstStageMask = wst;
        si.commandBufferCount = 1;
        si.pCommandBuffers = &cmd;
        VkSemaphore ss[] = {renderer.m_renderSemaphore};
        si.signalSemaphoreCount = 1;
        si.pSignalSemaphores = ss;
        vkQueueSubmit(renderer.m_graphicsQueue, 1, &si, renderer.m_fence);

        VkPresentInfoKHR pi{};
        pi.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        pi.waitSemaphoreCount = 1;
        pi.pWaitSemaphores = ss;
        pi.swapchainCount = 1;
        pi.pSwapchains = &renderer.m_swapchain;
        pi.pImageIndices = &ii;
        vkQueuePresentKHR(renderer.m_presentQueue, &pi);
    }

    vkDeviceWaitIdle(renderer.m_device);
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    renderer.shutdown();

    return 0;
}
