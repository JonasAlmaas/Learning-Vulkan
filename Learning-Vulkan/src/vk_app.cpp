#include "vk_app.hpp"

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <array>
#include <stddef.h>
#include <stdexcept>
#include <stdint.h>
#include <set>
#include <vector>
#include <iostream>
#include <optional>

#if _DEBUG
enum {ENABLE_VALIDATION_LAYERS=1};
#else
enum {ENABLE_VALIDATION_LAYERS=0};
#endif

static const std::array<const char *, 1> s_validation_layers = {
	"VK_LAYER_KHRONOS_validation"
};

static bool check_validation_layer_support()
{
	uint32_t layer_count;
	vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

	std::vector<VkLayerProperties> available_layers(layer_count);
	vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

	for (const auto layer_name : s_validation_layers) {
		bool found = false;

		for (const auto &layer_properties : available_layers) {
			if (strcmp(layer_name, layer_properties.layerName) == 0) {
				found = true;
				break;
			}
		}

		if (!found) {
			return false;
		}
	}

	return true;
}

static VkResult create_debug_utils_messenger_ext(
	VkInstance instance,
	const VkDebugUtilsMessengerCreateInfoEXT *create_info,
	const VkAllocationCallbacks *allocator,
	VkDebugUtilsMessengerEXT *debug_messenger)
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	return func != nullptr
		? func(instance, create_info, allocator, debug_messenger)
		: VK_ERROR_EXTENSION_NOT_PRESENT;
}

static void destroy_debug_utils_messenger_ext(
	VkInstance instance,
	VkDebugUtilsMessengerEXT debug_messenger,
	const VkAllocationCallbacks *allocator)
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) {
		func(instance, debug_messenger, allocator);
	}
}

static std::vector<const char *> get_required_extensions()
{
	uint32_t glfw_extension_count = 0;
	const char **glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
	std::vector<const char *> extensions(glfw_extensions, glfw_extensions + glfw_extension_count);
	if (ENABLE_VALIDATION_LAYERS) {
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}
	return extensions;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
	VkDebugUtilsMessageTypeFlagsEXT message_type,
	const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
	void *user_data)
{
	std::cerr << "Validation layer: " << callback_data->pMessage << '\n';
	return VK_FALSE;
}

static void populate_debug_messenger_create_info(VkDebugUtilsMessengerCreateInfoEXT &create_info)
{
	create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
		| VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
		| VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
		| VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
		| VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	create_info.pfnUserCallback = debugCallback;
}

struct queue_family_indices
{
	std::optional<uint32_t> graphics_family;
	std::optional<uint32_t> present_family;

	bool is_complete() const
	{
		return this->graphics_family.has_value() && this->present_family.has_value();
	}
};

static queue_family_indices find_queue_families(VkPhysicalDevice device, VkSurfaceKHR surface)
{
	queue_family_indices indices;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	int i = 0;
	for (const auto &queueFamily : queueFamilies) {
		if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			indices.graphics_family = i;
		}

		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

		if (presentSupport) {
			indices.present_family = i;
		}

		if (indices.is_complete()) {
			break;
		}

		++i;
	}

	return indices;
}

static bool is_device_suitable(VkPhysicalDevice device, VkSurfaceKHR surface)
{
	queue_family_indices indices = find_queue_families(device, surface);
	return indices.is_complete();
}

void vk_app::run()
{
	window_init();
	vulkan_init();

	loop();

	vulkan_deinit();
	window_deinit();
}

void vk_app::loop()
{
	while (this->running) {
		glfwPollEvents();
		
		if (glfwWindowShouldClose(this->window)) {
			this->running = false;
		}
	}
}

