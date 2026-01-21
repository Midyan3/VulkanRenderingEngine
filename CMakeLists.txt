cmake_minimum_required(VERSION 3.21)
project(OwnGameEngine CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(VULKAN_SDK "C:/VulkanSDK/1.4.321.1")
set(ENV{VULKAN_SDK} "C:/VulkanSDK/1.4.321.1")

find_package(Vulkan REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)
# REMOVED: find_package(imgui CONFIG REQUIRED)

# ImGui Manual Setup
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/External/imgui)

set(STB_IMAGE_DIR ${CMAKE_SOURCE_DIR}/External/stb_image_header)

set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
    ${IMGUI_DIR}/backends/imgui_impl_win32.cpp
)

set(STB_IMAGE 
    ${STB_IMAGE_DIR}/stb_image.h
)

include_directories(${IMGUI_DIR})
include_directories(${IMGUI_DIR}/backends)

# Shader compilation setup
set(GLSLC_EXECUTABLE "${VULKAN_SDK}/Bin/glslc.exe")
set(SHADER_SOURCE_DIR "${CMAKE_SOURCE_DIR}/Shaders") 
set(SHADER_BINARY_DIR "${CMAKE_BINARY_DIR}/Shaders")

file(MAKE_DIRECTORY ${SHADER_BINARY_DIR})

file(GLOB_RECURSE SHADER_SOURCES
    "${SHADER_SOURCE_DIR}/*.vert"
    "${SHADER_SOURCE_DIR}/*.frag"
    "${SHADER_SOURCE_DIR}/*.comp"
)

foreach(SHADER ${SHADER_SOURCES})
    get_filename_component(SHADER_NAME ${SHADER} NAME)
    set(SHADER_OUTPUT "${SHADER_BINARY_DIR}/${SHADER_NAME}.spv")
    
    add_custom_command(
        OUTPUT ${SHADER_OUTPUT}
        COMMAND ${GLSLC_EXECUTABLE} ${SHADER} -o ${SHADER_OUTPUT}
        DEPENDS ${SHADER}
        COMMENT "Compiling shader: ${SHADER_NAME}"
    )
    
    list(APPEND SHADER_BINARIES ${SHADER_OUTPUT})
endforeach()

add_custom_target(CompileShaders ALL DEPENDS ${SHADER_BINARIES})

add_executable(OwnGameEngine
    App/main.cc
    Core/Application/Application.cc
    Core/Application/Application.h
    Core/Application/WindowSpec/WindowSpec.h
    Core/Application/WindowSpec/WindowSpec.cc
    Core/Window/Window.h
    Core/Window/Window.cc
    Core/Window/OS-Windows/Win32/Win32Window.h
    Core/Window/OS-Windows/Win32/Win32Window.cc
    Core/Window/OS-Windows/Win32/WindowManager/WindowManager.cc
    Core/Window/OS-Windows/Win32/WindowManager/WindowManager.h
    Core/Window/OS-Windows/WidnowSetting/WindowSetting.h
    Core/DebugOutput/DubugOutput.h 
    Core/Window/OS-Windows/Win32/WindowStateEnum/WindowStateEnum.h
    Core/Renderer/VulkanInstance/VulkanInstance.h
    Core/Renderer/VulkanInstance/VulkanInstance.cpp
    Core/Renderer/VulkanInstance/VkPropStructs/VkPropStructs.h
    Core/Renderer/VulkanDevice/VulkanDevice.h
    Core/Renderer/VulkanDevice/VulkanDevice.cpp
    Core/Renderer/VulkanSurface/VulkanSurface.h
    Core/Renderer/VulkanSurface/VulkanSurface.cpp
    Core/Renderer/VulkanSwapchain/VulkanSwapchain.h
    Core/Renderer/VulkanSwapchain/VulkanSwapchain.cpp
    Core/Renderer/VulkanCommandBuffer/VulkanCommandBuffer.h
    Core/Renderer/VulkanCommandBuffer/VulkanCommandBuffer.cpp
    Core/Renderer/VulkanMemoryAllocator/VulkanMemoryAllocator.h
    Core/Renderer/VulkanMemoryAllocator/VulkanMemoryAllocator.cpp
    Core/Renderer/VulkanRenderPass/VulkanRenderPass.h
    Core/Renderer/VulkanRenderPass/VulkanRenderPass.cpp
    Core/Renderer/VulkanFrameBuffer/VulkanFrameBuffer.h
    Core/Renderer/VulkanFrameBuffer/VulkanFrameBuffer.cpp
    Core/Renderer/VulkanRenderer/VulkanRenderer.h
    Core/Renderer/VulkanRenderer/VulkanRenderer.cpp
    Core/Renderer/VulkanGraphicsPipeline/VulkanGraphicsPipeline.cpp
    Core/Renderer/VulkanGraphicsPipeline/VulkanGraphicsPipeline.h
    Core/Renderer/VulkanSynchronization/VulkanSynchronization.cpp
    Core/Renderer/VulkanSynchronization/VulkanSynchronization.h
    Core/Renderer/VertexTypes/Vertex.h
    Core/Camera/Camera.h 
    Core/Camera/Camera.cpp 
    Core/Math/Quaternions.h
    Core/Renderer/VulkanDescriptor/VulkanDescriptor.h
    Core/Renderer/VulkanDescriptor/VulkanDescriptor.cpp
    Core/Input/Input.h
    Core/Input/Input.cpp
    Core/Loaders/OBJLoader/OBJLoader.cpp
    Core/Loaders/OBJLoader/OBJLoader.h
    Core/Loaders/ModelLoader.h
    Core/Loaders/Model.h
    Core/Loaders/ModelLoader.cpp
    Core/Renderer/VertexTypes/ModelVertex.h
    ${IMGUI_SOURCES}
    ${STB_IMAGE}
 "Core/Renderer/TextureLoader/Texture.cpp" "Core/Renderer/TextureLoader/Texture.h" "Core/Renderer/VulkanImage/VulkanImage.h" "Core/Renderer/VulkanImage/VulkanImage.cpp" "Core/Renderer/VulkanImageView/VulkanImageView.cpp" "Core/Renderer/VulkanImageView/VulkanImageView.h" "Core/TextureManager/Vulkan/TextureManager.cpp" "Core/TextureManager/Vulkan/TextureManager.h" "App/main.h" "Core/MaterialHandler/Material.cpp" "Core/MaterialHandler/Material.h")

add_custom_command(
    TARGET OwnGameEngine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/App/Models"
            "$<TARGET_FILE_DIR:OwnGameEngine>/Models"
)

add_custom_command(
    TARGET OwnGameEngine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/Textures"
            "$<TARGET_FILE_DIR:OwnGameEngine>/Textures"
)

add_dependencies(OwnGameEngine CompileShaders)

target_link_libraries(OwnGameEngine PRIVATE
    Vulkan::Vulkan
    glm::glm-header-only
    GPUOpen::VulkanMemoryAllocator
    # REMOVED: imgui::imgui
)