#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <functional>
#include <cstdlib>
#include <string>
#include <optional>

class QueueFamilyIndices {
public:
	std::optional<uint32_t> m_GraphicsFamily;
	bool isComplete() {
		return m_GraphicsFamily.has_value();
	}
};
class HelloTriangleApplication {
public:
	const int WindowWidth = 800;
	const int WindowHeight = 600;
	const std::string AppName = "Vulkan Triangle";
	const std::vector<const char*> validationLayers = {
		"VK_LAYER_KHRONOS_validation"
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

	void initWindow() {
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		m_Window = glfwCreateWindow(WindowWidth, WindowHeight, AppName.c_str(), nullptr, nullptr);
	}

	void initVulkan() {
		createInstance();
		setupDebugMessenger();
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
		return families.isComplete();
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
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.queueFamilyIndex = indices.m_GraphicsFamily.value();
		float queuePriority = 1.0f;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfo.pNext = nullptr;

		VkPhysicalDeviceFeatures physicalDeviceFeatures = {};
		VkDeviceCreateInfo deviceCreateInfo = {};
		deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceCreateInfo.queueCreateInfoCount = 1;
		deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
		deviceCreateInfo.pEnabledFeatures = &physicalDeviceFeatures;
		deviceCreateInfo.enabledExtensionCount = 0;
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