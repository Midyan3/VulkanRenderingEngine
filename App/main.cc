// Initializes all core systems and renders a single "spinning" *Vulkan* triangle.

#include "../Core/Window/Window.h"
#include "../Core/Window/OS-Windows/Win32/Win32Window.h"
#include "../Core/Window/OS-Windows/Win32/WindowManager/WindowManager.h"
#include "../Core/Renderer/VulkanInstance/VulkanInstance.h"
#include "../Core/Renderer/VulkanDevice/VulkanDevice.h"
#include "../Core/Renderer/VulkanSurface/VulkanSurface.h"
#include "../Core/Renderer/VulkanSwapchain/VulkanSwapchain.h"
#include "../Core/Renderer/VulkanRenderPass/VulkanRenderPass.h"
#include "../Core/Renderer/VulkanFrameBuffer/VulkanFrameBuffer.h"
#include "../Core/Renderer/VulkanGraphicsPipeline/VulkanGraphicsPipeline.h"
#include "../Core/Renderer/VulkanCommandBuffer/VulkanCommandBuffer.h"
#include "../Core/Renderer/VulkanMemoryAllocator/VulkanMemoryAllocator.h"
#include "../Core/Renderer/VulkanSynchronization/VulkanSynchronization.h"
#include "../Core/Application/Application.h"
#include "../Core/Application/WindowSpec/WindowSpec.h"
#include "../Core/Renderer//VertexTypes/Vertex.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <chrono>

#ifdef _WIN32
#include <windows.h>
#include <vulkan/vulkan_win32.h>
#endif

#include <iostream>

class test : public Application
{
public:
    test()
    {
        m_instance = std::make_shared<VulkanInstance>();
        m_instance->Initialize("TestApp");

        windowSpec::WindowOptions options(60, 2560, 1440, "Test Window");
        auto windowUnique = Window::Create(options);
        m_window = std::move(windowUnique);
        m_window->Show();

        Win32Window* win32Window = dynamic_cast<Win32Window*>(m_window.get());
        VkWin32SurfaceCreateInfoKHR surfaceInfo = {};
        surfaceInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        surfaceInfo.hwnd = win32Window->GetHWND();
        surfaceInfo.hinstance = win32Window->GetHINSTANCE();
        VkSurfaceKHR tempSurface;
        vkCreateWin32SurfaceKHR(m_instance->GetInstance(), &surfaceInfo, nullptr, &tempSurface);

        m_device = std::make_shared<VulkanDevice>();
        m_device->Initialize(m_instance, tempSurface);
        vkDestroySurfaceKHR(m_instance->GetInstance(), tempSurface, nullptr);

        m_surface = std::make_shared<VulkanSurface>();
        m_surface->Initialize(m_instance, m_device, m_window);

        m_swapchain = std::make_shared<VulkanSwapchain>();
        m_swapchain->Initialize(m_instance, m_device, m_surface);

        m_renderPass = std::make_shared<VulkanRenderPass>();
        RenderPassConfig config = RenderPassConfig::SingleColorAttachment(m_swapchain->GetFormat());
        m_renderPass->Initialize(m_instance, m_device, config);

        uint32_t imageCount = m_swapchain->GetImageCount();
        m_framebuffers.resize(imageCount);
        for (uint32_t i = 0; i < imageCount; i++)
        {
            m_framebuffers[i] = std::make_shared<VulkanFrameBuffer>();
            m_framebuffers[i]->Initialize(
                m_instance,
                m_device,
                m_renderPass,
                { m_swapchain->GetImageView(i) },
                m_swapchain->GetExtent().width,
                m_swapchain->GetExtent().height
            );
        }

        // Adding descriptions for vertex buffer 2.
        auto bindingDescription = Vertex::GetBindingDescription();
        auto attributeDescriptions = Vertex::GetAttributeDescriptions();
        // End of 2.
        
        m_pipeline = std::make_shared<VulkanGraphicsPipeline>();
        GraphicsPipelineConfig pipelineConfig = GraphicsPipelineConfig::SimpleTriangle(
            "Shaders/triangle.vert.spv",
            "Shaders/triangle.frag.spv"
        );

        pipelineConfig.vertexInput.bindings = {bindingDescription};
        pipelineConfig.vertexInput.attributes = { attributeDescriptions.begin(), attributeDescriptions.end()};

        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(glm::mat4);
        pipelineConfig.pushConstantRanges = { pushConstantRange };

        pipelineConfig.viewport = m_swapchain->GetExtent();
        m_pipeline->Initialize(m_instance, m_device, m_renderPass, pipelineConfig);

        m_commandBuffer = std::make_shared<VulkanCommandBuffer>();
        m_commandBuffer->Initialize(m_instance, m_device, m_device->GetGraphicsQueueFamily());

        m_sync = std::make_shared<VulkanSynchronization>();
        m_sync->Initialize(m_instance, m_device, 3, imageCount);

        m_commandBuffers = m_commandBuffer->AllocateCommandBuffers(imageCount);
        m_currentFrame = 0;

        std::vector<Vertex> vertices = {
       {{0.0f, -0.7f, 0.0f}, {1.0f, 0.0f, 0.0f}},
       {{0.5f, 0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}},   
       {{-0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}}   
        };

        m_allocator = std::make_shared<VulkanMemoryAllocator>(); 

        m_allocator->Initialize(m_instance, m_device);

        m_allocator->CreateVertexBuffer(
            m_commandBuffer.get(),
            vertices.data(),
            sizeof(Vertex) * vertices.size(),
            m_vertexBuffer
        );

        m_window->OnResized([&](int width, int height)
            {
                m_swapchain->Recreate();
            }
        ); 

        std::cout << "Initialization complete\n";
        std::cout << m_surface->GetSurfaceInfo() << "\n";
        std::cout << m_swapchain->GetSwapchainInfo() << "\n";
        std::cout << m_renderPass->GetRenderPassInfo() << "\n";
        std::cout << m_pipeline->GetPipelineInfo() << "\n";
    }

