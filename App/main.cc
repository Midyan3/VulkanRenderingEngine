#include "main.h"

struct CameraUBO
{
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 projection;
};

class test : public Application
{
public:
    test()
    {
        InitializeCore();
        InitializeWindowAndSurface();
        InitializeSwapchainAndRenderPass();
        InitializeFramebuffers();
        InitializePipelineModel();
        InitializeCommandsAndSync();
        InitializeMemoryAndGeometry();
        InitializeTextureManager(); 
        InitalizeImGui(); 
        InitializeDescriptors();
        HookInput();
        LogInitSummary();
    }

    void Update(float deltaTime) override
    {
        WindowManager::PollAllWindowEvents();
        if(!m_manualOverride)
             CameraMovement(deltaTime);

        m_rotation += deltaTime * 45.0f;
        if (m_rotation > 360.0f) m_rotation -= 360.0f;

        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Debug");
        ImGui::Text("FPS: %.1f",  std::floor(1.0f / deltaTime));
       
        ImGui::SliderInt("FOV", &m_crest, 0, maxFOV);
        ImGui::SliderFloat("R", &r, 0, 255.0f);
        ImGui::SliderFloat("G", &g, 0, 255.0f);
        ImGui::SliderFloat("B", &b, 0, 255.0f);
        ImGui::SliderFloat("Alpha", &alpha, 0, 1.0f); 
        ImGui::Checkbox("Manual", &m_manualOverride); 
        ImGui::End();

        ImGui::Render();
        /*/
        
        {
            std::println("R = {}, G = {}, B = {}", r, g, b);
            std::cout << "Int test = " << m_crest << '\n'; 
        }
         
        
        */

        m_camera->SetFOV(m_crest);
    }

    void Render() override
    {
        m_sync->WaitForFence(m_currentFrame);

        m_renderPass->SetNewClearColor({ r / 255.0f , g / 255.0f, b / 255.0f, alpha}); 

        uint32_t imageIndex;
        if (!m_swapchain->AcquireNextImage(
            imageIndex,
            m_sync->GetImageSync(m_currentFrame).imageAvailableSemaphore,
            VK_NULL_HANDLE))
            return;

        m_sync->ResetFence(m_currentFrame);

        VkCommandBuffer cmd = m_commandBuffers[m_currentFrame];
        vkResetCommandBuffer(cmd, 0);
        m_commandBuffer->BeginRecording(cmd);

        BeginRenderPass(cmd, imageIndex);
        DrawModel(cmd);
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
        EndRenderPass(cmd);

        m_commandBuffer->EndRecording(cmd);

        m_commandBuffer->Submit(
            cmd,
            m_device->GetGraphicsQueue(),
            { m_sync->GetImageSync(m_currentFrame).imageAvailableSemaphore },
            { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT },
            { m_sync->GetImageSync(m_currentFrame).renderFinishedSemaphore },
            m_sync->GetFrameSync(m_currentFrame).inFlightFence
        );

        m_swapchain->PresentImage(
            imageIndex,
            { m_sync->GetImageSync(imageIndex).renderFinishedSemaphore }
        );

        m_currentFrame = (m_currentFrame + 1) % m_sync->GetMaxFramesInFlight();

        if (Input::Get().IsKeyPressed(VK::Escape))
        {
            Quit(); 
        }

        Input::Get().Update();
       
    }
    
private: 
    int m_crest = 120; 
    bool m_manualOverride = false; 
    void InitializeCore()
    {
        m_instance = std::make_shared<VulkanInstance>();
        m_instance->Initialize("TestApp");
    }

    void InitializeWindowAndSurface()
    {
        windowSpec::WindowOptions options(60, 2560, 1440, "Test Window");
        auto windowUnique = Window::Create(options);
        m_window = std::move(windowUnique);
        m_window->Show();

#ifdef _WIN32
        Win32Window* win32Window = dynamic_cast<Win32Window*>(m_window.get());
        VkWin32SurfaceCreateInfoKHR surfaceInfo{};
        surfaceInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        surfaceInfo.hwnd = win32Window->GetHWND();
        surfaceInfo.hinstance = win32Window->GetHINSTANCE();
        VkSurfaceKHR tempSurface;
        vkCreateWin32SurfaceKHR(m_instance->GetInstance(), &surfaceInfo, nullptr, &tempSurface);
#else
        VkSurfaceKHR tempSurface = VK_NULL_HANDLE;
#endif

        m_device = std::make_shared<VulkanDevice>();
        m_device->Initialize(m_instance, tempSurface);

#ifdef _WIN32
        vkDestroySurfaceKHR(m_instance->GetInstance(), tempSurface, nullptr);
#endif

        m_surface = std::make_shared<VulkanSurface>();
        m_surface->Initialize(m_instance, m_device, m_window);
    }

