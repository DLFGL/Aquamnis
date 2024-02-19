#include "AM_Entity.h"

glm::mat4 AM_Entity::GetNormalMatrix()
{
	const float c3 = glm::cos(myRotation.z);
	const float s3 = glm::sin(myRotation.z);
	const float c2 = glm::cos(myRotation.x);
	const float s2 = glm::sin(myRotation.x);
	const float c1 = glm::cos(myRotation.y);
	const float s1 = glm::sin(myRotation.y);
	const glm::vec3 invScale = 1.f / myScale;
	return glm::mat4
	{
		{
			invScale.x * (c1 * c3 + s1 * s2 * s3),
			invScale.x * (c2 * s3),
			invScale.x * (c1 * s2 * s3 - c3 * s1),
			0.f
		},
		{
			invScale.y * (c3 * s1 * s2 - c1 * s3),
			invScale.y * (c2 * c3),
			invScale.y * (c1 * c3 * s2 + s1 * s3),
			0.f
		},
		{
			invScale.z * (c2 * s1),
			invScale.z * (-s2),
			invScale.z * (c1 * c2),
			0.f
		},
		{
			0.f, 0.f, 0.f, 1.f
		}
	};
}

glm::mat4 AM_Entity::GetMatrix()
{
	const float c3 = glm::cos(myRotation.z);
	const float s3 = glm::sin(myRotation.z);
	const float c2 = glm::cos(myRotation.x);
	const float s2 = glm::sin(myRotation.x);
	const float c1 = glm::cos(myRotation.y);
	const float s1 = glm::sin(myRotation.y);
	return glm::mat4
	{
		{
			myScale.x * (c1 * c3 + s1 * s2 * s3),
			myScale.x * (c2 * s3),
			myScale.x * (c1 * s2 * s3 - c3 * s1),
			0.0f,
		},
		{
			myScale.y * (c3 * s1 * s2 - c1 * s3),
			myScale.y * (c2 * c3),
			myScale.y * (c1 * c3 * s2 + s1 * s3),
			0.0f,
		},
		{
			myScale.z * (c2 * s1),
			myScale.z * (-s2),
			myScale.z * (c1 * c2),
			0.0f,
		},
		{
			myTranslation.x,
			myTranslation.y,
			myTranslation.z,
			1.0f
		}
	};
}

AM_Entity::AM_Entity(uint64_t anID) 
	: myTexture{}
	, myTempVertexBuffer{}
	, myTempIndexBuffer{}
	, myColor{ 1.f, 1.f, 1.f }
	, myTranslation{ 0.f, 0.f, 0.f }
	, myScale{ 1.f, 1.f, 1.f }
	, myRotation{ 0.f, 0.f, 0.f }
	, myId(anID)
	, myLightIntensity(0.f)
	, myIsSkybox(false)
	, myIsEmissive(false)
{
}

AM_Entity* AM_Entity::CreateEntity()
{
	static uint64_t id = 1; // reserve 0
	return new AM_Entity(id++);
}
