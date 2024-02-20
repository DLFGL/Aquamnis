#pragma once
#include "AM_VkRenderMethod.h"
#include <glm/glm.hpp>

struct TempBuffer;
class AM_Entity;
class AM_Camera;
class AM_VkRenderMethodBillboard : public AM_VkRenderMethod<AM_VkRenderMethodBillboard>
{
public:
	AM_VkRenderMethodBillboard(
		AM_VkContext& aVkContext,
		const VkRenderPass aRenderPass,
		const std::string& aVertexShaderPath,
		const std::string& aFragmentShaderPath,
		uint32_t aBindingDescriptionCount = 1,
		uint32_t anAttributeDescriptionCount = 1,
		const VkVertexInputBindingDescription* aBindingDescription = nullptr,
		const VkVertexInputAttributeDescription* anAttributeDescription = nullptr);

	AM_VkRenderMethodBillboard(const AM_VkRenderMethodBillboard&) = delete;
	AM_VkRenderMethodBillboard& operator=(const AM_VkRenderMethodBillboard&) = delete;

	void CreatePipeline_Imp(AM_VkDescriptorSetLayoutBuilder& outBuilder, VkPushConstantRange& outRange, AM_PipelineUtils::GraphicsInitializer& outPipelineInitializer);
	void Render_Imp(AM_FrameRenderInfo& someInfo, std::vector<AM_Entity*>& someEntities, const TempBuffer* aBuffer);

private:
	struct PointLightPushConstants
	{
		glm::vec4 position{};
		glm::vec4 color{};
		float radius;
	};
};