    void InitializeTextureManager()
    {
        m_imageManager.Initialize(m_instance, m_device, m_allocator);
        m_imageViewManager.Initialize(m_instance, m_device);
        m_textureManager.Initialize(&m_imageManager, &m_imageViewManager,
            m_device.get(), m_commandBuffer.get());

        m_brickTexture = m_textureManager.GetTexture("Textures/brick.jpg");
     
        if (m_brickTexture) {
            std::cout << "   ImageView: " << (void*)m_brickTexture->GetImageView() << "\n";
            std::cout << "   Sampler: " << (void*)m_brickTexture->GetSampler() << "\n";
        }
        else {
            std::cout << "Texture pointer is NULL\n";
        }
    
    }
    void InitializeSwapchainAndRenderPass()
    {
        m_swapchain = std::make_shared<VulkanSwapchain>();
        m_swapchain->Initialize(m_instance, m_device, m_surface);

        m_renderPass = std::make_shared<VulkanRenderPass>();
        RenderPassConfig config = RenderPassConfig::SingleColorAttachment(m_swapchain->GetFormat());
        m_renderPass->Initialize(m_instance, m_device, config);
    }

    void InitializeFramebuffers()
    {
        const uint32_t imageCount = m_swapchain->GetImageCount();
        m_framebuffers.resize(imageCount);

        for (uint32_t i = 0; i < imageCount; ++i)
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
    }

   bool InitalizeImGui()
    {
        VkDescriptorPoolSize pool_sizes[] = {
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1}
        }; 

        VkDescriptorPoolCreateInfo info{}; 
        info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO; 
        info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        info.maxSets = 1; 
        info.poolSizeCount = 1; 
        info.pPoolSizes = pool_sizes; 

        VkResult result = vkCreateDescriptorPool(m_device->GetDevice(), &info, nullptr, &m_imguiPool);

        if (result != VK_SUCCESS)
        {
            return false; 
        }

        IMGUI_CHECKVERSION(); 

        ImGui::CreateContext(); 
        ImGuiIO& io = ImGui::GetIO(); 
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

        auto winWindow = dynamic_cast<Win32Window*>(m_window.get()); 

        ImGui_ImplWin32_Init(winWindow->GetHWND()); 

        ImGui_ImplVulkan_InitInfo init_info = {};
        
        init_info.ApiVersion = VK_API_VERSION_1_3;  
        init_info.Instance = m_instance->GetInstance();
        init_info.PhysicalDevice = m_device->GetPhysicalDevice();
        init_info.Device = m_device->GetDevice();
        init_info.QueueFamily = m_device->GetGraphicsQueueFamily();
        init_info.Queue = m_device->GetGraphicsQueue();
        init_info.DescriptorPool = m_imguiPool;
        init_info.MinImageCount = 2;
        init_info.ImageCount = m_swapchain->GetImageCount();

        init_info.PipelineInfoMain.RenderPass = m_renderPass->GetRenderPass();  
        init_info.PipelineInfoMain.Subpass = 0;                                 
        init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;       

        init_info.PipelineCache = VK_NULL_HANDLE;
        init_info.Allocator = nullptr;
        init_info.CheckVkResultFn = nullptr;
        init_info.UseDynamicRendering = false;  

        ImGui_ImplVulkan_Init(&init_info);  


