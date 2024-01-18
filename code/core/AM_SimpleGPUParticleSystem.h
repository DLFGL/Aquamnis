#pragma once
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include "AM_VkContext.h"
#include "AM_VkPrimitives.h"
#include "AM_VkDescriptorUtils.h"
#include <glm/glm.hpp>

class AM_Buffer;
class AM_SimpleGPUParticleSystem
{
public:
	AM_SimpleGPUParticleSystem(AM_VkContext& aVkContext, VkRenderPass aRenderPass);
	~AM_SimpleGPUParticleSystem(){}

	void Render(VkCommandBuffer aCommandBuffer, VkDescriptorSet& aDescriptorSet, AM_Buffer* anSSBO);
	// need to run outside of any render pass
	void DispatchWork(VkCommandBuffer aCommandBuffer, VkDescriptorSet& aDescriptorSet);
	const VkDescriptorSetLayout GetDescriptorSetLayout() const { return myDescriptorSetLayout.GetDescriptorSetLayout(); }
	VkDescriptorSetLayout& GetDescriptorSetLayout() { return myDescriptorSetLayout.GetDescriptorSetLayout(); }
	AM_VkDescriptorSetLayout& GetDescriptorSetLayoutWrapper() { return myDescriptorSetLayout; }

private:
	struct PushConstantData
	{
		glm::mat4 normalMat;
		glm::mat4 transform;
	};

	AM_SimpleGPUParticleSystem(const AM_SimpleGPUParticleSystem&) = delete;
	AM_SimpleGPUParticleSystem& operator=(const AM_SimpleGPUParticleSystem&) = delete;

	void CreateComputePipeline();
	void CreateGraphicsPipeline(VkRenderPass aRenderPass);

	// temp utils
	static std::vector<char> ReadFile(const std::string& filename);

	// should go into pipeline
	VkShaderModule CreateShaderModule(const std::vector<char>& code);
	void CreateDescriptorSetLayout();

	AM_VkContext& myVkContext;
	AM_VkPipeline myComputePipeline;
	AM_VkPipelineLayout myPipelineLayout;
	AM_VkDescriptorSetLayout myDescriptorSetLayout;

	AM_VkPipeline myGraphicsPipeline;
	AM_VkPipelineLayout myGfxPipelineLayout;
	AM_VkDescriptorSetLayout myGfxDescriptorSetLayout;
	
	uint32_t* myMaxComputeWorkGroupCount;
	uint32_t myMaxComputeWorkGroupInvocations;
	uint32_t* myMaxComputeWorkGroupSize;
};

