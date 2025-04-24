#pragma once

#include <vulkan/vulkan_core.h>

#include <stdint.h>
#include <vector>

struct vk_app
{
	vk_app():
		running(true),
		window_width(800),
		window_height(600) {}

	void run();

private:
	void loop();

	void window_init();
	void window_deinit();

	void vulkan_init();
	void vulkan_deinit();

private:
	bool running;

	uint32_t window_width, window_height;
	struct GLFWwindow *window = nullptr;
	
	VkInstance vk_instance = VK_NULL_HANDLE;
	VkDebugUtilsMessengerEXT vk_debug_messenger = VK_NULL_HANDLE;
	VkSurfaceKHR vk_surface = VK_NULL_HANDLE;
	VkPhysicalDevice vk_physical_device = VK_NULL_HANDLE;
	VkDevice vk_device = VK_NULL_HANDLE;
	VkQueue vk_graphics_queue = VK_NULL_HANDLE;
	VkQueue vk_present_queue = VK_NULL_HANDLE;
	VkSwapchainKHR vk_swapchain = VK_NULL_HANDLE;
	std::vector<VkImage> vk_swapchain_images;
	std::vector<VkImageView> vk_swapchain_image_views;
	VkCommandPool vk_cmd_pool = VK_NULL_HANDLE;
};
