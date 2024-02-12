#include "AM_VkContext.h"

AM_VkContext::AM_VkContext() 
	: deviceProperties{}
	, deviceFeatures{}
	, surfaceCapabilities{}
	, memoryProperties{}
	, surfaceFormat{}
	, enabledInstanceLayers
    {
#if _DEBUG
		"VK_LAYER_KHRONOS_validation" 
#endif
    }
	, deviceExtensions{ VK_KHR_SWAPCHAIN_EXTENSION_NAME }
#if _DEBUG
	, myDebugMessenger(VK_NULL_HANDLE)
#endif
	, instance(VK_NULL_HANDLE)
	, surface(VK_NULL_HANDLE)
	, physicalDevice(VK_NULL_HANDLE)
	, device(VK_NULL_HANDLE)
	, graphicsQueue(VK_NULL_HANDLE)
	, presentQueue(VK_NULL_HANDLE)
	, transferQueue(VK_NULL_HANDLE)
	, computeQueue(VK_NULL_HANDLE)
	, presentMode(VK_PRESENT_MODE_MAX_ENUM_KHR)
	, depthFormat(VK_FORMAT_MAX_ENUM)
	, maxMSAASamples(VK_SAMPLE_COUNT_1_BIT)
	, swapChainImageCount(0)
	, graphicsFamilyIndex(0)
	, presentFamilyIndex(0)
	, transferFamilyIndex(0)
	, computeFamilyIndex(0)
{
	GetAvailableInstanceExtensions();
	GetRequiredInstanceExtensions();
#if _DEBUG
	GetAvailableInstanceLayers();
#endif
}

AM_VkContext::~AM_VkContext()
{
	DestroyCommandPool(myTransferCommandPool);
	for (VkCommandPool commandPool : myComputeCommandPools)
		DestroyCommandPool(commandPool);

	for (VkCommandPool commandPool : myCommandPools)
		DestroyCommandPool(commandPool);

	vkDestroyDevice(device, nullptr);
	vkDestroySurfaceKHR(instance, surface, nullptr);
#ifdef _DEBUG
	DestroyDebugUtilsMessengerEXT(instance, myDebugMessenger, nullptr);
#endif
	vkDestroyInstance(instance, nullptr);
}

bool AM_VkContext::TryGetQueueFamilies(VkPhysicalDevice device, int& transferQueueIdx, int& graphicsQueueIdx, int& presentQueueIdx, int& computeQueueIdx)
{
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	for (uint32_t i = 0; i < queueFamilies.size(); ++i)
	{
		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
		if (!(queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT && presentSupport))
			continue;

		graphicsQueueIdx = i;
		presentQueueIdx = i;
		break;
	}

	for (uint32_t i = 0; i < queueFamilies.size(); ++i)
	{
		if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT || !(queueFamilies[i].queueFlags & VK_QUEUE_TRANSFER_BIT))
			continue;
		transferQueueIdx = i;
		break;
	}

	for (uint32_t i = 0; i < queueFamilies.size(); ++i)
	{
		if (!(queueFamilies[i].queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)))
			continue;
		computeQueueIdx = i;
		break;
	}

	return (graphicsQueueIdx >= 0) && (presentQueueIdx >= 0) && (transferQueueIdx >= 0) && (computeQueueIdx >= 0);
}

bool AM_VkContext::CheckDeviceExtensionSupport(VkPhysicalDevice device)
{
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

	std::unordered_set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
	for (const auto& extension : availableExtensions)
		requiredExtensions.erase(extension.extensionName);

	return requiredExtensions.empty();
}

void AM_VkContext::Init()
{
#if _DEBUG
	SetupDebugMessenger();
#endif
	ChoosePhysicalDevice();
	CreateLogicalDevice();
	CreateCommandPools();
	InitSwapChainCreationInfo();
	GetMaxMSAASampleCount();
	GetDepthFormat();
}

void AM_VkContext::GetAvailableInstanceExtensions()
{
	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
	availableInstanceExtensions.resize(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableInstanceExtensions.data());
}

void AM_VkContext::GetRequiredInstanceExtensions()
{
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	requiredInstanceExtensions.reserve(glfwExtensionCount + 1);
	requiredInstanceExtensions.insert(requiredInstanceExtensions.end(), glfwExtensions, glfwExtensions + glfwExtensionCount);
#ifdef _DEBUG
	requiredInstanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
}

void AM_VkContext::GetAvailableInstanceLayers()
{
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	availableInstanceLayers.resize(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableInstanceLayers.data());
}

