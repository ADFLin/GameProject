#include "EntityManager.h"
#include "Math/Base.h"

#include "MarcoCommon.h"

namespace ECS
{

	static EntityManager* GManagerList[1 << 8] = {};

	void EntityManager::registerToList()
	{
		CHECK(mIndexSlot == INDEX_NONE);
		for (int i = 0; i < ARRAY_SIZE(GManagerList); ++i)
		{
			if (GManagerList[i] == nullptr)
			{
				mIndexSlot = i;
				GManagerList[i] = this;
				return;
			}
		}

		NEVER_REACH("Entity Manager Can't Register");
	}

	void EntityManager::unregisterFromList()
	{
		CHECK(mIndexSlot != INDEX_NONE && GManagerList[mIndexSlot] == this);
		GManagerList[mIndexSlot] = nullptr;
		mIndexSlot = INDEX_NONE;
	}

	EntityManager* EntityManager::FromHandle(EntityHandle const& handle)
	{
		if (!handle.isNull())
		{
			CHECK(handle.indexManager < ARRAY_SIZE(GManagerList));
			return GManagerList[handle.indexManager];
		}

		return nullptr;
	}

	EntityManager& EntityManager::FromHandleChecked(EntityHandle const& handle)
	{
		CHECK(handle.indexManager < ARRAY_SIZE(GManagerList));
		return *GManagerList[handle.indexManager];
	}

	EntityHandle EntityManager::createEntity()
	{
		uint32 indexSlot;
		if( mFreeSlotIndices.empty() )
		{
			uint32 startIndex = mEntityLists.size();
			uint32 numNewNum = Math::Max<uint32>( 3 * mEntityLists.size() / 2 , 10 );
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
		data.handle = EntityHandle(indexSlot, mNextSerialNumber, mIndexSlot);

		++mNextSerialNumber;
		if( mNextSerialNumber == 0 )
		{
			mNextSerialNumber = 1;
		}

		return data.handle;
	}

	int EntityManager::getAllComponentInternal(EntityHandle const& handle, ComponentType* type, std::vector< EnitiyComponent* >& outComponents)
	{
		assert(isValid(handle));
		int result = 0;
		for( auto const& componetData : mEntityLists[handle.indexSlot].components )
		{
			if( componetData.type == type )
			{
				outComponents.push_back(componetData.ptr);
				++result;
			}
		}
		return result;
	}

	EnitiyComponent* EntityManager::getComponentInternal(EntityHandle const& handle, ComponentType* type)
	{
		assert(isValid(handle));
		for(auto const& componetData : mEntityLists[handle.indexSlot].components )
		{
			if (componetData.type == type)
				componetData.ptr;
		}
		return nullptr;
	}

	EnitiyComponent* EntityManager::addComponentInternal(EntityHandle const& handle, ComponentType* type)
	{
		assert(isValid(handle));
		EnitiyComponent* component = type->getPool()->fetchComponent();
		if( component )
		{
			ComponentData componentData;
			componentData.ptr = component;
			componentData.type = type;
			mEntityLists[handle.indexSlot].components.push_back(componentData);
		}
		return component;
	}

	bool EntityManager::removeComponentInternal(EntityHandle const& handle, ComponentType* type)
	{
		assert(isValid(handle));
		EntityData& entityData = mEntityLists[handle.indexSlot];
		auto iter = std::find_if( 
			entityData.components.begin() , entityData.components.end() , 
			[type](auto const& componetData)
			{
				return componetData.type == type;
			});

		if( iter == entityData.components.end() )
			return false;

		EnitiyComponent* component = iter->ptr;
		entityData.components.erase(iter);
		type->getPool()->releaseComponent(component);
		return true;
	}

	void EntityManager::destroyEntity(EntityHandle const& handle)
	{
		if( !isValid(handle) )
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

	bool EntityManager::isValid(EntityHandle const& handle) const
	{
		if (handle.indexManager != mIndexSlot)
			return false;

		if( handle.indexSlot >= mEntityLists.size() )
			return false;

		return handle.serialNumber == mEntityLists[handle.indexSlot].handle.serialNumber;
	}

	bool EntityManager::isPadingKill(EntityHandle const& handle) const
	{
		assert(isValid(handle));
		return !!(mEntityLists[handle.indexSlot].flags & EEntityFlag::PaddingKill);
	}

}