#pragma once

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <iostream>
#include <stdexcept>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <optional>


struct queue_family_indices
{
	std::optional<uint32_t> graphics_family;
	std::optional<uint32_t> present_family;

	bool is_complete() const
	{
		return this->graphics_family.has_value() && this->present_family.has_value();
	}
};

struct hello_triangle_application
{
	void run();

private:
	void loop();
	void cleanup();

	void init_window();

	void init_vulkan();
	void create_instance();

	void populate_debug_messenger_create_info(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
	void setup_debug_messenger();

	void create_surface();
	
	void pick_physical_device();
	bool is_device_suitable(VkPhysicalDevice device);
	queue_family_indices find_queue_families(VkPhysicalDevice device) const;

	void create_logical_device();

	std::vector<const char *> get_required_extensions();
	bool check_validation_layer_support();

private:
	GLFWwindow* window;
	uint32_t window_width = 800;
	uint32_t window_height = 600;

	VkInstance vk_instance;
	VkDebugUtilsMessengerEXT vk_debug_messenger;
	VkPhysicalDevice vk_physical_device = VK_NULL_HANDLE;
	VkDevice vk_device;
	VkQueue vk_graphics_queue;

	VkSurfaceKHR vk_surface;
	VkQueue vk_present_queue;
};