void AM_VkContext::ChoosePhysicalDevice()
{
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

	if (deviceCount == 0)
		throw std::runtime_error("failed to find GPUs with Vulkan support!");

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

	for (const VkPhysicalDevice availableDevice : devices)
	{
		if (!CheckDeviceExtensionSupport(availableDevice))
			continue;

		uint32_t formatCount = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(availableDevice, surface, &formatCount, nullptr);
		if (formatCount < 1)
			continue;

		uint32_t presentModeCount = 0;
		vkGetPhysicalDeviceSurfacePresentModesKHR(availableDevice, surface, &presentModeCount, nullptr);
		if (presentModeCount < 1)
			continue;

		VkPhysicalDeviceProperties props;
		vkGetPhysicalDeviceProperties(availableDevice, &props);
		if (!(props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU))
			continue;

		VkPhysicalDeviceFeatures feats;
		vkGetPhysicalDeviceFeatures(availableDevice, &feats);
		if (!(feats.geometryShader && feats.samplerAnisotropy && feats.sampleRateShading))
			continue;

		int transferQueueIdx = -1, graphicsQueueIdx = -1, presentQueueIdx = -1, computeQueueIdx = -1;
		if (!TryGetQueueFamilies(availableDevice, transferQueueIdx, graphicsQueueIdx, presentQueueIdx, computeQueueIdx))
			continue;

		// We find a suitable device
		surfaceFormats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(availableDevice, surface, &formatCount, surfaceFormats.data());

		presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(availableDevice, surface, &presentModeCount, presentModes.data());

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(availableDevice, surface, &surfaceCapabilities);

		vkGetPhysicalDeviceMemoryProperties(availableDevice, &memoryProperties);

#ifdef _DEBUG
		std::cout << "Memory Info\n";
		for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i)
		{
			const VkMemoryType& type = memoryProperties.memoryTypes[i];
			std::cout << "index: " << i << ", property: " << type.propertyFlags << ", heap index: " << type.heapIndex << "\n";
		}
#endif //_DEBUG

		deviceProperties = std::move(props);
		deviceFeatures = std::move(feats);
		transferFamilyIndex = static_cast<uint32_t>(transferQueueIdx);
		graphicsFamilyIndex = static_cast<uint32_t>(graphicsQueueIdx);
		presentFamilyIndex = static_cast<uint32_t>(presentQueueIdx);
		computeFamilyIndex = static_cast<uint32_t>(computeQueueIdx);
		physicalDevice = availableDevice;
		return;
	}
	throw std::runtime_error("failed to find a suitable GPU!");
}

