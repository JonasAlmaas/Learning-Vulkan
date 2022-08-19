#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <optional>

struct QueueFamilyIndices
{
    std::optional<uint32_t> GraphicsFamily;

    bool IsComplete()
    {
        return GraphicsFamily.has_value();
    }
};

class HelloTriangleApplication
{
public:
	void Run();

private:
    void MainLoop();
    void Cleanup();

    void InitWindow();

    void InitVulkan();
    void CreateInstance();
    void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
    void SetupDebugMessenger();
    void PickPhysicalDevice();
    bool IsDeviceSuitable(VkPhysicalDevice device);
    QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);

    void CreateLogicalDevice();

    std::vector<const char*> GetRequiredExtensions();
    bool CheckValidationLayerSupport();

private:
    GLFWwindow* m_Window;
    uint32_t m_WindowWidth = 800;
    uint32_t m_WindowHeight = 600;

    VkInstance m_VKInstance;
    VkDebugUtilsMessengerEXT m_VKDebugMessenger;
    VkPhysicalDevice m_VKPhysicalDevice = VK_NULL_HANDLE;
    VkDevice m_VKDevice;
    VkQueue m_GraphicsQueue;
};