void vk_app::window_init()
{
	if (!glfwInit()) {
		exit(EXIT_FAILURE);
	}

	if (!glfwVulkanSupported()) {
		exit(EXIT_FAILURE);
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	this->window = glfwCreateWindow(
		this->window_width,
		this->window_height,
		"Vulkan",
		nullptr,
		nullptr);

	if (!this->window) {
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwSetKeyCallback(this->window, [](GLFWwindow *window, int key_code, int scancode, int action, int mods) {

	});
	/*glfwSetKeyCallback(this->window, [](GLFWwindow *window, int key_code, int scancode, int action, int mods) {
		WindowData *data = (WindowData*)glfwGetWindowUserPointer(window);

		switch (action) {
		case GLFW_PRESS: {
			key_pressed_event e((elm::key)key_code, 0);
			data->event_callback(e);
			break;
		}
		case GLFW_RELEASE: {
			key_released_event e((elm::key)key_code);
			data->event_callback(e);
			break;
		}
		case GLFW_REPEAT: {
			key_pressed_event e((elm::key)key_code, 1);
			data->event_callback(e);
			break;
		}
		}
	});*/
}

void vk_app::window_deinit()
{
	glfwDestroyWindow(this->window);
	glfwTerminate();
}

void vk_app::vulkan_init()
{
	create_instance();
	setup_debug_messenger();
	create_surface();
	pick_physical_device();
	create_logical_device();
}

void vk_app::vulkan_deinit()
{
	vkDestroyDevice(this->vk_device, nullptr);

	if (ENABLE_VALIDATION_LAYERS) {
		destroy_debug_utils_messenger_ext(this->vk_instance, this->vk_debug_messenger, nullptr);
	}

	vkDestroySurfaceKHR(this->vk_instance, this->vk_surface, nullptr);
	vkDestroyInstance(this->vk_instance, nullptr);
}

void vk_app::create_instance()
{
	if (ENABLE_VALIDATION_LAYERS && !check_validation_layer_support()) {
		throw std::runtime_error("Validation layer requested, but not available");
	}

	VkApplicationInfo app_info = {};
	app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.pApplicationName = "Learning Vulkan";
	app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.pEngineName = "No Engine";
	app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	create_info.pApplicationInfo = &app_info;

	auto extensions = get_required_extensions();
	create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	create_info.ppEnabledExtensionNames = extensions.data();

	VkDebugUtilsMessengerCreateInfoEXT debug_create_info = {};
	if (ENABLE_VALIDATION_LAYERS) {
		create_info.enabledLayerCount = static_cast<uint32_t>(s_validation_layers.size());
		create_info.ppEnabledLayerNames = s_validation_layers.data();

		populate_debug_messenger_create_info(debug_create_info);
		create_info.pNext = (VkDebugUtilsMessengerCreateInfoEXT *)&debug_create_info;
	} else {
		create_info.enabledLayerCount = 0;
		create_info.pNext = nullptr;
	}

	if (vkCreateInstance(&create_info, nullptr, &vk_instance) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create instance");
	}
}

void vk_app::setup_debug_messenger()
{
	if (!ENABLE_VALIDATION_LAYERS) {
		return;
	}

	VkDebugUtilsMessengerCreateInfoEXT create_info;
	populate_debug_messenger_create_info(create_info);

	if (create_debug_utils_messenger_ext(
			this->vk_instance,
			&create_info,
			nullptr,
			&this->vk_debug_messenger) != VK_SUCCESS) {
		throw std::runtime_error("Failed to set up debug messenger");
	}
}

void vk_app::create_surface()
{
	if (glfwCreateWindowSurface(
			this->vk_instance,
			this->window,
			nullptr,
			&this->vk_surface) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create window surface");
	}
}

void vk_app::pick_physical_device()
{
	uint32_t device_count = 0;
	vkEnumeratePhysicalDevices(this->vk_instance, &device_count, nullptr);

	if (device_count == 0) {
		throw std::runtime_error("Failed to find GPUs with Vulkan support");
	}

	std::vector<VkPhysicalDevice> devices(device_count);
	vkEnumeratePhysicalDevices(this->vk_instance, &device_count, devices.data());

	for (const auto &device : devices) {
		if (is_device_suitable(device, this->vk_surface)) {
			this->vk_physical_device = device;
			break;
		}
	}

	if (this->vk_physical_device == VK_NULL_HANDLE) {
		throw std::runtime_error("Failed to find a suitable GPU");
	}
}

void vk_app::create_logical_device()
{
	auto indices = find_queue_families(this->vk_physical_device, this->vk_surface);

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> unique_queue_families = { indices.graphics_family.value(), indices.present_family.value() };

	float queue_priority = 1.0f;
	for (uint32_t queue_family : unique_queue_families) {
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queue_family;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queue_priority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkPhysicalDeviceFeatures deviceFeatures = {};

	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pQueueCreateInfos = queueCreateInfos.data();

	createInfo.pEnabledFeatures = &deviceFeatures;

	createInfo.enabledExtensionCount = 0;

	if (ENABLE_VALIDATION_LAYERS) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(s_validation_layers.size());
		createInfo.ppEnabledLayerNames = s_validation_layers.data();
	} else {
		createInfo.enabledLayerCount = 0;
	}

	if (vkCreateDevice(this->vk_physical_device, &createInfo, nullptr, &this->vk_device) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create logical device!");
	}

	vkGetDeviceQueue(this->vk_device, indices.graphics_family.value(), 0, &this->vk_graphics_queue);
	vkGetDeviceQueue(this->vk_device, indices.present_family.value(), 0, &this->vk_present_queue);
}