        return true; 
    }

    void InitializePipelineModel()
    {
        auto bindingDescription = ModelVertex::GetBindingDescription();
        auto attributeDescriptions = ModelVertex::GetAttributeDescriptions();

        m_pipeline = std::make_shared<VulkanGraphicsPipeline>();
        GraphicsPipelineConfig modelConfig = GraphicsPipelineConfig::SimpleTriangle(
            "Shaders/model.vert.spv",
            "Shaders/model.frag.spv"
        );

        modelConfig.vertexInput.bindings = { bindingDescription };
        modelConfig.vertexInput.attributes = { attributeDescriptions.begin(), attributeDescriptions.end() };

        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(glm::mat4) + sizeof(glm::vec3);
        modelConfig.pushConstantRanges = { pushConstantRange };
        modelConfig.viewport = m_swapchain->GetExtent();
        modelConfig.cullMode = VK_CULL_MODE_FRONT_BIT; 

        m_pendingPipelineConfig = modelConfig;
    }

    void InitializeCommandsAndSync()
    {
        m_commandBuffer = std::make_shared<VulkanCommandBuffer>();
        m_commandBuffer->Initialize(m_instance, m_device, m_device->GetGraphicsQueueFamily());

        const uint32_t imageCount = m_swapchain->GetImageCount();
        m_sync = std::make_shared<VulkanSynchronization>();
        m_sync->Initialize(m_instance, m_device, 3, imageCount);

        m_commandBuffers = m_commandBuffer->AllocateCommandBuffers(imageCount);
        m_currentFrame = 0;
    }

    void InitializeMemoryAndGeometry()
    {
        std::vector<Vertex> vertices = {
            {{ 0.0f, -0.7f, 0.0f }, { 1.0f, 0.0f, 0.0f }},
            {{ 0.5f,  0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f }},
            {{-0.5f,  0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }}
        };

        m_camera = std::make_unique<Camera>(
            glm::vec3(0.0f, 0.0f, 50.0f),
            glm::vec3(0.0f, 1.0f, 0.0f),
            -90.0f,
            0.0f
        );

        maxFOV = m_camera->GetSettings().maxFov;
        m_allocator = std::make_shared<VulkanMemoryAllocator>();
        m_allocator->Initialize(m_instance, m_device);

        m_allocator->CreateVertexBuffer(
            m_commandBuffer.get(),
            vertices.data(),
            sizeof(Vertex) * vertices.size(),
            m_vertexBuffer
        );

        m_allocator->CreateUniformBuffer(
            sizeof(CameraUBO),
            m_cameraUniformBuffer
        );

        LoadModel();
    }

    void InitializeDescriptors()
    {
        m_descriptor = std::make_shared<VulkanDescriptor>(); 
        m_descriptor->Initialize(m_instance, m_device);

        m_descriptor->AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);

        m_descriptor->AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);

        m_descriptor->Build(1);
        m_descriptor->BindBuffer(0, m_cameraUniformBuffer.buffer, sizeof(CameraUBO));

        m_descriptor->BindImage(1, m_brickTexture->GetImageView(), m_brickTexture->GetSampler());

        m_pendingPipelineConfig.descriptorSetLayouts = { m_descriptor->GetLayout() };
        m_pipeline->Initialize(m_instance, m_device, m_renderPass, m_pendingPipelineConfig);
    }

    struct PushConstants {
        glm::mat4 model;
        glm::vec3 light;
    };

    void DrawModel(VkCommandBuffer cmd)
    {
        CameraUBO cameraData{};
        cameraData.view = m_camera->GetViewMatrix();
        cameraData.projection = m_camera->GetProjectionMatrix(
            static_cast<float>(m_swapchain->GetExtent().width) /
            static_cast<float>(m_swapchain->GetExtent().height)
        );
        m_allocator->UploadDataToBuffer(m_cameraUniformBuffer, &cameraData, sizeof(CameraUBO));

        m_pipeline->Bind(cmd);

        VkDescriptorSet descriptorSet = m_descriptor->GetSet();
        vkCmdBindDescriptorSets(
            cmd,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            m_pipeline->GetLayout(),
            0, 1,
            &descriptorSet,
            0, nullptr
        );

        VkBuffer vertexBuffers[] = { m_modelVertexBuffer.buffer };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(cmd, m_ModelIndexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::rotate(model, glm::radians(180.0f), glm::vec3(1, 0, 0));
        model = glm::rotate(model, glm::radians(m_rotation), glm::vec3(0, 1, 0));

        PushConstants ps{}; 
        ps.model = model;
        ps.light = glm::vec3(5.0f , 5.0f, 5.0f);

        vkCmdPushConstants(
            cmd,
            m_pipeline->GetLayout(),
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            0,
            sizeof(PushConstants),
            &ps
        );

        vkCmdDrawIndexed(cmd, m_modelIndexCount, 1, 0, 0, 0);
    }

    void LoadModel()
    {
        std::cout << "LoadModel: START\n";
        auto loader = ModelLoader::CreateLoader("Models/Residential Buildings 010.obj");

        if (loader && loader->Load("Models/Residential Buildings 010.obj", m_model))
        {
            std::cout << "After Load:\n";
            std::cout << "  Vertices: " << m_model.vertices.size() << "\n";
            std::cout << "  Indices: " << m_model.indices.size() << "\n";

            if (m_model.vertices.empty() || m_model.indices.empty())
            {
                std::cout << "ERROR: Invalid model data\n";
                return;
            }

            bool vbResult = m_allocator->CreateVertexBuffer(
                m_commandBuffer.get(),
                m_model.vertices.data(),
                m_model.GetVertexBufferSize(),
                m_modelVertexBuffer
            );

            bool ibResult = m_allocator->CreateIndexBuffer(
                m_commandBuffer.get(),
                m_model.indices.data(),
                m_model.GetIndexBufferSize(),
                VK_INDEX_TYPE_UINT32,
                m_ModelIndexBuffer
            );

            m_modelIndexCount = static_cast<uint32_t>(m_model.GetIndexCount());
        }
    }

    void HookInput()
    {
        m_window->SetUpMouseAndKeyboard();
        m_window->OnKeyEvent([&](int keyCode, bool isPressed) {
            Debug::DebugOutput::outputDebug("Key pressed: " + std::to_string(keyCode));
            });
    }

    void LogInitSummary() const
    {
        std::cout << "Initialization complete\n";
        std::cout << m_surface->GetSurfaceInfo() << "\n";
        std::cout << m_swapchain->GetSwapchainInfo() << "\n";
        std::cout << m_renderPass->GetRenderPassInfo() << "\n";
        std::cout << m_pipeline->GetPipelineInfo() << "\n";
    }

    void BeginRenderPass(VkCommandBuffer cmd, uint32_t imageIndex)
    {
        auto clearValues = m_renderPass->GetDefaultClearValues();
        m_renderPass->Begin(
            cmd,
            m_framebuffers[imageIndex]->GetFramebuffer(),
            m_swapchain->GetExtent(),
            clearValues
        );
    }

    void EndRenderPass(VkCommandBuffer cmd)
    {
        m_renderPass->End(cmd);
    }

    void CameraMovement(float deltaTime)
    {
        if (Input::Get().IsKeyDown(VK::W))
            m_camera->ProcessKeyboard(CameraMovement::Forward, deltaTime);
        if (Input::Get().IsKeyDown(VK::S))
            m_camera->ProcessKeyboard(CameraMovement::Backward, deltaTime);
        if (Input::Get().IsKeyDown(VK::A))
            m_camera->ProcessKeyboard(CameraMovement::Left, deltaTime);
        if (Input::Get().IsKeyDown(VK::D))
            m_camera->ProcessKeyboard(CameraMovement::Right, deltaTime);
        if (Input::Get().IsKeyDown(VK::Space))
            m_camera->ProcessKeyboard(CameraMovement::Up, deltaTime);
        if (Input::Get().IsKeyDown(VK::Ctrl))
            m_camera->ProcessKeyboard(CameraMovement::Down, deltaTime);

        float mouseX = Input::Get().GetMouseDeltaX();
        float mouseY = Input::Get().GetMouseDeltaY();
        if (mouseX != 0.0f || mouseY != 0.0f)
            m_camera->ProcessMouseMovement(mouseX, mouseY);

        float scroll = Input::Get().GetScrollDelta();
        if (scroll != 0.0f)
            m_camera->ProcessMouseScroll(scroll);
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
    std::shared_ptr<VulkanDescriptor> m_descriptor;
    std::unique_ptr<Camera> m_camera;

    VkDescriptorPool m_imguiPool = VK_NULL_HANDLE;

    std::vector<VkCommandBuffer> m_commandBuffers;
    uint32_t m_currentFrame = 0;

    AllocatedBuffer m_vertexBuffer;
    AllocatedBuffer m_cameraUniformBuffer;
    AllocatedBuffer m_ModelIndexBuffer;
    AllocatedBuffer m_modelVertexBuffer;

    TextureManager m_textureManager; 
    VulkanImage m_imageManager; 
    VulkanImageView m_imageViewManager; 

    std::shared_ptr<Texture> m_brickTexture;

    GraphicsPipelineConfig m_pendingPipelineConfig{};
    Model::ModelMesh m_model;
    uint32_t m_modelIndexCount = 0;
    float m_rotation = 0.0f;
    int maxFOV = 90; 
    float r{ 0 }, b{ 0 }, g{ 0 }, alpha{ 1 };
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
