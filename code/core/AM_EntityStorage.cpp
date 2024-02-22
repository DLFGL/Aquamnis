#include "AM_EntityStorage.h"

#include "AM_Entity.h"
#include "AM_VmaUsage.h"

AM_EntityStorage::AM_EntityStorage()
{
	myEntities.reserve(10);
}

AM_EntityStorage::~AM_EntityStorage()
{
	for (auto& kv : myEntities)
	{
		if (kv.second)
			delete kv.second;
	}
}

AM_Entity* AM_EntityStorage::Add()
{
	AM_Entity* newEntity = AM_Entity::CreateEntity();
	myEntities.emplace(newEntity->myId, newEntity);
	return newEntity;
}

// not good for concurrency
AM_Entity* AM_EntityStorage::GetIfExist(uint64_t anId)
{
	auto ite = myEntities.find(anId);
	if (ite != myEntities.end())
		return ite->second;
	return nullptr;
}

void AM_EntityStorage::GetEntitiesOfType(std::vector<AM_Entity*>& outEntities, uint8_t aType)
{
	outEntities.clear();
	for (auto& kv : myEntities)
	{
		auto* entity = kv.second;
		if (entity->GetType() != aType)
			continue;
		outEntities.push_back(entity);
	}
}

void AM_EntityStorage::DestroyEntities(AM_VkContext& aVkContext, VmaAllocator anAllocator)
{
	for (auto& kv : myEntities)
		kv.second->DestroyVkResources(aVkContext, anAllocator);
	myEntities.clear();
}
