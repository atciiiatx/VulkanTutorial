#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <functional>
#include <cstdlib>
#include <string>
#include <optional>
#include <set>

class QueueFamilyIndices {
public:
	std::optional<uint32_t> m_GraphicsFamily;
	std::optional<uint32_t> m_PresentFamily;
	bool isComplete() {
		return m_GraphicsFamily.has_value() && m_PresentFamily.has_value();
	}
};

class SwapChainSupportDetails {
public:
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

class HelloTriangleApplication {
public:
	const int WindowWidth = 800;
	const int WindowHeight = 600;
	const std::string AppName = "Vulkan Triangle";
	const std::vector<const char*> validationLayers = {
		"VK_LAYER_KHRONOS_validation"
	};
	const std::vector<const char*> deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

#ifdef NDEBUG
	const bool enableValidationLayers = false;
#else
	const bool enableValidationLayers = true;
#endif

	void run() {
		initWindow();
		initVulkan();
		mainLoop();
		cleanup();
	}

private:
	GLFWwindow* m_Window = nullptr;
	VkInstance m_Instance = {};
	VkDebugUtilsMessengerEXT m_DebugMessenger = 0;
	VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
	VkDevice m_Device = nullptr;
	VkQueue m_GraphicsQueue = nullptr;
	VkQueue m_PresentQueue = nullptr;
	VkSurfaceKHR m_Surface = nullptr;

	void initWindow() {
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		m_Window = glfwCreateWindow(WindowWidth, WindowHeight, AppName.c_str(), nullptr, nullptr);
	}

	void initVulkan() {
		createInstance();
		setupDebugMessenger();
		createSurface();
		pickPhysicalDevice();
		createLogicalDevice();
	}

	void pickPhysicalDevice()
	{
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(m_Instance, &deviceCount, nullptr);
		if (deviceCount <= 0)
		{
			throw std::runtime_error("Found no GPU with Vulkan support.");
		}
		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(m_Instance, &deviceCount, devices.data());
		for (auto& d : devices)
		{
			if (isDeviceSuitable(d))
			{
				std::cout << "Using device " << d << std::endl;
				m_PhysicalDevice = d;
				break;
			}
		}

		if (m_PhysicalDevice == VK_NULL_HANDLE)
		{
			throw std::runtime_error("No GPU is suitable.");
		}
	}

	bool isDeviceSuitable(VkPhysicalDevice& device)
	{
		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties(device, &properties);
		std::cout << "Device Properties:" << std::endl;
		std::cout << "API Version " << properties.apiVersion << std::endl;
		std::cout << "Device Name " << properties.deviceName << std::endl;
		std::cout << "Device Type " << properties.deviceType << std::endl;
		std::cout << "Vendor ID " << properties.vendorID << std::endl;

		VkPhysicalDeviceFeatures features;
		vkGetPhysicalDeviceFeatures(device, &features);
		std::cout << "Device Features:" << std::endl;
		std::cout << "Geometry Shader " << features.geometryShader << std::endl;

		auto families = findQueueFamilies(device);
		auto extensionsSupported = checkDeviceExtensionSupport(device);
		auto swapChainSupport = querySwapChainSupport(device);

		auto swapChainsAdequate = false;
		if (extensionsSupported)
		{
			SwapChainSupportDetails details = querySwapChainSupport(device);
			swapChainsAdequate = !details.formats.empty() && !details.presentModes.empty();
		}
		return families.isComplete() && extensionsSupported && swapChainsAdequate;
	}

	bool checkDeviceExtensionSupport(VkPhysicalDevice& device)
	{
		if (deviceExtensions.size() == 0)
		{
			return true;
		}

		uint32_t extensionCount = 0;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
		if (extensionCount == 0)
		{
			throw std::runtime_error("No device extensions");
		}
		std::vector<VkExtensionProperties> extensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, extensions.data());
		std::cout << "Device Extensions:" << std::endl;
		for (auto& ext : extensions)
		{
			std::cout << ext.extensionName << std::endl;
		}

