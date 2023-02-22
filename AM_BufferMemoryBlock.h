#pragma once
#include "AM_Buffer.h"
#include "AM_SimpleMemoryBlock.h"
#include "AM_VkBuffer.h"

class AM_BufferMemoryBlock : public AM_SimpleMemoryBlock
{
public:
	AM_BufferMemoryBlock()
		: AM_SimpleMemoryBlock(AM_SimpleMemoryBlock::BUFFER)
	{
	}

	AM_BufferMemoryBlock(AM_BufferMemoryBlock&& aMemoryBlock) noexcept
		: AM_SimpleMemoryBlock(std::move(aMemoryBlock))
	{
		*this = std::move(aMemoryBlock);
	}

	~AM_BufferMemoryBlock()
	{
		myAllocationList.clear();
		myBuffer.Release();
		if (myMemory)
			vkFreeMemory(VkDrawContext::device, myMemory, nullptr);
	}

	void Init(AM_VkBuffer& aVkBuffer, const uint32_t aMemoryTypeIndex, const uint64_t anAlignment)
	{
		myBuffer = std::move(aVkBuffer);
		myAlignment = anAlignment;

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = VkDrawConstants::SINGLEALLOCSIZE;
		allocInfo.memoryTypeIndex = aMemoryTypeIndex;
		if (vkAllocateMemory(VkDrawContext::device, &allocInfo, nullptr, &myMemory) != VK_SUCCESS)
			throw std::runtime_error("failed to allocate memory of type ??? !");

		vkBindBufferMemory(VkDrawContext::device, myBuffer.myBuffer, myMemory, 0);
	}

	AM_Buffer* Allocate(const uint64_t aSize)
	{
		AM_Buffer& buffer = myAllocationList.emplace_back(myBuffer.myBuffer, myExtent, aSize);
		buffer.SetIsEmpty(false);
		myExtent += aSize;
		return &buffer;
	}

	std::list<AM_Buffer> myAllocationList;
	AM_VkBuffer myBuffer;

private:
	AM_BufferMemoryBlock(const AM_BufferMemoryBlock& aMemoryBlock) = delete;
	AM_BufferMemoryBlock& operator=(const AM_BufferMemoryBlock& aMemoryBlock) = delete;
	AM_BufferMemoryBlock& operator=(AM_BufferMemoryBlock&& aMemoryBlock) noexcept
	{
		if (this == &aMemoryBlock)
			return *this;

		myAllocationList = std::move(aMemoryBlock.myAllocationList);
		myBuffer = std::move(aMemoryBlock.myBuffer);
		return *this;
	}
};