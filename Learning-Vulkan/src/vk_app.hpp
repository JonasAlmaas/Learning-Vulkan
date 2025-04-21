#pragma once

#include <vulkan/vulkan_core.h>

#include <stdint.h>

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

	void create_instance();
	void setup_debug_messenger();
	void create_surface();
	void pick_physical_device();
	void create_logical_device();

private:
	bool running;

	uint32_t window_width, window_height;
	struct GLFWwindow *window = nullptr;

	VkInstance vk_instance = nullptr;
	VkDebugUtilsMessengerEXT vk_debug_messenger = nullptr;
	VkSurfaceKHR vk_surface = nullptr;
	VkPhysicalDevice vk_physical_device = nullptr;
	VkDevice vk_device = nullptr;
	VkQueue vk_graphics_queue = nullptr;
	VkQueue vk_present_queue = nullptr;
};