		for (auto& de : deviceExtensions)
		{
			auto found = false;
			for (auto& ext : extensions)
			{
				if (strcmp(de, ext.extensionName) == 0)
				{
					found = true;
					break;
				}
			}
			if (!found)
			{
				return false;
			}
		}
		return true;
	}

	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice& device)
	{
		SwapChainSupportDetails details;

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_Surface, &details.capabilities);
		uint32_t formatCount = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Surface, &formatCount, nullptr);
		if (formatCount > 0)
		{
			details.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Surface, &formatCount, details.formats.data());
		}

		uint32_t presentModeCount = 0;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_Surface, &presentModeCount, nullptr);
		if (presentModeCount > 0)
		{
			details.presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_Surface, &presentModeCount, details.presentModes.data());
		}
		std::cout << "querySwapChainSupport formats=" << details.formats.size()
			<< " presentModes=" << details.presentModes.size() << std::endl;
		return details;
	}

	void createInstance()
	{
		if (enableValidationLayers && !checkValidationLayerSupport())
		{
			throw std::runtime_error("Validation layers requested are not available.");
		}

		VkApplicationInfo appInfo = {};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = AppName.c_str();
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;

		std::cout << "Extensions Supported:" << std::endl;
		uint32_t eCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &eCount, nullptr);
		std::vector<VkExtensionProperties> eList(eCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &eCount, eList.data());
		for (auto& e : eList)
		{
			std::cout << e.extensionName << " " << e.specVersion << std::endl;
		}

		std::cout << "Extensions Required:" << std::endl;
		auto extensions = getRequiredExtensions();
		for (auto& ext: extensions)
		{
			std::cout << ext << std::endl;
		}
		
		VkInstanceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;
		createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		createInfo.ppEnabledExtensionNames = extensions.data();
		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
		if (enableValidationLayers)
		{
			std::cout << "Enabling validation layers:" << std::endl;
			for (auto& vl : validationLayers)
			{
				std::cout << vl << std::endl;
			}
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();

			populateDebugMessengerCreateInfo(debugCreateInfo);
			createInfo.pNext = &debugCreateInfo;

		}
		else
		{
			std::cout << "Enabling no validation layers." << std::endl;
			createInfo.enabledLayerCount = 0;
			createInfo.pNext = nullptr;
		}

		if (vkCreateInstance(&createInfo, nullptr, &m_Instance) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create Vulkan instance.");
		}
	}

	bool checkValidationLayerSupport()
	{
		uint32_t availLayerCount = 0;
		vkEnumerateInstanceLayerProperties(&availLayerCount, nullptr);
		std::vector<VkLayerProperties> availLayerProperties(availLayerCount);
		vkEnumerateInstanceLayerProperties(&availLayerCount, availLayerProperties.data());

		std::cout << "Available validation layers:" << std::endl;
		for (auto& al : availLayerProperties)
		{
			std::cout << al.layerName << " " << al.description << std::endl;
		}

		for (auto& vl : validationLayers)
		{
			bool found = false;
			for (auto& al : availLayerProperties)
			{
				if (strcmp(vl, al.layerName) == 0)
				{
					found = true;
					break;
				}
			}

			if (!found)
			{
				return false;
			}
		}
		return true;
	}

	std::vector<const char*> getRequiredExtensions()
	{
		uint32_t glfwExtensionCount = 0;
		auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		std::vector<const char*> extensions(glfwExtensions, glfwExtensions  + glfwExtensionCount);
		if (enableValidationLayers)
		{
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}
		return extensions;
	}

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData
	)
	{
		std::cerr << "Validation layer: " << pCallbackData->pMessage << std::endl;
		return VK_FALSE;
	}

	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
	{
		createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity =
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT ||
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType =
			VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = debugCallback;
		createInfo.pUserData = nullptr;
	}

	void setupDebugMessenger()
	{
		if (!enableValidationLayers) return;

		VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
		populateDebugMessengerCreateInfo(createInfo);
		if (CreateDebugUtilsMessengerEXT(m_Instance, &createInfo, nullptr, &m_DebugMessenger) !=
			VK_SUCCESS)
		{
			throw std::runtime_error("Failed to setup debug messenger.");
		}
	}

	static VkResult CreateDebugUtilsMessengerEXT(
		VkInstance instance,
		VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
		const VkAllocationCallbacks* pAllocator,
		VkDebugUtilsMessengerEXT* pDebugMessenger)
	{
		const char* funcName = "vkCreateDebugUtilsMessengerEXT";
		auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance,
			funcName);
		if (func != nullptr)
		{
			std::cout << "Found function " << funcName << std::endl;
			return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
		}
		else
		{
			std::cout << "Did not find function " << funcName << std::endl;
			return VK_ERROR_EXTENSION_NOT_PRESENT;
		}
	}

	static void DestroyDebugUtilsMessengerEXT(
		VkInstance instance,
		const VkAllocationCallbacks* pAllocator,
		VkDebugUtilsMessengerEXT pDebugMessenger)
	{
		const char* funcName = "vkDestroyDebugUtilsMessengerEXT";
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance,
			funcName);
		if (func != nullptr)
		{
			std::cout << "Found function " << funcName << std::endl;
			func(instance, pDebugMessenger, pAllocator);
		}
		else
		{
			std::cout << "Did not find function " << funcName << std::endl;
		}
	}

	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device)
	{
		QueueFamilyIndices indices;
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
		std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilyProperties.data());

		std::cout << "Device Queue Families:" << std::endl;
		for (auto& qp : queueFamilyProperties)
		{
			std::cout << "Count " << qp.queueCount << " Flags " << qp.queueFlags << std::endl;
		}

		int i = 0;
		for (auto& qp : queueFamilyProperties)
		{
			if (qp.queueCount > 0 && qp.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				indices.m_GraphicsFamily = i;
				std::cout << "Graphics Family = " << i << std::endl;
			}
			VkBool32 presentSupport = false;
			if (vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_Surface, &presentSupport) != VK_SUCCESS)
			{
				throw std::runtime_error("Trouble querying device for present support.");
			}
			if ((qp.queueCount > 0) && presentSupport)
			{
				indices.m_PresentFamily = i;
				std::cout << "Present Family = " << i << std::endl;
			}
			if (indices.isComplete())
			{
				break;
			}
			i++;
		}
		return indices;
	}

	void createLogicalDevice()
	{
		auto indices = findQueueFamilies(m_PhysicalDevice);
		std::vector <VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<uint32_t> uniqueQueueFamilies = { 
			indices.m_GraphicsFamily.value(),
			indices.m_PresentFamily.value() 
		};
		float queuePriority = 1.0f;
		for (auto& qFamily : uniqueQueueFamilies)
		{
			VkDeviceQueueCreateInfo queueCreateInfo = {};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.queueFamilyIndex = qFamily;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfo.pNext = nullptr;
			queueCreateInfos.push_back(queueCreateInfo);
		}

		VkPhysicalDeviceFeatures physicalDeviceFeatures = {};
		VkDeviceCreateInfo deviceCreateInfo = {};
		deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
		deviceCreateInfo.pEnabledFeatures = &physicalDeviceFeatures;
		deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
		deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
		if (enableValidationLayers)
		{
			deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			deviceCreateInfo.ppEnabledLayerNames = validationLayers.data();
		}
		else
		{
			deviceCreateInfo.enabledLayerCount = 0;
		}

		if (vkCreateDevice(m_PhysicalDevice, &deviceCreateInfo, nullptr, &m_Device) != VK_SUCCESS)
		{
			throw std::runtime_error("Can't create logical device.");
		}

		vkGetDeviceQueue(m_Device, indices.m_GraphicsFamily.value(), 0, &m_GraphicsQueue);
		vkGetDeviceQueue(m_Device, indices.m_PresentFamily.value(), 0, &m_PresentQueue);
	}

	void createSurface()
	{
		if (glfwCreateWindowSurface(m_Instance, m_Window, nullptr, &m_Surface) != VK_SUCCESS)
		{
			throw std::runtime_error("Can't create window surface");
		}
	}

	void mainLoop() {
		while (!glfwWindowShouldClose(m_Window))
		{
			glfwPollEvents();
		}
	}

	void cleanup() {
		if (enableValidationLayers)
		{
			DestroyDebugUtilsMessengerEXT(m_Instance, nullptr, m_DebugMessenger);
		}
		vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
		vkDestroyDevice(m_Device, nullptr);
		vkDestroyInstance(m_Instance, nullptr);
		glfwDestroyWindow(m_Window);
		glfwTerminate();
	}
};

int main() {
	HelloTriangleApplication app;

	try {
		app.run();
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}