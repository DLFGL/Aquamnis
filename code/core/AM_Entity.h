#pragma once
#include "AM_Texture.h"
#include "AM_VkContext.h"
#include "TempBuffer.h"
#include <glm/glm.hpp>
#include <vector>

VK_DEFINE_HANDLE(VmaAllocator);
class AM_EntityStorage;
class AM_Entity
{
	friend AM_EntityStorage;
public:
	enum EntityType : uint8_t
	{
		MESH,
		SKINNEDMESH,
		PARTICLE,
		BILLBOARD,
		SKYBOX,
		POINT,
		LINE,
	};

	struct EntityUBO
	{
		alignas(16) glm::mat4 transform{ 1.f };
		alignas(16) glm::mat4 normalMat{1.f};
		alignas(16) glm::vec4 color{1.f,1.f,1.f,1.f};
		alignas(4) float radius{1.f};
	};

	AM_Entity(AM_Entity&& anEntity) noexcept
	{
		*this = std::move(anEntity);
	}

	AM_Entity(const AM_Entity& anEntity) = delete;
	AM_Entity& operator=(const AM_Entity& anEntity) = delete;
	
	glm::mat4 GetNormalMatrix();
	glm::mat4 GetMatrix();
	
	uint64_t GetId() const { return myId; }
	EntityType GetType() const { return myType; }
	void SetType(EntityType aType) { myType = aType; }

	void SetIsEmissive(bool anIsEmissive) { myIsEmissive = anIsEmissive; }
	bool IsEmissive() const { return myIsEmissive; }

	void SetLightIntensity(float anIntensity = 1.f) { myLightIntensity = anIntensity; }
	float GetLightIntensity() const { return myLightIntensity; }

	void SetIsSkybox(bool anIsSkybox) { myIsSkybox = anIsSkybox; }
	bool GetIsSkybox() const { return myIsSkybox; }

	void SetColor(const glm::vec3& aColor) { myColor = aColor; }
	const glm::vec3& GetColor() const { return myColor; }

	AM_Texture& GetTexture() { return myTexture; }
	const AM_Texture& GetTexture() const { return myTexture; }

	void SetVertexBuffer(TempBuffer aVertexBuffer) { myTempVertexBuffer = aVertexBuffer; }
	void SetIndexBuffer(TempBuffer anIndexBuffer) { myTempIndexBuffer = anIndexBuffer; }
	void SetUniformBuffer(TempBuffer aUniformBuffer) { myTempUniformBuffer = aUniformBuffer; }
	const TempBuffer* GetTempVertexBuffer() const { return &myTempVertexBuffer; }
	const TempBuffer* GetTempIndexBuffer() const { return &myTempIndexBuffer; }
	const TempBuffer* GetUniformBuffer() const { return &myTempUniformBuffer; }

	const std::vector<VkDescriptorSet>& GetDescriptorSets() const { return myDescriptorSets; }
	std::vector<VkDescriptorSet>& GetDescriptorSets()  { return myDescriptorSets; }

	void SetTransparency(bool anIsTransparent) { myIsTransparent = anIsTransparent; }
	bool IsTransparent() const { return myIsTransparent; }

	void SetIndexBufferSize(uint32_t aSize) { myIndexBufferSize = aSize; }
	uint32_t GetIndexBufferSize() const { return myIndexBufferSize; }

	void DestroyVkResources(AM_VkContext& aVkContext, VmaAllocator anAllocator);
	void UpdateUBO_Transform();
	void UpdateUBO_Color();

	EntityUBO& GetUBO() { return myUBO; }
	const EntityUBO& GetUBO() const { return myUBO; }
	void ResetUpdateFlag(uint32_t aFrameIndex);
	bool GetShouldUpdateUniformBuffer(uint32_t aFrameIndex) const;

	glm::vec3 myTranslation{ 0.f, 0.f, 0.f };
	glm::vec3 myScale{ 1.f, 1.f, 1.f };
	glm::vec3 myRotation{ 0.f, 0.f, 0.f };

private:
	AM_Entity& operator=(AM_Entity&& anEntity) noexcept
	{
		if (this == &anEntity)
			return *this;

		myId = std::exchange(anEntity.myId, 0);
		myDescriptorSets = std::move(anEntity.myDescriptorSets);
		myUBO = anEntity.myUBO;
		myTexture = anEntity.myTexture;
		myTempVertexBuffer = anEntity.myTempVertexBuffer;
		myTempIndexBuffer = anEntity.myTempIndexBuffer;
		myTempUniformBuffer = anEntity.myTempUniformBuffer;
		myColor = anEntity.myColor;
		myTranslation = anEntity.myTranslation;
		myScale = anEntity.myScale;
		myRotation = anEntity.myRotation;
		myLightIntensity = anEntity.myLightIntensity;
		myIndexBufferSize = anEntity.myIndexBufferSize;
		myType = anEntity.myType;
		myIsSkybox = anEntity.myIsSkybox;
		myIsEmissive = anEntity.myIsEmissive;
		myIsTransparent = anEntity.myIsTransparent;
		myShouldUpdateUniformBuffer = anEntity.myShouldUpdateUniformBuffer;
		return *this;
	}

	explicit AM_Entity(uint64_t anID);
	static AM_Entity* CreateEntity();
	void SetId(uint64_t anId) { myId = anId; }
	
	EntityUBO myUBO;
	AM_Texture myTexture;
	std::vector<VkDescriptorSet> myDescriptorSets;
	TempBuffer myTempVertexBuffer;
	TempBuffer myTempIndexBuffer;
	TempBuffer myTempUniformBuffer;
	glm::vec3 myColor;
	uint64_t myId;
	float myLightIntensity;
	uint32_t myIndexBufferSize;
	EntityType myType;
	uint8_t myShouldUpdateUniformBuffer;
	bool myIsSkybox : 1;
	bool myIsEmissive : 1;
	bool myIsTransparent : 1;
};

