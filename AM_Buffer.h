#pragma once
#include "AM_AllocationObject.h"

class AM_Buffer : public AM_AllocationObject
{
public:
	AM_Buffer() 
		: AM_AllocationObject(AM_AllocationObject::BUFFER)
		, myBuffer(VK_NULL_HANDLE)
	{
	}

	AM_Buffer(const VkBuffer aBuffer, const uint64_t anOffset, const uint64_t aSize)
		: AM_AllocationObject(AM_AllocationObject::BUFFER, anOffset, aSize)
		, myBuffer(aBuffer)
	{
	}

	AM_Buffer(AM_Buffer&& aBuffer) noexcept
		: AM_AllocationObject(std::move(aBuffer))
	{
		*this = std::move(aBuffer);
	}

	~AM_Buffer() = default;

	AM_Buffer& operator=(AM_Buffer&& aBuffer) noexcept
	{
		if (this == &aBuffer)
			return *this;

		myBuffer = std::exchange(aBuffer.myBuffer, nullptr);
		return *this;
	}

	void Reset()
	{
		AM_AllocationObject::Reset();
		myBuffer = VK_NULL_HANDLE;
	}

	VkBuffer myBuffer;
private:
	AM_Buffer(const AM_Buffer& aBuffer) = delete;
	AM_Buffer& operator=(const AM_Buffer& aBuffer) = delete;
};

