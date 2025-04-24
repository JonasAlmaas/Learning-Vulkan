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
#include <iomanip>
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
	return func
		? func(instance, create_info, allocator, debug_messenger)
		: VK_ERROR_EXTENSION_NOT_PRESENT;
}

static void destroy_debug_utils_messenger_ext(
	VkInstance instance,
	VkDebugUtilsMessengerEXT debug_messenger,
	const VkAllocationCallbacks *allocator)
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func) {
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

static const char *vk_get_debug_severity(VkDebugUtilsMessageSeverityFlagBitsEXT severity)
{
	switch (severity) {
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: return "Verbose";
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: return "Info";
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: return "Warning";
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: return "Error";
	default: return "(UNKNOWN SEVERITY)";
	}
}

static const char *vk_get_debug_type(VkDebugUtilsMessageTypeFlagsEXT type)
{
	switch (type) {
	case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT: return "General";
	case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT: return "Validation";
	case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT: return "Performance";
	case VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT: return "Device address binding";
	default: return "(UNKNOWN TYPE)";
	}
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
	VkDebugUtilsMessageSeverityFlagBitsEXT severity,
	VkDebugUtilsMessageTypeFlagsEXT type,
	const VkDebugUtilsMessengerCallbackDataEXT *data,
	void *user_data)
{
	std::cerr << "Debug callback: " << data->pMessage << "\n";
	std::cerr << "  Severity: " << vk_get_debug_severity(severity) << "\n";
	std::cerr << "  Severity: " << vk_get_debug_type(type) << "\n";
	std::cerr << "  Objects ";
	
	for (uint32_t i=0u; i<data->objectCount; ++i) {
		std::cerr << std::hex << std::setw(16) << std::setfill('0') << data->pObjects[i].objectHandle << ' ';
	}
	
	std::cout << std::dec << std::setfill(' ') << '\n';

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
	create_info.pfnUserCallback = debug_callback;
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
		/*WindowData *data = (WindowData *)glfwGetWindowUserPointer(window);*/

		switch (action) {
		case GLFW_PRESS: {
			/*key_pressed_event e((elm::key)key_code, 0);
			data->event_callback(e);*/
			break;
		}
		case GLFW_RELEASE: {
			/*key_released_event e((elm::key)key_code);
			data->event_callback(e);*/
			break;
		}
		case GLFW_REPEAT: {
			/*key_pressed_event e((elm::key)key_code, 1);
			data->event_callback(e);*/
			break;
		}
		}
	});
}

void vk_app::window_deinit()
{
	glfwDestroyWindow(this->window);
	this->window = nullptr;
	glfwTerminate();
}

static VkInstance create_instance()
{
	if (ENABLE_VALIDATION_LAYERS && !check_validation_layer_support()) {
		throw std::runtime_error("Validation layer requested, but not available");
	}

	auto extensions = get_required_extensions();

	VkApplicationInfo app_info = {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pApplicationName = "Learning Vulkan",
		.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
		.pEngineName = "No Engine",
		.engineVersion = VK_MAKE_VERSION(1, 0, 0),
		.apiVersion = VK_API_VERSION_1_0};

	VkInstanceCreateInfo create_info = {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pApplicationInfo = &app_info,
		.enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
		.ppEnabledExtensionNames = extensions.data()};

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

	VkInstance instance;
	if (vkCreateInstance(&create_info, nullptr, &instance) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create instance");
	}

	return instance;
}

static VkDebugUtilsMessengerEXT setup_debug_messenger(VkInstance instance)
{
	if (!ENABLE_VALIDATION_LAYERS) {
		return VK_NULL_HANDLE;
	}

	VkDebugUtilsMessengerCreateInfoEXT create_info;
	populate_debug_messenger_create_info(create_info);

	VkDebugUtilsMessengerEXT messenger;
	if (create_debug_utils_messenger_ext(
			instance,
			&create_info,
			nullptr,
			&messenger) != VK_SUCCESS) {
		throw std::runtime_error("Failed to set up debug messenger");
	}

	return messenger;
}

static VkSurfaceKHR create_surface(VkInstance instance, GLFWwindow *window)
{
	VkSurfaceKHR surface;
	if (glfwCreateWindowSurface(
			instance,
			window,
			nullptr,
			&surface) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create window surface");
	}
	return surface;
}

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

static VkPhysicalDevice pick_physical_device(VkInstance instance, VkSurfaceKHR surface)
{
	uint32_t device_count = 0;
	vkEnumeratePhysicalDevices(instance, &device_count, nullptr);

	if (device_count == 0) {
		throw std::runtime_error("Failed to find GPUs with Vulkan support");
	}

	std::vector<VkPhysicalDevice> devices(device_count);
	vkEnumeratePhysicalDevices(instance, &device_count, devices.data());

	VkPhysicalDevice ret = VK_NULL_HANDLE;
	for (const auto &device : devices) {
		if (is_device_suitable(device, surface)) {
			ret = device;
			break;
		}
	}

	if (ret == VK_NULL_HANDLE) {
		throw std::runtime_error("Failed to find a suitable GPU");
	}

	return ret;
}

