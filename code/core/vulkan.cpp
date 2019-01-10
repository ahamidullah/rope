#include <vulkan.h>
#include <ShaderLang.h>

#include <stdio.h>
#include <algorithm>

static VKAPI_ATTR VkBool32 VKAPI_CALL
vk_debug_callback(
    VkDebugReportFlagsEXT flags,
    VkDebugReportObjectTypeEXT object_type,
    uint64_t object,
    size_t location,
    int32_t code,
    const char* layer_prefix,
    const char* message,
    void *user_data)
{
    debug_print("%s\n", message);
    return VK_FALSE;
}

const char *
vk_result_to_string(VkResult r)
{
	switch(r) {
		case (VK_SUCCESS):
			return "VK_SUCCESS";
		case (VK_NOT_READY):
			return "VK_NOT_READY";
		case (VK_TIMEOUT):
			return "VK_TIMEOUT";
		case (VK_EVENT_SET):
			return "VK_EVENT_SET";
		case (VK_EVENT_RESET):
			return "VK_EVENT_RESET";
		case (VK_INCOMPLETE):
			return "VK_INCOMPLETE";
		case (VK_ERROR_OUT_OF_HOST_MEMORY):
			return "VK_ERROR_OUT_OF_HOST_MEMORY";
		case (VK_ERROR_INITIALIZATION_FAILED):
			return "VK_ERROR_INITIALIZATION_FAILED";
		case (VK_ERROR_DEVICE_LOST):
			return "VK_ERROR_DEVICE_LOST";
		case (VK_ERROR_MEMORY_MAP_FAILED):
			return "VK_ERROR_MEMORY_MAP_FAILED";
		case (VK_ERROR_LAYER_NOT_PRESENT):
			return "VK_ERROR_LAYER_NOT_PRESENT";
		case (VK_ERROR_EXTENSION_NOT_PRESENT):
			return "VK_ERROR_EXTENSION_NOT_PRESENT";
		case (VK_ERROR_FEATURE_NOT_PRESENT):
			return "VK_ERROR_FEATURE_NOT_PRESENT";
		case (VK_ERROR_INCOMPATIBLE_DRIVER):
			return "VK_ERROR_INCOMPATIBLE_DRIVER";
		case (VK_ERROR_TOO_MANY_OBJECTS):
			return "VK_ERROR_TOO_MANY_OBJECTS";
		case (VK_ERROR_FORMAT_NOT_SUPPORTED):
			return "VK_ERROR_FORMAT_NOT_SUPPORTED";
		case (VK_ERROR_FRAGMENTED_POOL):
			return "VK_ERROR_FRAGMENTED_POOL";
		case (VK_ERROR_OUT_OF_DEVICE_MEMORY):
			return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
		case (VK_ERROR_OUT_OF_POOL_MEMORY):
			return "VK_ERROR_OUT_OF_POOL_MEMORY";
		case (VK_ERROR_INVALID_EXTERNAL_HANDLE):
			return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
		case (VK_ERROR_SURFACE_LOST_KHR):
			return "VK_ERROR_SURFACE_LOST_KHR";
		case (VK_ERROR_NATIVE_WINDOW_IN_USE_KHR):
			return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
		case (VK_SUBOPTIMAL_KHR):
			return "VK_SUBOPTIMAL_KHR";
		case (VK_ERROR_OUT_OF_DATE_KHR):
			return "VK_ERROR_OUT_OF_DATE_KHR";
		case (VK_ERROR_INCOMPATIBLE_DISPLAY_KHR):
			return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
		case (VK_ERROR_VALIDATION_FAILED_EXT):
			return "VK_ERROR_VALIDATION_FAILED_EXT";
		case (VK_ERROR_INVALID_SHADER_NV):
			return "VK_ERROR_INVALID_SHADER_NV";
		case (VK_ERROR_NOT_PERMITTED_EXT):
			return "VK_ERROR_NOT_PERMITTED_EXT";
		case (VK_RESULT_RANGE_SIZE):
			return "VK_RESULT_RANGE_SIZE";
		case (VK_RESULT_MAX_ENUM):
			return "VK_RESULT_MAX_ENUM";
	}
	return "Unknown VkResult Code";
}

