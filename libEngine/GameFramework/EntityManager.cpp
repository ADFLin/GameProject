#include "EntityManager.h"

namespace ECS
{

	EntityHandle EntityManager::createEntity()
	{
		uint32 indexSlot;
		if( mFreeSlotIndices.empty() )
		{
			uint32 startIndex = mEntityLists.size();
			uint32 numNewNum = mEntityLists.size();
			mEntityLists.resize(mEntityLists.size() + numNewNum);

			indexSlot = startIndex;
			--numNewNum;
			mFreeSlotIndices.resize(numNewNum);
			for( uint32 i = 0; i < numNewNum; ++i )
			{
				mFreeSlotIndices[i] = startIndex + i + 1;
			}
			std::make_heap(mFreeSlotIndices.begin(), mFreeSlotIndices.end(), FreeSlotCmp());

		}
		else
		{
			std::pop_heap(mFreeSlotIndices.begin(), mFreeSlotIndices.end(), FreeSlotCmp());
			indexSlot = mFreeSlotIndices.back();
			mFreeSlotIndices.pop_back();
		}

		auto& data = mEntityLists[indexSlot];
		data.flags |= EEntityFlag::Used;
		data.serialNumber = mNextSerialNumber;

		++mNextSerialNumber;
		if( mNextSerialNumber == 0 )
		{
			mNextSerialNumber = 1;
		}

		return EntityHandle(indexSlot, data.serialNumber);
	}

	int EntityManager::getAllComponentInternal(EntityHandle const& handle, ComponentType* type, std::vector< EnitiyComponent* >& outComponents)
	{
		assert(isVaild(handle));
		int result = 0;
		for( auto componet : mEntityLists[handle.indexSlot].components )
		{
			if( componet->type == type )
			{
				outComponents.push_back(componet);
				++result;
			}
		}
		return result;
	}

	EnitiyComponent* EntityManager::getComponentInternal(EntityHandle const& handle, ComponentType* type)
	{
		assert(isVaild(handle));
		for( auto componet : mEntityLists[handle.indexSlot].components )
		{
			if( componet->type == type )
				return componet;
		}
		return nullptr;
	}

	EnitiyComponent* EntityManager::addComponentInternal(EntityHandle const& handle, ComponentType* type)
	{
		assert(isVaild(handle));
		EnitiyComponent* component = type->getPool()->fetchComponent();
		if( component )
		{
			mEntityLists[handle.indexSlot].components.push_back(component);
		}
		return component;
	}

	bool EntityManager::removeComponentInternal(EntityHandle const& handle, ComponentType* type)
	{
		assert(isVaild(handle));
		EntityData& entityData = mEntityLists[handle.indexSlot];
		auto iter = std::find_if( 
			entityData.components.begin() , entityData.components.end() , 
			[type](EnitiyComponent* comp)
			{
				return comp->type == type;
			});

		if( iter == entityData.components.end() )
			return false;

		EnitiyComponent* component = *iter;
		entityData.components.erase(iter);
		type->getPool()->releaseComponent(component);
		return true;
	}

	void EntityManager::destroyEntity(EntityHandle const& handle)
	{
		if( !isVaild(handle) )
			return;

		notifyEvent([handle](IEntityEventLister* listener)
		{
			listener->noitfyEntityPrevDestroy(handle);
		});

		auto& data = mEntityLists[handle.indexSlot];


		mFreeSlotIndices.push_back(handle.indexSlot);
		data.flags = 0;
		std::push_heap(mFreeSlotIndices.begin(), mFreeSlotIndices.end(), FreeSlotCmp());

		notifyEvent([handle](IEntityEventLister* listener)
		{
			listener->noitfyEntityPostDestroy(handle);
		});
	}

	bool EntityManager::isVaild(EntityHandle const& handle) const
	{
		if( handle.indexSlot >= mEntityLists.size() )
			return false;

		return handle.serialNumber == mEntityLists[handle.indexSlot].serialNumber;
	}

	bool EntityManager::isPadingKill(EntityHandle const& handle) const
	{
		assert(isVaild(handle));
		return !!(mEntityLists[handle.indexSlot].flags & EEntityFlag::PaddingKill);
	}

}