static VkDevice create_logical_device(
	VkSurfaceKHR surface,
	VkPhysicalDevice physical_device,
	VkQueue *graphics_queue,
	VkQueue *present_queue,
	const queue_family_indices &indices)
{
	std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
	std::set<uint32_t> unique_queue_families = { indices.graphics_family.value(), indices.present_family.value() };

	float queue_priority = 1.0f;
	for (uint32_t queue_family : unique_queue_families) {
		VkDeviceQueueCreateInfo queue_create_info = {
			.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			.queueFamilyIndex = queue_family,
			.queueCount = 1,
			.pQueuePriorities = &queue_priority};
		queue_create_infos.push_back(queue_create_info);
	}

	std::vector<const char *> dev_exts = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME};

	VkPhysicalDeviceFeatures deviceFeatures = {
		/*.geometryShader = VK_TRUE,
		.tessellationShader = VK_TRUE*/};

	VkDeviceCreateInfo create_info = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.queueCreateInfoCount = (uint32_t)queue_create_infos.size(),
		.pQueueCreateInfos = queue_create_infos.data(),
		.enabledLayerCount = 0, // DEPRECATED
		.ppEnabledLayerNames = nullptr, // DEPRECATED
		.enabledExtensionCount = (uint32_t)dev_exts.size(),
		.ppEnabledExtensionNames = dev_exts.data(),
		.pEnabledFeatures = &deviceFeatures};

	if (ENABLE_VALIDATION_LAYERS) {
		create_info.enabledLayerCount = static_cast<uint32_t>(s_validation_layers.size());
		create_info.ppEnabledLayerNames = s_validation_layers.data();
	}

	VkDevice device;
	if (vkCreateDevice(physical_device, &create_info, nullptr, &device) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create logical device");
	}

	vkGetDeviceQueue(device, indices.graphics_family.value(), 0, graphics_queue);
	vkGetDeviceQueue(device, indices.present_family.value(), 0, present_queue);

	return device;
}

static VkImageView create_image_view(
	VkDevice device,
	VkImage image,
	VkFormat format,
	VkImageAspectFlags aspect_flags,
	VkImageViewType view_type,
	uint32_t layer_count,
	uint32_t mip_levels)
{
	VkImageViewCreateInfo create_info = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.image = image,
		.viewType = view_type,
		.format = format,
		.components = {
			.r = VK_COMPONENT_SWIZZLE_IDENTITY,
			.g = VK_COMPONENT_SWIZZLE_IDENTITY,
			.b = VK_COMPONENT_SWIZZLE_IDENTITY,
			.a = VK_COMPONENT_SWIZZLE_IDENTITY},
		.subresourceRange = {
			.aspectMask = aspect_flags,
			.baseMipLevel = 0,
			.levelCount = mip_levels,
			.baseArrayLayer = 0,
			.layerCount = layer_count}
	};

	VkImageView view;
	if (vkCreateImageView(device, &create_info, nullptr, &view) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create image view");
	}
	return view;
}