void AM_VkContext::CreateLogicalDevice()
{
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos{};
	std::unordered_set<uint32_t> uniqueQueueFamilies = { graphicsFamilyIndex, presentFamilyIndex, transferFamilyIndex, computeFamilyIndex };
	float queuePriority = 1.0f;
	for ( const uint32_t& queueFamily : uniqueQueueFamilies )
	{
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.pEnabledFeatures = &deviceFeatures;
	createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
	createInfo.ppEnabledExtensionNames = deviceExtensions.data();
	createInfo.enabledLayerCount = static_cast<uint32_t>(enabledInstanceLayers.size());
	createInfo.ppEnabledLayerNames = enabledInstanceLayers.data();

	if ( vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS )
		throw std::runtime_error("failed to create logical device!");

	vkGetDeviceQueue(device, graphicsFamilyIndex, 0, &graphicsQueue);
	vkGetDeviceQueue(device, presentFamilyIndex, 0, &presentQueue);
	vkGetDeviceQueue(device, transferFamilyIndex, 0, &transferQueue);
	vkGetDeviceQueue(device, computeFamilyIndex, 0, &computeQueue);
}

void AM_VkContext::InitSwapChainCreationInfo()
{
	surfaceFormat = surfaceFormats[0];
	for (const VkSurfaceFormatKHR& availableFormat : surfaceFormats)
	{
		if (availableFormat.format != VK_FORMAT_B8G8R8A8_SRGB || availableFormat.colorSpace != VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			continue;
		surfaceFormat = availableFormat;
		break;
	}

	presentMode = VK_PRESENT_MODE_FIFO_KHR;
	for (const VkPresentModeKHR& availablePresentMode : presentModes)
	{
		if (availablePresentMode != VK_PRESENT_MODE_IMMEDIATE_KHR)
			continue;
		presentMode = availablePresentMode;
	}
	for (const VkPresentModeKHR& availablePresentMode : presentModes)
	{
		if (presentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
			break;

		if (availablePresentMode != VK_PRESENT_MODE_MAILBOX_KHR)
			continue;
		presentMode = availablePresentMode;
	}
	swapChainImageCount = std::clamp(surfaceCapabilities.minImageCount + 1, surfaceCapabilities.minImageCount, surfaceCapabilities.maxImageCount);   
}

void AM_VkContext::GetMaxMSAASampleCount()
{
	VkSampleCountFlags counts =
		deviceProperties.limits.framebufferColorSampleCounts
		& deviceProperties.limits.framebufferDepthSampleCounts;

	if ( counts & VK_SAMPLE_COUNT_64_BIT )
		maxMSAASamples = VK_SAMPLE_COUNT_64_BIT;
	else if ( counts & VK_SAMPLE_COUNT_32_BIT )
		maxMSAASamples = VK_SAMPLE_COUNT_32_BIT;
	else if ( counts & VK_SAMPLE_COUNT_16_BIT )
		maxMSAASamples = VK_SAMPLE_COUNT_16_BIT;
	else if ( counts & VK_SAMPLE_COUNT_8_BIT )
		maxMSAASamples = VK_SAMPLE_COUNT_8_BIT;
	else if ( counts & VK_SAMPLE_COUNT_4_BIT )
		maxMSAASamples = VK_SAMPLE_COUNT_4_BIT;
	else if ( counts & VK_SAMPLE_COUNT_2_BIT )
		maxMSAASamples = VK_SAMPLE_COUNT_2_BIT;
	else
		maxMSAASamples = VK_SAMPLE_COUNT_1_BIT;
}

VkFormat AM_VkContext::FindSupportedFormat(const std::vector<VkFormat>& candidates, const VkImageTiling& tiling, const VkFormatFeatureFlags& features) const
{
	for (const VkFormat& format : candidates)
	{
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

		if (tiling == VK_IMAGE_TILING_LINEAR && ((props.linearTilingFeatures & features) ^ features) == 0)
			return format;

		if (tiling == VK_IMAGE_TILING_OPTIMAL && ((props.optimalTilingFeatures & features) ^ features) == 0)
			return format;
	}
	throw std::runtime_error("failed to find supported format!");
}

void AM_VkContext::GetDepthFormat()
{
	depthFormat = FindSupportedFormat
	(
		{
			VK_FORMAT_D32_SFLOAT,
			VK_FORMAT_D32_SFLOAT_S8_UINT,
			VK_FORMAT_D24_UNORM_S8_UINT
		},
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);
}

void AM_VkContext::CreateCommandPools()
{
	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = graphicsFamilyIndex;

	myCommandPools.resize(AM_VkRenderCoreConstants::MAX_FRAMES_IN_FLIGHT);
	myComputeCommandPools.resize(AM_VkRenderCoreConstants::MAX_FRAMES_IN_FLIGHT);

	for (int i = 0; i < AM_VkRenderCoreConstants::MAX_FRAMES_IN_FLIGHT; ++i)
	{
		myCommandPools[i] = CreateCommandPool(poolInfo);
		myComputeCommandPools[i] = CreateCommandPool(poolInfo);
	}

	VkCommandPoolCreateInfo transferPoolInfo{};
	transferPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	transferPoolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
	transferPoolInfo.queueFamilyIndex = transferFamilyIndex;

	myTransferCommandPool = CreateCommandPool(transferPoolInfo);
}

VkSemaphore AM_VkContext::CreateSemaphore()
{
	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkSemaphore obj;
	if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &obj) != VK_SUCCESS)
		throw std::runtime_error("failed to create semaphores!");
	return obj;
}

VkFence AM_VkContext::CreateFence()
{
	VkFence obj;
	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	if (vkCreateFence(device, &fenceInfo, nullptr, &obj) != VK_SUCCESS)
		throw std::runtime_error("failed to create semaphores!");

	return obj;
}

VkPipelineLayout AM_VkContext::CreatePipelineLayout(const VkPipelineLayoutCreateInfo& aCreateInfo)
{
	VkPipelineLayout obj;
	if (vkCreatePipelineLayout(device, &aCreateInfo, nullptr, &obj) != VK_SUCCESS)
		throw std::runtime_error("failed to create pipeline layout!");

	return obj;
}

VkPipeline AM_VkContext::CreateGraphicsPipeline(const VkGraphicsPipelineCreateInfo& aCreateInfo)
{
	VkPipeline obj;
	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &aCreateInfo, nullptr, &obj) != VK_SUCCESS)
		throw std::runtime_error("failed to create graphics pipeline!");
	return obj;
}

VkPipeline AM_VkContext::CreateComputePipeline(const VkComputePipelineCreateInfo& aCreateInfo)
{
	VkPipeline obj;
	if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &aCreateInfo, nullptr, &obj) != VK_SUCCESS)
		throw std::runtime_error("failed to create compute pipeline!");
	return obj;
}

VkFramebuffer AM_VkContext::CreateFrameBuffer(const VkFramebufferCreateInfo& aCreateInfo)
{
	VkFramebuffer obj;
	if (vkCreateFramebuffer(device, &aCreateInfo, nullptr, &obj) != VK_SUCCESS)
		throw std::runtime_error("failed to create framebuffer!");
	return obj;
}

