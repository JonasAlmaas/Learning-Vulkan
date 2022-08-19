#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>


class HelloTriangleApplication
{
public:
	void Run();

private:
    void InitWindow();

    void InitVulkan();
    void CreateInstance();
    void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
    void SetupDebugMessenger();
    bool CheckValidationLayerSupport();
    std::vector<const char*> GetRequiredExtensions();
    void PickPhysicalDevice();

    void MainLoop();

    void Cleanup();

private:
    GLFWwindow* m_Window;
    uint32_t m_WindowWidth = 800;
    uint32_t m_WindowHeight = 600;

    VkInstance m_VKInstance;
    VkDebugUtilsMessengerEXT m_VKDebugMessenger;

};