static VkSwapchainKHR create_swap_chain(
	VkPhysicalDevice physical_device,
	VkDevice device,
	VkSurfaceKHR surface,
	const queue_family_indices &indices,
	std::vector<VkImage> &swapchain_images,
	std::vector<VkImageView> &swapchain_image_views)
{
	VkSurfaceCapabilitiesKHR surface_caps;
	if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &surface_caps) != VK_SUCCESS) {
		throw std::runtime_error("Failed to get physical device surface capabilities");
	}
	uint32_t num_images = surface_caps.minImageCount + 1;
	if (num_images > surface_caps.maxImageCount) {
		--num_images;
	}

	/* Get desired surface format if supported */
	uint32_t num_formats;
	std::vector<VkSurfaceFormatKHR> surface_formats;
	if (vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &num_formats, nullptr) != VK_SUCCESS) {
		throw std::runtime_error("Failed to get physical device surface formats");
	}
	surface_formats.resize(num_formats);
	if (vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &num_formats, surface_formats.data()) != VK_SUCCESS) {
		throw std::runtime_error("Failed to get physical device surface formats");
	}
	VkSurfaceFormatKHR surface_format = surface_formats[0];
	for (const auto &format : surface_formats) {
		if (format.format == VK_FORMAT_R8G8B8A8_SRGB
				&& format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			surface_format = format;
			break;
		}
	}

	/* Get desired present mode if supported */
	uint32_t num_present_modes;
	std::vector<VkPresentModeKHR> present_modes;
	if (vkGetPhysicalDeviceSurfacePresentModesKHR(
			physical_device,
			surface,
			&num_present_modes,
			nullptr) != VK_SUCCESS) {
		throw std::runtime_error("Failed to get physical device surface present modes");
	}
	present_modes.resize(num_present_modes);
	if (vkGetPhysicalDeviceSurfacePresentModesKHR(
			physical_device,
			surface,
			&num_present_modes,
			present_modes.data()) != VK_SUCCESS) {
		throw std::runtime_error("Failed to get physical device surface present modes");
	}
	VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
	for (const auto &pm : present_modes) {
		if (pm == VK_PRESENT_MODE_MAILBOX_KHR) {
			present_mode = pm;
			break;
		}
	}

	VkSwapchainCreateInfoKHR create_info = {
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.pNext = nullptr,
		.flags = 0,
		.surface = surface,
		.minImageCount = num_images,
		.imageFormat = surface_format.format,
		.imageColorSpace = surface_format.colorSpace,
		.imageExtent = surface_caps.currentExtent,
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
			| VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 1,
		.pQueueFamilyIndices = &indices.graphics_family.value(),
		.preTransform = surface_caps.currentTransform,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = present_mode,
		.clipped = VK_TRUE};

	VkSwapchainKHR swapchain;
	if (vkCreateSwapchainKHR(device, &create_info, nullptr, &swapchain) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create swapchain");
	}

	uint32_t num_swapchain_images = 0;
	if (vkGetSwapchainImagesKHR(device, swapchain, &num_swapchain_images, nullptr) != VK_SUCCESS) {
		throw std::runtime_error("Failed to get swapchain images");
	}
	swapchain_images.resize(num_swapchain_images);
	swapchain_image_views.resize(num_swapchain_images);
	if (vkGetSwapchainImagesKHR(device, swapchain, &num_swapchain_images, swapchain_images.data()) != VK_SUCCESS) {
		throw std::runtime_error("Failed to get swapchain images");
	}

	int layer_count = 1;
	int mip_levels = 1;
	for (uint32_t i=0u; i<num_swapchain_images; ++i) {
		swapchain_image_views[i] = create_image_view(
			device,
			swapchain_images[i],
			surface_format.format,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_VIEW_TYPE_2D,
			layer_count,
			mip_levels);
	}

	return swapchain;
}

static VkCommandPool create_cmd_pool(VkDevice device, const queue_family_indices &indices)
{
	VkCommandPoolCreateInfo create_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.queueFamilyIndex = indices.graphics_family.value()};

	VkCommandPool cmd_pool;
	if (vkCreateCommandPool(device, &create_info, nullptr, &cmd_pool) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create command pool");
	}

	return cmd_pool;
}

void vk_app::vulkan_init()
{
	this->vk_instance = create_instance();
	this->vk_debug_messenger = setup_debug_messenger(this->vk_instance);
	this->vk_surface = create_surface(this->vk_instance, this->window);
	this->vk_physical_device = pick_physical_device(this->vk_instance, this->vk_surface);
	auto indices = find_queue_families(this->vk_physical_device, this->vk_surface);
	this->vk_device = create_logical_device(
		this->vk_surface,
		this->vk_physical_device,
		&this->vk_graphics_queue,
		&this->vk_present_queue,
		indices);
	this->vk_swapchain = create_swap_chain(
		this->vk_physical_device,
		this->vk_device,
		this->vk_surface,
		indices,
		this->vk_swapchain_images,
		this->vk_swapchain_image_views);
	this->vk_cmd_pool = create_cmd_pool(this->vk_device, indices);
}

void vk_app::vulkan_deinit()
{
	for (auto &image : this->vk_swapchain_image_views) {
		vkDestroyImageView(this->vk_device, image, nullptr);
	}
	this->vk_swapchain_image_views.clear();

	vkDestroySwapchainKHR(this->vk_device, this->vk_swapchain, nullptr);
	this->vk_device = VK_NULL_HANDLE;

	vkDestroyDevice(this->vk_device, nullptr);
	this->vk_device = VK_NULL_HANDLE;

	if (ENABLE_VALIDATION_LAYERS) {
		destroy_debug_utils_messenger_ext(this->vk_instance, this->vk_debug_messenger, nullptr);
		this->vk_debug_messenger = VK_NULL_HANDLE;
	}

	vkDestroySurfaceKHR(this->vk_instance, this->vk_surface, nullptr);
	this->vk_surface = VK_NULL_HANDLE;

	vkDestroyInstance(this->vk_instance, nullptr);
	this->vk_instance = VK_NULL_HANDLE;
}