VkCommandBuffer AM_VkContext::AllocateCommandBuffer(const VkCommandBufferAllocateInfo& aCreateInfo)
{
	VkCommandBuffer obj;
	if (vkAllocateCommandBuffers(device, &aCreateInfo, &obj) != VK_SUCCESS)
		throw std::runtime_error("failed to allocate command buffer!");
	return obj;
}

VkSampler AM_VkContext::CreateSampler(const VkSamplerCreateInfo& aCreateInfo)
{
	VkSampler obj;
	if (vkCreateSampler(device, &aCreateInfo, nullptr, &obj) != VK_SUCCESS)
		throw std::runtime_error("failed to create texture sampler!");
	return obj;
}

void AM_VkContext::DestroySampler(VkSampler aSampler)
{
	vkDestroySampler(device, aSampler, nullptr);
}

VkImageView AM_VkContext::CreateImageView(const VkImageViewCreateInfo& aCreateInfo)
{
	VkImageView obj;
	if (vkCreateImageView(device, &aCreateInfo, nullptr, &obj) != VK_SUCCESS)
		throw std::runtime_error("failed to create texture image view!");
	return obj;
}

VkRenderPass AM_VkContext::CreateRenderPass(const VkRenderPassCreateInfo& aCreateInfo)
{
	VkRenderPass obj;
	if (vkCreateRenderPass(device, &aCreateInfo, nullptr, &obj) != VK_SUCCESS)
		throw std::runtime_error("failed to create render pass!");
	return obj;
}

VkCommandPool AM_VkContext::CreateCommandPool(const VkCommandPoolCreateInfo& aCreateInfo)
{
	VkCommandPool obj;
	if (vkCreateCommandPool(device, &aCreateInfo, nullptr, &obj) != VK_SUCCESS)
		throw std::runtime_error("failed to create command pool!");
	return obj;
}

VkDescriptorPool AM_VkContext::CreateDescriptorPool(uint32_t aMaxSetCount, VkDescriptorPoolCreateFlags somePoolFlags, const std::vector<VkDescriptorPoolSize>& somePoolSizes)
{
	VkDescriptorPool obj;

	VkDescriptorPoolCreateInfo descriptorPoolInfo{};
	descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(somePoolSizes.size());
	descriptorPoolInfo.pPoolSizes = somePoolSizes.data();
	descriptorPoolInfo.maxSets = aMaxSetCount;
	descriptorPoolInfo.flags = somePoolFlags;

	if (vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &obj) != VK_SUCCESS)
		throw std::runtime_error("failed to create descriptor pool!");
	return obj;
}

void AM_VkContext::AllocateDescriptorSets(VkDescriptorPool aPool, std::vector<VkDescriptorSetLayout>& someDescriptorSetLayouts, std::vector<VkDescriptorSet>& outDescriptorSets)
{
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = aPool;
	allocInfo.pSetLayouts = someDescriptorSetLayouts.data();
	allocInfo.descriptorSetCount = static_cast<uint32_t>(outDescriptorSets.size());

	if (vkAllocateDescriptorSets(device, &allocInfo, outDescriptorSets.data()) != VK_SUCCESS)
		throw std::runtime_error("failed to create descriptor set from pool!");
}

void AM_VkContext::WriteToDescriptorSet(VkDescriptorSet aDescriptorSet, std::vector<VkWriteDescriptorSet>& someWrites)
{
	for (VkWriteDescriptorSet& write : someWrites)
		write.dstSet = aDescriptorSet;

	vkUpdateDescriptorSets(device, static_cast<uint32_t>(someWrites.size()), someWrites.data(), 0, nullptr);
}

VkDescriptorSetLayout AM_VkContext::CreateDescriptorSetLayout(std::vector<VkDescriptorSetLayoutBinding>& someBindings)
{
	VkDescriptorSetLayout obj;

	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{};
	descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorSetLayoutInfo.bindingCount = static_cast<uint32_t>(someBindings.size());
	descriptorSetLayoutInfo.pBindings = someBindings.data();

	if (vkCreateDescriptorSetLayout(device, &descriptorSetLayoutInfo, nullptr, &obj) != VK_SUCCESS)
		throw std::runtime_error("failed to create descriptor set layout!");

	return obj;
}


#ifdef _DEBUG
VKAPI_ATTR VkBool32 VKAPI_CALL AM_VkContext::DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
	std::cerr << "\nvalidation layer: " << pCallbackData->pMessage << std::endl;
	return VK_FALSE;
}

void AM_VkContext::PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
	createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = DebugCallback;
}

void AM_VkContext::SetupDebugMessenger()
{
	VkDebugUtilsMessengerCreateInfoEXT createInfo{};
	PopulateDebugMessengerCreateInfo(createInfo);

	if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &myDebugMessenger) != VK_SUCCESS)
		throw std::runtime_error("failed to set up debug messenger!");
}
#endif //_DEBUG