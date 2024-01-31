#pragma once
#include <vulkan/vulkan.h>

struct VmaAllocation_T;
typedef VmaAllocation_T* VmaAllocation;

struct TempBuffer
{
	VkBuffer myBuffer{ nullptr };
	VmaAllocation myAllocation{ nullptr };
};