    void Update(float deltaTime) override 
    {
        m_rotation += deltaTime * 45.0f;  
        if (m_rotation > 360.0f)
            m_rotation -= 360.0f;
    }

    void Render() override
    {
        WindowManager::PollAllWindowEvents();
        m_sync->WaitForFence(m_currentFrame);

        uint32_t imageIndex;
        if (!m_swapchain->AcquireNextImage(imageIndex,
            m_sync->GetImageSync(m_currentFrame).imageAvailableSemaphore,
            VK_NULL_HANDLE))
            return;

        m_sync->ResetFence(m_currentFrame);

        VkCommandBuffer cmd = m_commandBuffers[m_currentFrame];
        vkResetCommandBuffer(cmd, 0);
        m_commandBuffer->BeginRecording(cmd);

        auto clearValues = m_renderPass->GetDefaultClearValues();
        m_renderPass->Begin(cmd, m_framebuffers[imageIndex]->GetFramebuffer(),
            m_swapchain->GetExtent(), clearValues);

        m_pipeline->Bind(cmd);

        glm::mat4 transform = glm::rotate(glm::mat4(1.0f),
            glm::radians(m_rotation),
            glm::vec3(0.0f, 0.0f, 1.0f));

        vkCmdPushConstants(cmd,
            m_pipeline->GetLayout(),
            VK_SHADER_STAGE_VERTEX_BIT,
            0,
            sizeof(glm::mat4),
            &transform);
        
        VkBuffer vertexBuffers[] = { m_vertexBuffer.buffer };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);

        vkCmdDraw(cmd, 3, 1, 0, 0);

        m_renderPass->End(cmd);
          
        m_commandBuffer->EndRecording(cmd);

        // Changed to use class functions instead of redoing work. 1.
        m_commandBuffer->Submit(
            cmd,
            m_device->GetGraphicsQueue(),
            { m_sync->GetImageSync(m_currentFrame).imageAvailableSemaphore },
            { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT },
            { m_sync->GetImageSync(m_currentFrame).renderFinishedSemaphore },
            m_sync->GetFrameSync(m_currentFrame).inFlightFence
        );

        m_swapchain->PresentImage(imageIndex,
            { m_sync->GetImageSync(imageIndex).renderFinishedSemaphore });

        // End of 1.

        m_currentFrame = (m_currentFrame + 1) % m_sync->GetMaxFramesInFlight();
    }

private:
    std::shared_ptr<VulkanInstance> m_instance;
    std::shared_ptr<VulkanDevice> m_device;
    std::shared_ptr<VulkanSurface> m_surface;
    std::shared_ptr<VulkanSwapchain> m_swapchain;
    std::shared_ptr<VulkanRenderPass> m_renderPass;
    std::vector<std::shared_ptr<VulkanFrameBuffer>> m_framebuffers;
    std::shared_ptr<VulkanGraphicsPipeline> m_pipeline;
    std::shared_ptr<VulkanCommandBuffer> m_commandBuffer;
    std::shared_ptr<VulkanSynchronization> m_sync;
    std::shared_ptr<VulkanMemoryAllocator> m_allocator;  
    std::shared_ptr<Window> m_window;
    std::vector<VkCommandBuffer> m_commandBuffers;
    uint32_t m_currentFrame;
    AllocatedBuffer m_vertexBuffer;  
    float m_rotation = 0.0f;
};

Application* CreateApplication()
{
    return new test();
}

int main()
{
    auto app = CreateApplication();
    app->Run();
    delete app;
    return 0;
}
