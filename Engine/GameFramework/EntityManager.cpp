#include "EntityManager.h"
#include "Math/Base.h"

#include "MarcoCommon.h"
#include "CoreShare.h"

namespace ECS
{
#if CORE_SHARE_CODE
	static EntityManager* GManagerList[1 <<(EntityHandle::ManagerIndexBits)] = {};

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
#endif


	EntityHandle EntityManager::createEntity()
	{
		uint32 indexSlot;
		if( mFreeEntityIndices.empty() )
		{
			uint32 startIndex = mEntityLists.size();
			uint32 numNewNum = Math::Max<uint32>( 3 * mEntityLists.size() / 2 , 10 );
			mEntityLists.resize(mEntityLists.size() + numNewNum);

			indexSlot = startIndex;
			--numNewNum;
			mFreeEntityIndices.resize(numNewNum);
			for( uint32 i = 0; i < numNewNum; ++i )
			{
				mFreeEntityIndices[i] = startIndex + i + 1;
			}
			std::make_heap(mFreeEntityIndices.begin(), mFreeEntityIndices.end(), FreeSlotCmp());

		}
		else
		{
			std::pop_heap(mFreeEntityIndices.begin(), mFreeEntityIndices.end(), FreeSlotCmp());
			indexSlot = mFreeEntityIndices.back();
			mFreeEntityIndices.pop_back();
		}

		auto& data = mEntityLists[indexSlot];
		data.flags |= EEntityFlag::Used;
		data.handle = EntityHandle(indexSlot, mNextSerialNumber, mIndexSlot);

		mUsedEntityIndices.push_back(indexSlot);

		++mNextSerialNumber;
		if( mNextSerialNumber == 0 )
		{
			mNextSerialNumber = 1;
		}

		return data.handle;
	}


	EntityComponent* EntityManager::getComponentInternal(EntityHandle const& handle, ComponentType* type)
	{
		CHECK(isValid(handle));
		EntityData& entityData = mEntityLists[handle.indexSlot];
		for(auto const& componetData : entityData.components )
		{
			if (componetData.type == type)
				return componetData.ptr;
		}
		return nullptr;
	}

	EntityComponent* EntityManager::addComponentInternal(EntityHandle const& handle, ComponentType* type)
	{
		CHECK(isValid(handle));
		EntityComponent* component = type->getPool()->fetchComponent();
		if( component )
		{
			ComponentData componentData;
			componentData.ptr = component;
			componentData.type = type;
			mEntityLists[handle.indexSlot].components.push_back(componentData);

			type->registerComponent(handle, component);
		}
		return component;
	}

	bool EntityManager::removeComponentInternal(EntityHandle const& handle, ComponentType* type)
	{
		CHECK(isValid(handle));
		EntityData& entityData = mEntityLists[handle.indexSlot];
		auto iter = std::find_if( 
			entityData.components.begin() , entityData.components.end() , 
			[type](auto const& componetData)
			{
				return componetData.type == type;
			});


		if( iter == entityData.components.end() )
			return false;

		EntityComponent* component = iter->ptr;
		entityData.components.erase(iter);
		type->unregisterComponent(handle, component);
		type->getPool()->releaseComponent(component);
		return true;
	}

	void EntityManager::destroyEntity(EntityHandle const& handle)
	{
		if( !isValid(handle) )
			return;

		visitListener([handle](IEntityEventLister* listener)
		{
			listener->noitfyEntityPrevDestroy(handle);
		});

		auto& entityData = mEntityLists[handle.indexSlot];

		for (auto const& compentData : entityData.components)
		{
			compentData.type->unregisterComponent(handle, compentData.ptr);
		}

		mFreeEntityIndices.push_back(handle.indexSlot);
		entityData.flags = EEntityFlag::PendingKill;
		std::push_heap(mFreeEntityIndices.begin(), mFreeEntityIndices.end(), FreeSlotCmp());

		visitListener([handle](IEntityEventLister* listener)
		{
			listener->noitfyEntityPostDestroy(handle);
		});
	}

	bool EntityManager::isValid(EntityHandle const& handle) const
	{
		CHECK(handle.indexManager == mIndexSlot);

		if( handle.indexSlot >= mEntityLists.size() )
			return false;

		auto& entityData = mEntityLists[handle.indexSlot];

		if (handle.serialNumber != entityData.handle.serialNumber)
			return false;

		if (entityData.flags & EEntityFlag::PendingKill)
			return false;

		return true;
	}

	bool EntityManager::isPadingKill(EntityHandle const& handle) const
	{
		assert(isValid(handle));
		return !!(mEntityLists[handle.indexSlot].flags & EEntityFlag::PendingKill);
	}

}