#define VK_DIE_IF_ERROR(fn, ...) \
{ \
	VkResult r = fn(__VA_ARGS__); \
	if (r != VK_SUCCESS) \
		_abort("Function %s failed with VkResult %s.\n", #fn, vk_result_to_string(r)); \
}

const char *
vk_physical_device_type_to_string(VkPhysicalDeviceType t)
{
	switch (t) {
		case VK_PHYSICAL_DEVICE_TYPE_OTHER: {
			return "VK_PHYSICAL_DEVICE_TYPE_OTHER";
		} break;
		case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: {
			return "VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU";
		} break;
		case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: {
			return "VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU";
		} break;
		case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU: {
			return "VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU";
		} break;
		case VK_PHYSICAL_DEVICE_TYPE_CPU: {
			return "VK_PHYSICAL_DEVICE_TYPE_CPU";
		} break;
		default: {
			debug_print("Unknown VkPhysicalDeviceType enum value %d.", t);
			return "Unknown VkPhysicalDeviceType enum value"; 
		}
	}
	return "Unknown device type";
}

struct Swapchain_Context {
	VkSwapchainKHR swapchain;
	VkExtent2D extent;
	Array<VkImageView> image_views;
	VkRenderPass render_pass;
	Array<VkFramebuffer> framebuffers;
	VkPipeline pipeline;
	VkPipelineLayout pipeline_layout;
	VkShaderModule vert_shader_module;
	VkShaderModule frag_shader_module;
};

struct Vulkan_Context {
	VkInstance instance;
	VkPhysicalDevice physical_device;
	s32 graphics_queue_family;
	s32 present_queue_family;
	VkDevice device;
	VkPresentModeKHR present_mode;
	VkSurfaceKHR surface;
	VkSurfaceCapabilitiesKHR surface_capabilities;
	VkSurfaceFormatKHR surface_format;
	VkQueue graphics_queue;
	VkQueue present_queue;
	VkCommandPool command_pool;
	Array<VkCommandBuffer> command_buffers;
	VkSemaphore image_available_semaphore;
	VkSemaphore render_finished_semaphore;
	VkDebugReportCallbackEXT debug_callback;
	VkBuffer vertex_buffer;
	VkDeviceMemory vertex_buffer_memory;
	VkBuffer index_buffer;
	VkDeviceMemory index_buffer_memory;
	VkDescriptorSetLayout descriptor_set_layout;
	VkBuffer uniform_buffer;
	VkDeviceMemory uniform_buffer_memory;
	VkDescriptorPool descriptor_pool;
	VkDescriptorSet descriptor_set;

	Swapchain_Context swapchain_context;
} vk_context;

#if 0
VkInstance
make_instance(Array<const char *>instance_extensions, Array<const char *>instance_layers)
{
	u32 num_available_instance_extensions = 0;
	VK_DIE_IF_ERROR(vkEnumerateInstanceExtensionProperties, NULL, &num_available_instance_extensions, NULL);
	Array<VkExtensionProperties> available_instance_extensions{num_available_instance_extensions, num_available_instance_extensions};
	VK_DIE_IF_ERROR(vkEnumerateInstanceExtensionProperties, NULL, &num_available_instance_extensions, available_instance_extensions.data);
	debug_print("Querying Vulkan instance extensions:\n%u instance extensions found.\n", num_available_instance_extensions);
	for (const auto& available_extension : available_instance_extensions)
		debug_print("\t%s\n",available_extension.extensionName);
	for (const char *required_extension : instance_extensions) {
		bool found = false;
		for (const auto &available_extension : available_instance_extensions) {
			if (compare_strings(required_extension, available_extension.extensionName) == 0)
				found = true;
		}
		if (!found)
			log_and_abort("Could not find required extension '%s'.", required_extension);
	}

	u32 num_available_instance_layers = 0;
	VK_DIE_IF_ERROR(vkEnumerateInstanceLayerProperties, &num_available_instance_layers, NULL);
	debug_print("Querying Vulkan instance layers:\n%u instance layers found.\n", num_available_instance_layers);
	Array<VkLayerProperties> available_instance_layers{num_available_instance_layers, num_available_instance_layers};
	VK_DIE_IF_ERROR(vkEnumerateInstanceLayerProperties, &num_available_instance_layers, available_instance_layers.data);
	for (const auto& available_layer : available_instance_layers)
		debug_print("\t%s\n", available_layer.layerName);
	for (const char *required_layer : instance_layers) {
		bool found = false;
		for (const auto &available_layer : available_instance_layers) {
			if (compare_strings(required_layer, available_layer.layerName) == 0)
				found = true;
		}
		if (!found)
			log_and_abort("Could not find required layer '%s'.", required_layer);
	}

	VkApplicationInfo app_info = {};
	app_info.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.pApplicationName   = "CGE";
	app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.pEngineName        = "CGE";
	app_info.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
	app_info.apiVersion         = VK_API_VERSION_1_0;

	VkInstanceCreateInfo ci = {};
	ci.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	ci.pNext                   = NULL;
	ci.flags                   = 0;
	ci.pApplicationInfo        = &app_info;
	ci.enabledExtensionCount   = instance_extensions.size;
	ci.enabledLayerCount       = instance_layers.size;
	ci.ppEnabledLayerNames     = instance_layers.data;
	ci.ppEnabledExtensionNames = instance_extensions.data;

	VkInstance instance;
	VK_DIE_IF_ERROR(vkCreateInstance, &ci, NULL, &instance);

	return instance;
}

/*
VkSurfaceKHR
make_surface(VkInstance instance)
{
	// @TODO: Make this platform generic.
	// Type-def the surface creation types and create wrapper calls in the platform layer.
	VkSurfaceKHR surface;
	VkXlibSurfaceCreateInfoKHR ci;
	ci.sType  = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
	ci.pNext  = NULL;
	ci.flags  = 0;
	ci.dpy    = linux_context.display;
	ci.window = linux_context.window;
	VK_DIE_IF_ERROR(vkCreateXlibSurfaceKHR, instance, &ci, NULL, &surface);
	return surface;
}
*/

Physical_Device_Info
pick_physical_device(VkInstance instance, Array<const char *>device_extensions)
{
	Physical_Device_Info info = {};

	// @TODO: Make this platform generic.
	// Type-def the surface creation types and create wrapper calls in the platform layer.
	VkSurfaceKHR surface;
	VkXlibSurfaceCreateInfoKHR ci;
	ci.sType  = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
	ci.pNext  = NULL;
	ci.flags  = 0;
	ci.dpy    = linux_context.display;
	ci.window = linux_context.window;
	VK_DIE_IF_ERROR(vkCreateXlibSurfaceKHR, instance, &ci, NULL, &surface);
	//return surface;

	u32 num_physical_devices = 0;
	VK_DIE_IF_ERROR(vkEnumeratePhysicalDevices, instance, &num_physical_devices, NULL);
	if (num_physical_devices == 0)
		log_and_abort("Could not find any physical devices.");
	debug_print("Found %u available physical devices.\n", num_physical_devices);
	Array<VkPhysicalDevice> available_physical_devices(num_physical_devices, num_physical_devices);
	VK_DIE_IF_ERROR(vkEnumeratePhysicalDevices, instance, &num_physical_devices, available_physical_devices.data);
	u32 query_count = 0;

	for (auto available_physical_device : available_physical_devices) {
		debug_print("Querying physical device %u.\n", query_count++);

		VkPhysicalDeviceProperties physical_device_properties;
		vkGetPhysicalDeviceProperties(available_physical_device, &physical_device_properties);
		debug_print("\tDevice type = %s\n", vk_physical_device_type_to_string(physical_device_properties.deviceType));
		if (physical_device_properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
			debug_print("Skipping physical device because it is not a discrete gpu.\n");
			continue;
		}
		VkPhysicalDeviceFeatures physical_device_features;
		vkGetPhysicalDeviceFeatures(available_physical_device, &physical_device_features);
		debug_print("\tHas geometry shader = %d\n", physical_device_features.geometryShader);
		if (!physical_device_features.geometryShader) {
			debug_print("Skipping physical device because it does not have a geometry shader.\n");
			continue;
		}

		u32 num_available_device_extensions = 0;
		VK_DIE_IF_ERROR(vkEnumerateDeviceExtensionProperties, available_physical_device, NULL, &num_available_device_extensions, NULL);
		debug_print("\tNumber of available device extensions: %u\n", num_available_device_extensions);
		Array<VkExtensionProperties> available_device_extensions{num_available_device_extensions, num_available_device_extensions};
		VK_DIE_IF_ERROR(vkEnumerateDeviceExtensionProperties, available_physical_device, NULL, &num_available_device_extensions, available_device_extensions.data);
		bool missing_required_device_extension = false;
		for (const auto &required_ext : device_extensions) {
			bool found = false;
			for (const auto &available_ext : available_device_extensions) {
				debug_print("\t\t%s\n", available_ext.extensionName);
				if (compare_strings(available_ext.extensionName, required_ext) == 0)
					found = true;
			}
			if (!found) {
				debug_print("\tCould not find required device extension '%s'.\n", required_ext);
				missing_required_device_extension = true;
				break;
			}
		}
		if (missing_required_device_extension)
			continue;

		// Make sure the swap chain is compatible with our window surface.
		// If we have at least one supported surface format and present mode, we will consider the device.
		// @TODO: Should we die if error, or just skip this physical device?
		u32 num_available_surface_formats = 0;
		VK_DIE_IF_ERROR(vkGetPhysicalDeviceSurfaceFormatsKHR, available_physical_device, surface, &num_available_surface_formats, NULL);
		u32 num_available_present_modes = 0;
		VK_DIE_IF_ERROR(vkGetPhysicalDeviceSurfacePresentModesKHR, available_physical_device, surface, &num_available_present_modes, NULL);
		if (num_available_surface_formats == 0 || num_available_present_modes == 0) {
			debug_print("\tSkipping physical device because no supported format or present mode for this surface.\n\t\num_available_surface_formats = %u\n\t\num_available_present_modes = %u\n", num_available_surface_formats, num_available_present_modes);
			continue;
		}
		debug_print("\tPhysical device is compatible with surface.\n");

		// Select the best swap chain settings.
		Array<VkSurfaceFormatKHR> available_surface_formats{num_available_surface_formats};
		VK_DIE_IF_ERROR(vkGetPhysicalDeviceSurfaceFormatsKHR, available_physical_device, surface, &num_available_surface_formats, available_surface_formats.data);
		assert(num_available_surface_formats > 0);
		VkSurfaceFormatKHR surface_format =  available_surface_formats[0];
		debug_print("\tAvailable surface formats (VkFormat, VkColorSpaceKHR):\n");
		if (num_available_surface_formats == 1 && available_surface_formats[0].format == VK_FORMAT_UNDEFINED) {
			surface_format = {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
			debug_print("\t\tNo preferred format.\n");
		} else {
			for (auto asf : available_surface_formats) {
				if (asf.format == VK_FORMAT_B8G8R8A8_UNORM && asf.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
					surface_format = asf;
				}
				printf("\t\t%d, %d\n", asf.format, asf.colorSpace);
			}
		}

		VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
		Array<VkPresentModeKHR> available_present_modes{num_available_present_modes, num_available_present_modes};
		VK_DIE_IF_ERROR(vkGetPhysicalDeviceSurfacePresentModesKHR, available_physical_device, surface, &num_available_present_modes, available_present_modes.data);
		debug_print("\tAvailable present modes:\n");
		for (auto apm : available_present_modes) {
			if (apm == VK_PRESENT_MODE_MAILBOX_KHR)
				present_mode = apm;
			debug_print("\t\t%d\n", apm);
		}

		//VkSurfaceCapabilitiesKHR surface_capabilities;
		//VK_DIE_IF_ERROR(vkGetPhysicalDeviceSurfaceCapabilitiesKHR, available_physical_device, surface, &surface_capabilities);
/*
		VkExtent2D swapchain_extent;
		if (surface_capabilities.currentExtent.width != UINT32_MAX) {
			swapchain_extent = surface_capabilities.currentExtent;
		} else {
			//u32 window_width = 1920, window_height = 1080; // @TODO: Use real values.
			swapchain_extent.width = std::max(surface_capabilities.minImageExtent.width, std::min(surface_capabilities.maxImageExtent.width, window_dimensions.width));
			swapchain_extent.height = std::max(surface_capabilities.minImageExtent.height, std::min(surface_capabilities.maxImageExtent.height, window_dimensions.height));
		}
		debug_print("\tSwap extent %u x %u\n", swapchain_extent.width, swapchain_extent.height);

		u32 num_requested_swap_images = surface_capabilities.minImageCount + 1;
		if (surface_capabilities.maxImageCount > 0 && num_requested_swap_images > surface_capabilities.maxImageCount) {
			num_requested_swap_images = surface_capabilities.maxImageCount;
		}
		debug_print("\tMax swap images: %u\n\tMin swap images: %u\n", surface_capabilities.maxImageCount, surface_capabilities.minImageCount);
*/

		uint32_t num_queue_families = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(available_physical_device, &num_queue_families, NULL);
		Array<VkQueueFamilyProperties> queue_families(num_queue_families, num_queue_families);
		vkGetPhysicalDeviceQueueFamilyProperties(available_physical_device, &num_queue_families, queue_families.data);

		debug_print("\tFound %u queue families.\n", num_queue_families);

		s32 graphics_queue_family = -1;
		s32 present_queue_family = -1;
		for (size_t i = 0; i < queue_families.size; ++i) {
			if (queue_families[i].queueCount > 0) {
				if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
					graphics_queue_family = i;
				VkBool32 present_support = false;
				VK_DIE_IF_ERROR(vkGetPhysicalDeviceSurfaceSupportKHR, available_physical_device, i, surface, &present_support);
				if (present_support)
					present_queue_family = i;
				if (graphics_queue_family != -1 && present_queue_family != -1) {
					info.physical_device = available_physical_device;
					//info.surface_capabilities = surface_capabilities;
					info.graphics_queue_family = graphics_queue_family;
					info.present_queue_family = present_queue_family;
					//info.num_available_present_modes = num_available_present_modes;
					//info.num_available_surface_formats = num_available_surface_formats;
					info.surface_format = surface_format;
					info.surface = surface;
					info.present_mode = present_mode;
					//info.swapchain_extent = swapchain_extent;
					//info.num_requested_swapchain_images = num_requested_swap_images;

					assert(physical_device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && physical_device_features.geometryShader);
					debug_print("\nSelected physical device %d.\n", query_count);
					debug_print("\tVkFormat: %d\n\tVkColorSpaceKHR %d\n", info.surface_format.format, info.surface_format.colorSpace);
					debug_print("\tVkPresentModeKHR %d\n", info.present_mode);

					return info;
				}
			}
		}
	}
	log_and_abort("Could not find suitable physical device.\n");
	return info;
}

VkDevice
make_logical_device(Physical_Device_Info pd, Array<const char *>device_extensions, Array<const char *>instance_layers)
{
	u32 num_unique_queue_families = 1;
	if (pd.graphics_queue_family != pd.present_queue_family)
		num_unique_queue_families = 2;

	Array<VkDeviceQueueCreateInfo> dqci{num_unique_queue_families};
	dqci[0].sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	dqci[0].queueFamilyIndex = pd.graphics_queue_family;
	dqci[0].queueCount       = 1;
	float queue_priority = 1.0f;
	dqci[0].pQueuePriorities = &queue_priority;
	if (pd.present_queue_family != pd.graphics_queue_family) {
		dqci[1].sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		dqci[1].queueFamilyIndex = pd.present_queue_family;
		dqci[1].queueCount       = 1;
		dqci[1].pQueuePriorities = &queue_priority;
	}

	VkPhysicalDeviceFeatures physical_device_features = {};
	VkDeviceCreateInfo dci = {};
	dci.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	dci.pQueueCreateInfos       = dqci.data;
	dci.queueCreateInfoCount    = dqci.size;
	dci.pEnabledFeatures        = &physical_device_features;
	dci.enabledExtensionCount   = device_extensions.size;
	dci.ppEnabledExtensionNames = device_extensions.data;
	dci.enabledLayerCount       = instance_layers.size;
	dci.ppEnabledLayerNames     = instance_layers.data;

	VkDevice d;
	VK_DIE_IF_ERROR(vkCreateDevice, pd.physical_device, &dci, NULL, &d);

	return d;
}
#endif

struct Vertex {
	V2 position;
	V3 color;
};

const Vertex vertices[] = {
	{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
	{{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
	{{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
	{{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
};

const u16 indices[] = {
    0, 1, 2, 2, 3, 0
};

Swapchain_Context
make_swapchain(VkPhysicalDevice physical_device, VkDevice device, VkSurfaceKHR surface, VkSurfaceFormatKHR surface_format, VkPresentModeKHR present_mode, s32 graphics_queue_family, s32 present_queue_family)
{
	Swapchain_Context sc;

	//u32 num_available_surface_formats = 0;
	//VK_DIE_IF_ERROR(vkGetPhysicalDeviceSurfaceFormatsKHR, pdi.physical_device, surface, &num_available_surface_formats, NULL);

/*
	assert(pdi.num_available_surface_formats > 0);
	Array<VkSurfaceFormatKHR> available_surface_formats{pdi.num_available_surface_formats};
	VK_DIE_IF_ERROR(vkGetPhysicalDeviceSurfaceFormatsKHR, pdi.physical_device, surface, &pdi.num_available_surface_formats, available_surface_formats.data);

	si.surface_format =  available_surface_formats[0];
	debug_print("\tAvailable surface formats (VkFormat, VkColorSpaceKHR):\n");
	if (num_available_surface_formats == 1 && available_surface_formats[0].format == VK_FORMAT_UNDEFINED) {
		si.surface_format = {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
		debug_print("\t\tNo preferred format.\n");
	} else {
		for (auto asf : available_surface_formats) {
			if (asf.format == VK_FORMAT_B8G8R8A8_UNORM && asf.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				si.surface_format = asf;
			}
			printf("\t\t%d, %d\n", asf.format, asf.colorSpace);
		}
	}

	si.present_mode = VK_PRESENT_MODE_FIFO_KHR;
	Array<VkPresentModeKHR> available_present_modes{pdi.num_available_present_modes};
	VK_DIE_IF_ERROR(vkGetPhysicalDeviceSurfacePresentModesKHR, pdi.physical_device, surface, &pdi.num_available_present_modes, available_present_modes.data);
	debug_print("\tAvailable present modes:\n");
	for (auto apm : available_present_modes) {
		if (apm == VK_PRESENT_MODE_MAILBOX_KHR)
			si.present_mode = apm;
		debug_print("\t\t%d\n", apm);
	}
*/

	VkSurfaceCapabilitiesKHR surface_capabilities;
	VK_DIE_IF_ERROR(vkGetPhysicalDeviceSurfaceCapabilitiesKHR, physical_device, surface, &surface_capabilities);
	if (surface_capabilities.currentExtent.width != UINT32_MAX) {
		sc.extent = surface_capabilities.currentExtent;
	} else {
		sc.extent.width = std::max(surface_capabilities.minImageExtent.width, std::min(surface_capabilities.maxImageExtent.width, window_dimensions.width));
		sc.extent.height = std::max(surface_capabilities.minImageExtent.height, std::min(surface_capabilities.maxImageExtent.height, window_dimensions.height));
	}
	debug_print("\tSwap extent %u x %u\n", sc.extent.width, sc.extent.height);

	u32 num_requested_swapchain_images = surface_capabilities.minImageCount + 1;
	if (surface_capabilities.maxImageCount > 0 && num_requested_swapchain_images > surface_capabilities.maxImageCount) {
		num_requested_swapchain_images = surface_capabilities.maxImageCount;
	}
	debug_print("\tMax swap images: %u\n\tMin swap images: %u\n", surface_capabilities.maxImageCount, surface_capabilities.minImageCount);
	debug_print("\tRequested swap image count: %u\n", num_requested_swapchain_images);

	// Swap chain handle.
	{
		VkSwapchainCreateInfoKHR ci = {};
		ci.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		ci.surface          = surface;
		ci.minImageCount    = num_requested_swapchain_images;
		ci.imageFormat      = surface_format.format;
		ci.imageColorSpace  = surface_format.colorSpace;
		ci.imageExtent      = sc.extent;
		ci.imageArrayLayers = 1;
		ci.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		ci.preTransform     = surface_capabilities.currentTransform;
		ci.compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		ci.presentMode      = present_mode;
		ci.clipped          = VK_TRUE;
		ci.oldSwapchain     = VK_NULL_HANDLE;

		if (graphics_queue_family != present_queue_family) {
			u32 queue_family_indices[] = { (u32)graphics_queue_family, (u32)present_queue_family };
			ci.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
			ci.queueFamilyIndexCount = ARRAY_COUNT(queue_family_indices);
			ci.pQueueFamilyIndices   = queue_family_indices;
		} else {
			ci.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
			ci.queueFamilyIndexCount = 0;
			ci.pQueueFamilyIndices   = NULL;
		}

		VK_DIE_IF_ERROR(vkCreateSwapchainKHR, device, &ci, NULL, &sc.swapchain);
	}

	// Swap images.
	{
		u32 num_created_swapchain_images = 0;
		VK_DIE_IF_ERROR(vkGetSwapchainImagesKHR, device, sc.swapchain, &num_created_swapchain_images, NULL);
		Array<VkImage> swapchain_images{num_created_swapchain_images};
		VK_DIE_IF_ERROR(vkGetSwapchainImagesKHR, device, sc.swapchain, &num_created_swapchain_images, swapchain_images.data);

		debug_print("\tCreated swap image count: %u\n", num_created_swapchain_images);

		//Array<VkImageView> swapchain_image_views{num_created_swap_images};
		sc.image_views.resize(num_created_swapchain_images);
		for (u32 i = 0; i < sc.image_views.size; ++i) {
			VkImageViewCreateInfo ci = {};
			ci.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			ci.image                           = swapchain_images[i];
			ci.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
			ci.format                          = surface_format.format;
			ci.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
			ci.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
			ci.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
			ci.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
			ci.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
			ci.subresourceRange.baseMipLevel   = 0;
			ci.subresourceRange.levelCount     = 1;
			ci.subresourceRange.baseArrayLayer = 0;
			ci.subresourceRange.layerCount     = 1;
			VK_DIE_IF_ERROR(vkCreateImageView, device, &ci, NULL, &sc.image_views[i]);
		}
	}

	// Render pass.
	{
		VkAttachmentDescription color_attachment = {};
		color_attachment.format         = surface_format.format;
		color_attachment.samples        = VK_SAMPLE_COUNT_1_BIT;
		color_attachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
		color_attachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
		color_attachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		color_attachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
		color_attachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference color_attachment_ref = {};
		color_attachment_ref.attachment = 0;
		color_attachment_ref.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments    = &color_attachment_ref;

		VkRenderPassCreateInfo render_pass_info = {};
		render_pass_info.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		render_pass_info.attachmentCount = 1;
		render_pass_info.pAttachments    = &color_attachment;
		render_pass_info.subpassCount    = 1;
		render_pass_info.pSubpasses      = &subpass;

		VK_DIE_IF_ERROR(vkCreateRenderPass, device, &render_pass_info, NULL, &sc.render_pass);
	}

	// Pipeline.
	{
		Memory_Arena load_shaders = mem_make_arena();
		String vert_spirv = read_entire_file("../build/vert.spirv", &load_shaders);
		assert(vert_spirv != STRING_ERROR);
		String frag_spirv = read_entire_file("../build/frag.spirv", &load_shaders);
		assert(frag_spirv != STRING_ERROR);

		auto make_shader_module = [](VkDevice d, String binary)
		{
			VkShaderModuleCreateInfo create_info = {};
			create_info.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			create_info.codeSize = binary.length;
			create_info.pCode    = (uint32_t *)binary.data;
			VkShaderModule sm;
			VK_DIE_IF_ERROR(vkCreateShaderModule, d, &create_info, NULL, &sm);
			return sm;
		};
		sc.vert_shader_module = make_shader_module(device, vert_spirv);
		sc.frag_shader_module = make_shader_module(device, frag_spirv);

		DEFER(vkDestroyShaderModule(device, sc.vert_shader_module, NULL));
		DEFER(vkDestroyShaderModule(device, sc.frag_shader_module, NULL));

		VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = sc.vert_shader_module;
		vertShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = sc.frag_shader_module;
		fragShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo pipeline_shader_stages[] = {vertShaderStageInfo, fragShaderStageInfo};

		// Vertex input description.
		VkVertexInputBindingDescription bindingDescription = {};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		VkVertexInputAttributeDescription attributeDescriptions[2];
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, position);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, color);

		VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
		vertexInputInfo.vertexAttributeDescriptionCount = ARRAY_COUNT(attributeDescriptions);
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions;

		VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		VkViewport viewport = {};
		viewport.x         =  0.0f;
		viewport.y         =  0.0f;
		viewport.width     =  (float)sc.extent.width;
		viewport.height    =  (float)sc.extent.height;
		viewport.minDepth  =  0.0f;
		viewport.maxDepth  =  1.0f;

		VkRect2D scissor = {};
		scissor.offset = {0, 0};
		scissor.extent = sc.extent;

		VkPipelineViewportStateCreateInfo viewportState = {};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount  =  1;
		viewportState.pViewports     =  &viewport;
		viewportState.scissorCount   =  1;
		viewportState.pScissors      =  &scissor;

		VkPipelineRasterizationStateCreateInfo rasterizer = {};
		rasterizer.sType                    =  VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable         =  VK_FALSE;
		rasterizer.rasterizerDiscardEnable  =  VK_FALSE;
		rasterizer.polygonMode              =  VK_POLYGON_MODE_FILL;
		rasterizer.lineWidth                =  1.0f;
		//rasterizer.cullMode                 =  VK_CULL_MODE_BACK_BIT;
		//rasterizer.frontFace                =  VK_FRONT_FACE_CLOCKWISE;
		rasterizer.cullMode                 =  VK_CULL_MODE_BACK_BIT;
		rasterizer.frontFace                =  VK_FRONT_FACE_COUNTER_CLOCKWISE;

		rasterizer.depthBiasEnable = VK_FALSE;
		rasterizer.depthBiasConstantFactor  =  0.0f;
		rasterizer.depthBiasClamp           =  0.0f;
		rasterizer.depthBiasSlopeFactor     =  0.0f;

		VkPipelineMultisampleStateCreateInfo multisampling = {};
		multisampling.sType                  =  VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable    =  VK_FALSE;
		multisampling.rasterizationSamples   =  VK_SAMPLE_COUNT_1_BIT;
		multisampling.minSampleShading       =  1.0f;
		multisampling.pSampleMask            =  NULL;
		multisampling.alphaToCoverageEnable  =  VK_FALSE;
		multisampling.alphaToOneEnable       =  VK_FALSE;

		// Per frame buffer color blending settings.
		VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
		colorBlendAttachment.colorWriteMask       =  VK_COLOR_COMPONENT_R_BIT  |  VK_COLOR_COMPONENT_G_BIT  |  VK_COLOR_COMPONENT_B_BIT  |  VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable          =  VK_FALSE;
		colorBlendAttachment.srcColorBlendFactor  =  VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstColorBlendFactor  =  VK_BLEND_FACTOR_ZERO;
		colorBlendAttachment.colorBlendOp         =  VK_BLEND_OP_ADD;
		colorBlendAttachment.srcAlphaBlendFactor  =  VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstAlphaBlendFactor  =  VK_BLEND_FACTOR_ZERO;
		colorBlendAttachment.alphaBlendOp         =  VK_BLEND_OP_ADD;

		// Global color blending settings.
		VkPipelineColorBlendStateCreateInfo colorBlending = {};
		colorBlending.sType              =  VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable      =  VK_FALSE;
		colorBlending.logicOp            =  VK_LOGIC_OP_COPY;
		colorBlending.attachmentCount    =  1;
		colorBlending.pAttachments       =  &colorBlendAttachment;
		colorBlending.blendConstants[0]  =  0.0f;
		colorBlending.blendConstants[1]  =  0.0f;
		colorBlending.blendConstants[2]  =  0.0f;
		colorBlending.blendConstants[3]  =  0.0f;

		// Use pipeline layout to specify uniform layout.
		VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
		pipelineLayoutInfo.sType                   =  VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount          =  1;
		pipelineLayoutInfo.pSetLayouts             =  &vk_context.descriptor_set_layout;
		pipelineLayoutInfo.pushConstantRangeCount  =  0;
		pipelineLayoutInfo.pPushConstantRanges     =  NULL;

		VK_DIE_IF_ERROR(vkCreatePipelineLayout, device, &pipelineLayoutInfo, nullptr, &sc.pipeline_layout);

		VkGraphicsPipelineCreateInfo pipelineInfo = {};
		pipelineInfo.sType                =  VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount           =  2;
		pipelineInfo.pStages              =  pipeline_shader_stages;
		pipelineInfo.pVertexInputState    =  &vertexInputInfo;
		pipelineInfo.pInputAssemblyState  =  &inputAssembly;
		pipelineInfo.pViewportState       =  &viewportState;
		pipelineInfo.pRasterizationState  =  &rasterizer;
		pipelineInfo.pMultisampleState    =  &multisampling;
		pipelineInfo.pDepthStencilState   =  NULL;
		pipelineInfo.pColorBlendState     =  &colorBlending;
		pipelineInfo.pDynamicState        =  NULL;
		pipelineInfo.layout               =  sc.pipeline_layout;
		pipelineInfo.renderPass           =  sc.render_pass;
		pipelineInfo.subpass              =  0;
		pipelineInfo.basePipelineHandle   =  VK_NULL_HANDLE;
		pipelineInfo.basePipelineIndex    =  -1;
		pipelineInfo.basePipelineHandle   =  VK_NULL_HANDLE;
		pipelineInfo.basePipelineIndex    =  -1;

		VK_DIE_IF_ERROR(vkCreateGraphicsPipelines, device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &sc.pipeline);
	}

	// Framebuffers.
	{
		sc.framebuffers.resize(sc.image_views.size);
		for (size_t i = 0; i < sc.image_views.size; ++i) {
			VkImageView attachments[] = {sc.image_views[i]};

			VkFramebufferCreateInfo ci = {};
			ci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			ci.renderPass = sc.render_pass;
			ci.attachmentCount = 1;
			ci.pAttachments = attachments;
			ci.width = sc.extent.width;
			ci.height = sc.extent.height;
			ci.layers = 1;

			VK_DIE_IF_ERROR(vkCreateFramebuffer, device, &ci, NULL, &sc.framebuffers[i]);
		}
	}

	//return framebuffers;
	//return gp;
	//return render_pass;
	//return swapchain_image_views;
	return sc;
}

#if 0
Array<VkImageView>
make_image_views(Physical_Device_Info pd, VkDevice device, Swapchain_Info si)
{
	u32 num_created_swap_images = 0;
	VK_DIE_IF_ERROR(vkGetSwapchainImagesKHR, device, si.swapchain, &num_created_swap_images, NULL);
	Array<VkImage> swapchain_images{num_created_swap_images};
	VK_DIE_IF_ERROR(vkGetSwapchainImagesKHR, device, si.swapchain, &num_created_swap_images, swapchain_images.data);

	debug_print("\tCreated swap image count: %u\n", num_created_swap_images);

	Array<VkImageView> swapchain_image_views{num_created_swap_images};
	for (u32 i = 0; i < swapchain_image_views.size; ++i) {
		VkImageViewCreateInfo ci = {};
		ci.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		ci.image                           = swapchain_images[i];
		ci.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
		ci.format                          = pd.surface_format.format;
		ci.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
		ci.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
		ci.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
		ci.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
		ci.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
		ci.subresourceRange.baseMipLevel   = 0;
		ci.subresourceRange.levelCount     = 1;
		ci.subresourceRange.baseArrayLayer = 0;
		ci.subresourceRange.layerCount     = 1;
		VK_DIE_IF_ERROR(vkCreateImageView, device, &ci, NULL, &swapchain_image_views[i]);
	}
	return swapchain_image_views;
}

VkRenderPass
make_render_pass(Physical_Device_Info pd, VkDevice device)
{
	VkAttachmentDescription color_attachment = {};
	color_attachment.format         = pd.surface_format.format;
	color_attachment.samples        = VK_SAMPLE_COUNT_1_BIT;
	color_attachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
	color_attachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
	color_attachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	color_attachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
	color_attachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference color_attachment_ref = {};
	color_attachment_ref.attachment = 0;
	color_attachment_ref.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments    = &color_attachment_ref;

	//VkRenderPass renderPass;
	VkRenderPassCreateInfo render_pass_info = {};
	render_pass_info.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	render_pass_info.attachmentCount = 1;
	render_pass_info.pAttachments    = &color_attachment;
	render_pass_info.subpassCount    = 1;
	render_pass_info.pSubpasses      = &subpass;

	VkRenderPass render_pass;
	VK_DIE_IF_ERROR(vkCreateRenderPass, device, &render_pass_info, NULL, &render_pass);

	return render_pass;
}

Pipeline_Info
make_graphics_pipeline(Swapchain_Info si, VkDevice device, VkRenderPass rp)
{
	Pipeline_Info gp;

	// Compile shaders.
	Memory_Arena load_shaders = mem_make_arena();
	String vert_spirv = read_entire_file("../build/vert.spirv", &load_shaders);
	assert(vert_spirv != STRING_ERROR);
	String frag_spirv = read_entire_file("../build/frag.spirv", &load_shaders);
	assert(frag_spirv != STRING_ERROR);

	auto make_shader_module = [](VkDevice d, String binary)
	{
		VkShaderModuleCreateInfo create_info = {};
		create_info.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		create_info.codeSize = binary.length;
		create_info.pCode    = (uint32_t *)binary.data;
		VkShaderModule sm;
		VK_DIE_IF_ERROR(vkCreateShaderModule, d, &create_info, NULL, &sm);
		return sm;
	};
	gp.vert_shader_module = make_shader_module(device, vert_spirv);
	gp.frag_shader_module = make_shader_module(device, frag_spirv);

	DEFER(vkDestroyShaderModule(device, gp.vert_shader_module, NULL));
	DEFER(vkDestroyShaderModule(device, gp.frag_shader_module, NULL));

	VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = gp.vert_shader_module;
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = gp.frag_shader_module;
	fragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo pipeline_shader_stages[] = {vertShaderStageInfo, fragShaderStageInfo};

	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 0;
	vertexInputInfo.pVertexBindingDescriptions = NULL;
	vertexInputInfo.vertexAttributeDescriptionCount = 0;
	vertexInputInfo.pVertexAttributeDescriptions = NULL;

	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)si.extent.width;
	viewport.height = (float)si.extent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = {0, 0};
	scissor.extent = si.extent;

	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;

	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f;
	rasterizer.depthBiasClamp = 0.0f;
	rasterizer.depthBiasSlopeFactor = 0.0f;

	VkPipelineMultisampleStateCreateInfo multisampling = {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f;
	multisampling.pSampleMask = nullptr;
	multisampling.alphaToCoverageEnable = VK_FALSE;
	multisampling.alphaToOneEnable = VK_FALSE;

	// Per frame buffer color blending settings.
	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	// Global color blending settings.
	VkPipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f;
	colorBlending.blendConstants[1] = 0.0f;
	colorBlending.blendConstants[2] = 0.0f;
	colorBlending.blendConstants[3] = 0.0f;

	// Use pipeline layout to specify uniform layout.
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 0;
	pipelineLayoutInfo.pSetLayouts = nullptr;
	pipelineLayoutInfo.pushConstantRangeCount = 0;
	pipelineLayoutInfo.pPushConstantRanges = nullptr;

	VK_DIE_IF_ERROR(vkCreatePipelineLayout, device, &pipelineLayoutInfo, nullptr, &gp.layout);

	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = pipeline_shader_stages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = NULL;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = NULL;
	pipelineInfo.layout = gp.layout;
	pipelineInfo.renderPass = rp;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex = -1;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex = -1;

	//VkPipeline graphicsPipeline;
	VK_DIE_IF_ERROR(vkCreateGraphicsPipelines, device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &gp.pipeline);

	return gp;
}

Array<VkFramebuffer>
make_framebuffers(Swapchain_Info si, VkDevice device, Array<VkImageView> image_views, VkRenderPass render_pass)
{
	Array<VkFramebuffer> framebuffers{image_views.size};
	{
		for (size_t i = 0; i < image_views.size; ++i) {
			VkImageView attachments[] = {image_views[i]};

			VkFramebufferCreateInfo ci = {};
			ci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			ci.renderPass = render_pass;
			ci.attachmentCount = 1;
			ci.pAttachments = attachments;
			ci.width = si.extent.width;
			ci.height = si.extent.height;
			ci.layers = 1;

			VK_DIE_IF_ERROR(vkCreateFramebuffer, device, &ci, NULL, &framebuffers[i]);
		}
	}
	return framebuffers;
}

VkCommandPool
make_command_pool(Physical_Device_Info pd, VkDevice device)
{
	VkCommandPool cp;

	VkCommandPoolCreateInfo ci = {};
	ci.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	ci.queueFamilyIndex = pd.graphics_queue_family;
	ci.flags            = 0;
	VK_DIE_IF_ERROR(vkCreateCommandPool, device, &ci, NULL, &cp);

	return cp;
}
#endif

Array<VkCommandBuffer>
make_command_buffers(Swapchain_Context sc, VkDevice device, VkCommandPool cp)
{
	Array<VkCommandBuffer> command_buffers{sc.framebuffers.size};

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = cp;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t)command_buffers.size;
	VK_DIE_IF_ERROR(vkAllocateCommandBuffers, device, &allocInfo, command_buffers.data);

	// Fill command buffers.
	for (size_t i = 0; i < command_buffers.size; ++i) {
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags            = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
		beginInfo.pInheritanceInfo = NULL;
		vkBeginCommandBuffer(command_buffers[i], &beginInfo);

		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass        = sc.render_pass;
		renderPassInfo.framebuffer       = sc.framebuffers[i];
		renderPassInfo.renderArea.offset = {0, 0};
		renderPassInfo.renderArea.extent = sc.extent;
		VkClearValue clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;
		vkCmdBeginRenderPass(command_buffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, sc.pipeline);

		VkBuffer vertexBuffers[] = {vk_context.vertex_buffer};
		VkDeviceSize offsets[] = {0};
		vkCmdBindVertexBuffers(command_buffers[i], 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(command_buffers[i], vk_context.index_buffer, 0, VK_INDEX_TYPE_UINT16);
		vkCmdBindDescriptorSets(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, vk_context.swapchain_context.pipeline_layout, 0, 1, &vk_context.descriptor_set, 0, NULL);

		//vkCmdDraw(command_buffers[i], ARRAY_COUNT(vertices), 1, 0, 0);
		vkCmdDrawIndexed(command_buffers[i], ARRAY_COUNT(indices), 1, 0, 0, 0);

		vkCmdEndRenderPass(command_buffers[i]);

		VK_DIE_IF_ERROR(vkEndCommandBuffer, command_buffers[i]);
	}
	return command_buffers;
}

#if 0
Semaphores
make_semaphores(VkDevice device)
{
	Semaphores s;
	VkSemaphoreCreateInfo ci = {};
	ci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	VK_DIE_IF_ERROR(vkCreateSemaphore, device, &ci, NULL, &s.image_available);
	VK_DIE_IF_ERROR(vkCreateSemaphore, device, &ci, NULL, &s.render_finished);
	return s;
}

Queues
make_queues(VkDevice device, Physical_Device_Info pdi)
{
	Queues q = {};
	vkGetDeviceQueue(device, pdi.graphics_queue_family, 0, &q.graphics);
	vkGetDeviceQueue(device, pdi.present_queue_family, 0, &q.present);
	return q;
}

VkDebugReportCallbackEXT
make_debug_callback()
{
	VkDebugReportCallbackEXT drc;
	VkDebugReportCallbackCreateInfoEXT ci = {};
	ci.sType       = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
	ci.flags       = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
	ci.pfnCallback = vk_debug_callback;
	auto vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(vk_context.instance, "vkCreateDebugReportCallbackEXT");
	if (!vkCreateDebugReportCallbackEXT)
		log_and_abort("Could not load vkCreateDebugReportCallbackEXT function.");
	VK_DIE_IF_ERROR(vkCreateDebugReportCallbackEXT, vk_context.instance, &ci, NULL, &drc);
	return drc;
}
#endif

struct Vk_Buffer_Info {
	VkBuffer buffer;
	VkDeviceMemory buffer_memory;
};

// @TODO: Create bigger memory allocations and batch out buffers from it so we don't run into the physical device limit maxMemoryAllocationCount.
// @TODO: Also use a single buffer handle for all of our buffers.
Vk_Buffer_Info
make_buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
{
	VkBuffer buffer;

	VkBufferCreateInfo ci = {};
	ci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	ci.size = size;
	ci.usage = usage;
	ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	VK_DIE_IF_ERROR(vkCreateBuffer, vk_context.device, &ci, NULL, &buffer);

	VkMemoryRequirements mem_requirements;
	vkGetBufferMemoryRequirements(vk_context.device, buffer, &mem_requirements);

	VkMemoryAllocateInfo ai = {};
	ai.sType           =  VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	ai.allocationSize  =  mem_requirements.size;

	VkPhysicalDeviceMemoryProperties mem_properties;
	vkGetPhysicalDeviceMemoryProperties(vk_context.physical_device, &mem_properties);

	for (u32 i = 0; i < mem_properties.memoryTypeCount; ++i) {
		if ((mem_requirements.memoryTypeBits & (1 << i)) && (mem_properties.memoryTypes[i].propertyFlags & properties) == properties)
			ai.memoryTypeIndex = i;
	}

	VkDeviceMemory buffer_memory;
	VK_DIE_IF_ERROR(vkAllocateMemory, vk_context.device, &ai, NULL, &buffer_memory);
	VK_DIE_IF_ERROR(vkBindBufferMemory, vk_context.device, buffer, buffer_memory, 0);

	return Vk_Buffer_Info{buffer, buffer_memory};
}

void
copy_buffer(VkBuffer source_buffer, VkBuffer dest_buffer, VkDeviceSize size)
{
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType               =  VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level               =  VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool         =  vk_context.command_pool;
	allocInfo.commandBufferCount  =  1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(vk_context.device, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	VkBufferCopy copyRegion = {};
	copyRegion.srcOffset  =  0;
	copyRegion.dstOffset  =  0;
	copyRegion.size       =  size;
	vkCmdCopyBuffer(commandBuffer, source_buffer, dest_buffer, 1, &copyRegion);

	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType               =  VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount  =  1;
	submitInfo.pCommandBuffers     =  &commandBuffer;

	vkQueueSubmit(vk_context.graphics_queue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(vk_context.graphics_queue);
	// @TODO: Use vkWaitForFences for multiple copies?

	vkFreeCommandBuffers(vk_context.device, vk_context.command_pool, 1, &commandBuffer);
}

struct Uniform_Buffer_Object {
    M4 model;
    M4 view;
    M4 projection;
};

void
render_init()
{
	// @TODO: Create distinction between optional and required layers/extensions.
	// @TODO: Create distinction between release and debug layers/extensions.
	Array<const char *> required_instance_extensions{3, 0};
	required_instance_extensions.push("VK_KHR_surface");
	required_instance_extensions.push("VK_KHR_xlib_surface");
	required_instance_extensions.push("VK_EXT_debug_report");

	Array<const char *> required_instance_layers{1, 0};
	required_instance_layers.push("VK_LAYER_LUNARG_standard_validation");

	Array<const char *> required_device_extensions{1, 0};
	required_device_extensions.push("VK_KHR_swapchain");

	// Instance.
	{
		u32 num_available_instance_extensions = 0;
		VK_DIE_IF_ERROR(vkEnumerateInstanceExtensionProperties, NULL, &num_available_instance_extensions, NULL);
		Array<VkExtensionProperties> available_instance_extensions{num_available_instance_extensions, num_available_instance_extensions};
		VK_DIE_IF_ERROR(vkEnumerateInstanceExtensionProperties, NULL, &num_available_instance_extensions, available_instance_extensions.data);
		debug_print("Querying Vulkan instance extensions:\n%u instance extensions found.\n", num_available_instance_extensions);
		for (const auto& available_extension : available_instance_extensions)
			debug_print("\t%s\n",available_extension.extensionName);
		for (const char *required_extension : required_instance_extensions) {
			bool found = false;
			for (const auto &available_extension : available_instance_extensions) {
				if (compare_strings(required_extension, available_extension.extensionName) == 0)
					found = true;
			}
			if (!found)
				_abort("Could not find required extension '%s'.", required_extension);
		}

		u32 num_available_instance_layers = 0;
		VK_DIE_IF_ERROR(vkEnumerateInstanceLayerProperties, &num_available_instance_layers, NULL);
		debug_print("Querying Vulkan instance layers:\n%u instance layers found.\n", num_available_instance_layers);
		Array<VkLayerProperties> available_instance_layers{num_available_instance_layers, num_available_instance_layers};
		VK_DIE_IF_ERROR(vkEnumerateInstanceLayerProperties, &num_available_instance_layers, available_instance_layers.data);
		for (const auto& available_layer : available_instance_layers)
			debug_print("\t%s\n", available_layer.layerName);
		for (const char *required_layer : required_instance_layers) {
			bool found = false;
			for (const auto &available_layer : available_instance_layers) {
				if (compare_strings(required_layer, available_layer.layerName) == 0)
					found = true;
			}
			if (!found)
				_abort("Could not find required layer '%s'.", required_layer);
		}

		VkApplicationInfo app_info = {};
		app_info.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		app_info.pApplicationName   = "CGE";
		app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		app_info.pEngineName        = "CGE";
		app_info.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
		app_info.apiVersion         = VK_API_VERSION_1_0;

		VkInstanceCreateInfo ci = {};
		ci.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		ci.pNext                   = NULL;
		ci.flags                   = 0;
		ci.pApplicationInfo        = &app_info;
		ci.enabledExtensionCount   = required_instance_extensions.size;
		ci.enabledLayerCount       = required_instance_layers.size;
		ci.ppEnabledLayerNames     = required_instance_layers.data;
		ci.ppEnabledExtensionNames = required_instance_extensions.data;

		VK_DIE_IF_ERROR(vkCreateInstance, &ci, NULL, &vk_context.instance);
	}

	// Debug callback.
	{
		VkDebugReportCallbackCreateInfoEXT ci = {};
		ci.sType       = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
		ci.flags       = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
		ci.pfnCallback = vk_debug_callback;
		auto vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(vk_context.instance, "vkCreateDebugReportCallbackEXT");
		if (!vkCreateDebugReportCallbackEXT)
			_abort("Could not load vkCreateDebugReportCallbackEXT function.");
		VK_DIE_IF_ERROR(vkCreateDebugReportCallbackEXT, vk_context.instance, &ci, NULL, &vk_context.debug_callback);
	}

	// Physical device.
	{
		bool found_suitable_physical_device = false;

		// @TODO: Make this platform generic.
		// Type-def the surface creation types and create wrapper calls in the platform layer.
		VkSurfaceKHR surface;
		VkXlibSurfaceCreateInfoKHR ci;
		ci.sType  = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
		ci.pNext  = NULL;
		ci.flags  = 0;
		ci.dpy    = linux_context.display;
		ci.window = linux_context.window;
		VK_DIE_IF_ERROR(vkCreateXlibSurfaceKHR, vk_context.instance, &ci, NULL, &surface);

		u32 num_physical_devices = 0;
		VK_DIE_IF_ERROR(vkEnumeratePhysicalDevices, vk_context.instance, &num_physical_devices, NULL);
		if (num_physical_devices == 0)
			_abort("Could not find any physical devices.");
		debug_print("Found %u available physical devices.\n", num_physical_devices);
		Array<VkPhysicalDevice> available_physical_devices(num_physical_devices, num_physical_devices);
		VK_DIE_IF_ERROR(vkEnumeratePhysicalDevices, vk_context.instance, &num_physical_devices, available_physical_devices.data);
		u32 query_count = 0;

		for (auto available_physical_device : available_physical_devices) {
			debug_print("Querying physical device %u.\n", query_count++);

			VkPhysicalDeviceProperties physical_device_properties;
			vkGetPhysicalDeviceProperties(available_physical_device, &physical_device_properties);
			debug_print("\tDevice type = %s\n", vk_physical_device_type_to_string(physical_device_properties.deviceType));
			if (physical_device_properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
				debug_print("Skipping physical device because it is not a discrete gpu.\n");
				continue;
			}
			VkPhysicalDeviceFeatures physical_device_features;
			vkGetPhysicalDeviceFeatures(available_physical_device, &physical_device_features);
			debug_print("\tHas geometry shader = %d\n", physical_device_features.geometryShader);
			if (!physical_device_features.geometryShader) {
				debug_print("Skipping physical device because it does not have a geometry shader.\n");
				continue;
			}

			u32 num_available_device_extensions = 0;
			VK_DIE_IF_ERROR(vkEnumerateDeviceExtensionProperties, available_physical_device, NULL, &num_available_device_extensions, NULL);
			debug_print("\tNumber of available device extensions: %u\n", num_available_device_extensions);
			Array<VkExtensionProperties> available_device_extensions{num_available_device_extensions, num_available_device_extensions};
			VK_DIE_IF_ERROR(vkEnumerateDeviceExtensionProperties, available_physical_device, NULL, &num_available_device_extensions, available_device_extensions.data);
			bool missing_required_device_extension = false;
			for (const auto &required_ext : required_device_extensions) {
				bool found = false;
				for (const auto &available_ext : available_device_extensions) {
					debug_print("\t\t%s\n", available_ext.extensionName);
					if (compare_strings(available_ext.extensionName, required_ext) == 0)
						found = true;
				}
				if (!found) {
					debug_print("\tCould not find required device extension '%s'.\n", required_ext);
					missing_required_device_extension = true;
					break;
				}
			}
			if (missing_required_device_extension)
				continue;

			// Make sure the swap chain is compatible with our window surface.
			// If we have at least one supported surface format and present mode, we will consider the device.
			// @TODO: Should we die if error, or just skip this physical device?
			u32 num_available_surface_formats = 0;
			VK_DIE_IF_ERROR(vkGetPhysicalDeviceSurfaceFormatsKHR, available_physical_device, surface, &num_available_surface_formats, NULL);
			u32 num_available_present_modes = 0;
			VK_DIE_IF_ERROR(vkGetPhysicalDeviceSurfacePresentModesKHR, available_physical_device, surface, &num_available_present_modes, NULL);
			if (num_available_surface_formats == 0 || num_available_present_modes == 0) {
				debug_print("\tSkipping physical device because no supported format or present mode for this surface.\n\t\num_available_surface_formats = %u\n\t\num_available_present_modes = %u\n", num_available_surface_formats, num_available_present_modes);
				continue;
			}
			debug_print("\tPhysical device is compatible with surface.\n");

			// Select the best swap chain settings.
			Array<VkSurfaceFormatKHR> available_surface_formats{num_available_surface_formats};
			VK_DIE_IF_ERROR(vkGetPhysicalDeviceSurfaceFormatsKHR, available_physical_device, surface, &num_available_surface_formats, available_surface_formats.data);
			assert(num_available_surface_formats > 0);
			VkSurfaceFormatKHR surface_format =  available_surface_formats[0];
			debug_print("\tAvailable surface formats (VkFormat, VkColorSpaceKHR):\n");
			if (num_available_surface_formats == 1 && available_surface_formats[0].format == VK_FORMAT_UNDEFINED) {
				surface_format = {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
				debug_print("\t\tNo preferred format.\n");
			} else {
				for (auto asf : available_surface_formats) {
					if (asf.format == VK_FORMAT_B8G8R8A8_UNORM && asf.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
						surface_format = asf;
					}
					printf("\t\t%d, %d\n", asf.format, asf.colorSpace);
				}
			}

			VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
			Array<VkPresentModeKHR> available_present_modes{num_available_present_modes, num_available_present_modes};
			VK_DIE_IF_ERROR(vkGetPhysicalDeviceSurfacePresentModesKHR, available_physical_device, surface, &num_available_present_modes, available_present_modes.data);
			debug_print("\tAvailable present modes:\n");
			for (auto apm : available_present_modes) {
				if (apm == VK_PRESENT_MODE_MAILBOX_KHR)
					present_mode = apm;
				debug_print("\t\t%d\n", apm);
			}

			uint32_t num_queue_families = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(available_physical_device, &num_queue_families, NULL);
			Array<VkQueueFamilyProperties> queue_families(num_queue_families, num_queue_families);
			vkGetPhysicalDeviceQueueFamilyProperties(available_physical_device, &num_queue_families, queue_families.data);

			debug_print("\tFound %u queue families.\n", num_queue_families);

			s32 graphics_queue_family = -1;
			s32 present_queue_family = -1;
			for (size_t i = 0; i < queue_families.size; ++i) {
				if (queue_families[i].queueCount > 0) {
					if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
						graphics_queue_family = i;
					VkBool32 present_support = false;
					VK_DIE_IF_ERROR(vkGetPhysicalDeviceSurfaceSupportKHR, available_physical_device, i, surface, &present_support);
					if (present_support)
						present_queue_family = i;
					if (graphics_queue_family != -1 && present_queue_family != -1) {
						vk_context.physical_device = available_physical_device;
						vk_context.graphics_queue_family = graphics_queue_family;
						vk_context.present_queue_family = present_queue_family;
						vk_context.surface_format = surface_format;
						vk_context.surface = surface;
						vk_context.present_mode = present_mode;

						assert(physical_device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && physical_device_features.geometryShader);
						debug_print("\nSelected physical device %d.\n", query_count);
						debug_print("\tVkFormat: %d\n\tVkColorSpaceKHR %d\n", surface_format.format, surface_format.colorSpace);
						debug_print("\tVkPresentModeKHR %d\n", present_mode);

						found_suitable_physical_device = true;
						break;
					}
				}
			}
			if (found_suitable_physical_device)
				break;
		}
		if (!found_suitable_physical_device)
			_abort("Could not find suitable physical device.\n");
	}

	// Logical device.
	{
		u32 num_unique_queue_families = 1;
		if (vk_context.graphics_queue_family != vk_context.present_queue_family)
			num_unique_queue_families = 2;

		Array<VkDeviceQueueCreateInfo> qci{num_unique_queue_families};
		qci[0].sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		qci[0].queueFamilyIndex = vk_context.graphics_queue_family;
		qci[0].queueCount       = 1;
		float queue_priority    = 1.0f;
		qci[0].pQueuePriorities = &queue_priority;
		if (vk_context.present_queue_family != vk_context.graphics_queue_family) {
			qci[1].sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			qci[1].queueFamilyIndex = vk_context.present_queue_family;
			qci[1].queueCount       = 1;
			qci[1].pQueuePriorities = &queue_priority;
		}

		VkPhysicalDeviceFeatures physical_device_features = {};
		VkDeviceCreateInfo dci = {};
		dci.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		dci.pQueueCreateInfos       = qci.data;
		dci.queueCreateInfoCount    = qci.size;
		dci.pEnabledFeatures        = &physical_device_features;
		dci.enabledExtensionCount   = required_device_extensions.size;
		dci.ppEnabledExtensionNames = required_device_extensions.data;
		dci.enabledLayerCount       = required_instance_layers.size;
		dci.ppEnabledLayerNames     = required_instance_layers.data;

		VK_DIE_IF_ERROR(vkCreateDevice, vk_context.physical_device, &dci, NULL, &vk_context.device);
	}

	// Queues.
	{
		vkGetDeviceQueue(vk_context.device, vk_context.graphics_queue_family, 0, &vk_context.graphics_queue);
		vkGetDeviceQueue(vk_context.device, vk_context.present_queue_family, 0, &vk_context.present_queue);
	}

	// Descriptor set layout.
	{
		VkDescriptorSetLayoutBinding uboLayoutBinding = {};
		uboLayoutBinding.binding = 0;
		uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboLayoutBinding.descriptorCount = 1;
		uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		uboLayoutBinding.pImmutableSamplers = NULL;

		VkDescriptorSetLayoutCreateInfo layoutInfo = {};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = 1;
		layoutInfo.pBindings = &uboLayoutBinding;

		VK_DIE_IF_ERROR(vkCreateDescriptorSetLayout, vk_context.device, &layoutInfo, NULL, &vk_context.descriptor_set_layout);
	}

	vk_context.swapchain_context = make_swapchain(vk_context.physical_device, vk_context.device, vk_context.surface, vk_context.surface_format, vk_context.present_mode, vk_context.graphics_queue_family, vk_context.present_queue_family);

	// Command pool.
	{
		VkCommandPoolCreateInfo ci = {};
		ci.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		ci.queueFamilyIndex = vk_context.graphics_queue_family;
		ci.flags            = 0;
		VK_DIE_IF_ERROR(vkCreateCommandPool, vk_context.device, &ci, NULL, &vk_context.command_pool);
	}

	// @TODO: Try to use a seperate queue family for transfer operations so that it can be parallelized.

	// Vertex buffers.
	{
	/*
		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = sizeof(vertices[0]) * ARRAY_COUNT(vertices);
		bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		VK_DIE_IF_ERROR(vkCreateBuffer, vk_context.device, &bufferInfo, NULL, &vk_context.vertex_buffer);

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(vk_context.device, vk_context.vertex_buffer, &memRequirements);

		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		//allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(vk_context.physical_device, &memProperties);
		VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		for (u32 i = 0; i < memProperties.memoryTypeCount; ++i) {
			if ((memRequirements.memoryTypeBits & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
				allocInfo.memoryTypeIndex = i;
		}

		VK_DIE_IF_ERROR(vkAllocateMemory, vk_context.device, &allocInfo, NULL, &vk_context.vertex_buffer_memory);
		VK_DIE_IF_ERROR(vkBindBufferMemory, vk_context.device, vk_context.vertex_buffer, vk_context.vertex_buffer_memory, 0);
*/
		VkDeviceSize buffer_size = sizeof(vertices[0]) * ARRAY_COUNT(vertices);
		auto [ staging_buffer, staging_buffer_memory ] = make_buffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		void* data;
		VK_DIE_IF_ERROR(vkMapMemory, vk_context.device, staging_buffer_memory, 0, buffer_size, 0, &data);
		memcpy(data, vertices, (size_t)buffer_size);
		vkUnmapMemory(vk_context.device, staging_buffer_memory);

		auto res = make_buffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		vk_context.vertex_buffer = res.buffer;
		vk_context.vertex_buffer_memory = res.buffer_memory;

		copy_buffer(staging_buffer, vk_context.vertex_buffer, buffer_size);

		vkDestroyBuffer(vk_context.device, staging_buffer, NULL);
		vkFreeMemory(vk_context.device, staging_buffer_memory, NULL);
	}

	// Index buffers.
	{
		VkDeviceSize buffer_size = sizeof(indices[0]) * ARRAY_COUNT(indices);
		auto [ staging_buffer, staging_buffer_memory ] = make_buffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		void* data;
		VK_DIE_IF_ERROR(vkMapMemory, vk_context.device, staging_buffer_memory, 0, buffer_size, 0, &data);
		memcpy(data, indices, (size_t)buffer_size);
		vkUnmapMemory(vk_context.device, staging_buffer_memory);

		auto res = make_buffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		vk_context.index_buffer = res.buffer;
		vk_context.index_buffer_memory = res.buffer_memory;

		copy_buffer(staging_buffer, vk_context.index_buffer, buffer_size);

		vkDestroyBuffer(vk_context.device, staging_buffer, NULL);
		vkFreeMemory(vk_context.device, staging_buffer_memory, NULL);
	}

	// Uniform buffers
	{
		VkDeviceSize buffer_size = sizeof(Uniform_Buffer_Object);
		auto res = make_buffer(buffer_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		vk_context.uniform_buffer = res.buffer;
		vk_context.uniform_buffer_memory = res.buffer_memory;
	}

	// Descriptor pool.
	{
		VkDescriptorPoolSize pool_size = {};
		pool_size.type             =  VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		pool_size.descriptorCount  =  1;

		VkDescriptorPoolCreateInfo pool_info = {};
		pool_info.sType          =  VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		pool_info.poolSizeCount  =  1;
		pool_info.pPoolSizes     =  &pool_size;
		pool_info.maxSets        =  1;

		VK_DIE_IF_ERROR(vkCreateDescriptorPool, vk_context.device, &pool_info, NULL, &vk_context.descriptor_pool);
	}

	// Descriptor set.
	{
		VkDescriptorSetLayout layouts[] = {vk_context.descriptor_set_layout};
		VkDescriptorSetAllocateInfo ai = {};
		ai.sType               =  VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		ai.descriptorPool      =  vk_context.descriptor_pool;
		ai.descriptorSetCount  =  1;
		ai.pSetLayouts         =  layouts;
		VK_DIE_IF_ERROR(vkAllocateDescriptorSets, vk_context.device, &ai, &vk_context.descriptor_set);

		VkDescriptorBufferInfo bufferInfo = {};
		bufferInfo.buffer  =  vk_context.uniform_buffer;
		bufferInfo.offset  =  0;
		bufferInfo.range   =  sizeof(Uniform_Buffer_Object);

		VkWriteDescriptorSet descriptorWrite = {};
		descriptorWrite.sType            =  VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet           =  vk_context.descriptor_set;
		descriptorWrite.dstBinding       =  0;
		descriptorWrite.dstArrayElement  =  0;
		descriptorWrite.descriptorType   =  VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrite.descriptorCount  =  1;
		descriptorWrite.pBufferInfo = &bufferInfo;
		descriptorWrite.pImageInfo = NULL;
		descriptorWrite.pTexelBufferView = NULL;

		vkUpdateDescriptorSets(vk_context.device, 1, &descriptorWrite, 0, NULL);
	}

	vk_context.command_buffers = make_command_buffers(vk_context.swapchain_context, vk_context.device, vk_context.command_pool);

	// Semaphore.
	{
		VkSemaphoreCreateInfo ci = {};
		ci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		VK_DIE_IF_ERROR(vkCreateSemaphore, vk_context.device, &ci, NULL, &vk_context.image_available_semaphore);
		VK_DIE_IF_ERROR(vkCreateSemaphore, vk_context.device, &ci, NULL, &vk_context.render_finished_semaphore);
	}
}

void
render_resize_window()
{
	debug_print("Vulkan window resize: recreating swap chain and command buffers...\n");

	vkDeviceWaitIdle(vk_context.device);

	// Cleanup old resources.
	for (auto fb : vk_context.swapchain_context.framebuffers)
		vkDestroyFramebuffer(vk_context.device, fb, NULL);
	vkFreeCommandBuffers(vk_context.device, vk_context.command_pool, vk_context.command_buffers.size, vk_context.command_buffers.data);
	vkDestroyPipeline(vk_context.device, vk_context.swapchain_context.pipeline, NULL);
	vkDestroyPipelineLayout(vk_context.device, vk_context.swapchain_context.pipeline_layout, NULL);
	vkDestroyRenderPass(vk_context.device, vk_context.swapchain_context.render_pass, NULL);
	for (auto iv : vk_context.swapchain_context.image_views)
		vkDestroyImageView(vk_context.device, iv, NULL);
	vkDestroySwapchainKHR(vk_context.device, vk_context.swapchain_context.swapchain, NULL);

	vk_context.swapchain_context       =  make_swapchain(vk_context.physical_device, vk_context.device, vk_context.surface, vk_context.surface_format, vk_context.present_mode, vk_context.graphics_queue_family, vk_context.present_queue_family);
	vk_context.command_buffers         =  make_command_buffers(vk_context.swapchain_context, vk_context.device, vk_context.command_pool);
}

void
render()
{
	Uniform_Buffer_Object ubo = {};
	//ubo.model = make_m4();
	ubo.view = look_at({2.0f, 2.0f, 2.0f}, {0.0f, 0.0f, 0.0f});
	float aspect_ratio = (float)vk_context.swapchain_context.extent.width / (float)vk_context.swapchain_context.extent.height;
	ubo.projection = perspective_matrix(45.0f, aspect_ratio, 0.1f, 10.0f);
	ubo.projection[1][1] *= -1;

	void* data;
	vkMapMemory(vk_context.device, vk_context.uniform_buffer_memory, 0, sizeof(ubo), 0, &data);
	memcpy(data, &ubo, sizeof(ubo));
	vkUnmapMemory(vk_context.device, vk_context.uniform_buffer_memory);





	// Wait for the device to be idle. Only necessary when using validation layers.
	vkDeviceWaitIdle(vk_context.device);

	uint32_t imageIndex;
	vkAcquireNextImageKHR(vk_context.device, vk_context.swapchain_context.swapchain, UINT64_MAX, vk_context.image_available_semaphore, VK_NULL_HANDLE, &imageIndex);
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = {vk_context.image_available_semaphore};
	VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT};
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &vk_context.command_buffers[imageIndex];
	VkSemaphore signalSemaphores[] = {vk_context.render_finished_semaphore};
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;
	VK_DIE_IF_ERROR(vkQueueSubmit, vk_context.graphics_queue, 1, &submitInfo, VK_NULL_HANDLE);

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = {vk_context.swapchain_context.swapchain};
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = NULL;
	vkQueuePresentKHR(vk_context.present_queue, &presentInfo);
}

void
render_cleanup()
{
	// Make sure vulkan is done.
	vkDeviceWaitIdle(vk_context.device);

	VkDevice d = vk_context.device;

	auto vkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(vk_context.instance, "vkDestroyDebugReportCallbackEXT");
	if(!vkDestroyDebugReportCallbackEXT)
		_abort("Could not load vkDestroyDebugReportCallbackEXT function.");
	vkDestroyDebugReportCallbackEXT(vk_context.instance, vk_context.debug_callback, NULL);
	vkDestroySemaphore(d, vk_context.render_finished_semaphore, NULL);
	vkDestroySemaphore(d, vk_context.image_available_semaphore, NULL);
	vkDestroyCommandPool(d, vk_context.command_pool, NULL);
	for (auto fb : vk_context.swapchain_context.framebuffers)
		vkDestroyFramebuffer(d, fb, NULL);
	vkDestroyPipeline(d, vk_context.swapchain_context.pipeline, NULL);
	vkDestroyPipelineLayout(d, vk_context.swapchain_context.pipeline_layout, NULL);
	vkDestroyRenderPass(d, vk_context.swapchain_context.render_pass, NULL);
	for (auto iv : vk_context.swapchain_context.image_views)
		vkDestroyImageView(d, iv, NULL);
	vkDestroySwapchainKHR(d, vk_context.swapchain_context.swapchain, NULL);

	vkDestroyDescriptorPool(d, vk_context.descriptor_pool, NULL);
	vkDestroyDescriptorSetLayout(d, vk_context.descriptor_set_layout, NULL);
	vkDestroyBuffer(d, vk_context.uniform_buffer, NULL);
	vkFreeMemory(d, vk_context.uniform_buffer_memory, NULL);
	vkDestroyBuffer(d, vk_context.vertex_buffer, NULL);
	vkFreeMemory(d, vk_context.vertex_buffer_memory, NULL);
	vkDestroyBuffer(d, vk_context.index_buffer, NULL);
	vkFreeMemory(d, vk_context.index_buffer_memory, NULL);
	vkDestroyDevice(d, NULL);
	vkDestroySurfaceKHR(vk_context.instance, vk_context.surface, NULL);
	vkDestroyInstance(vk_context.instance, NULL);
}

