#ifndef Entity_h__
#define Entity_h__

#include "Core/TypeHash.h"
#include "MarcoCommon.h"
#include "CoreShare.h"

#include <vector>
#include <unordered_map>
#include <typeindex>
#include <algorithm>
#include <cassert>



namespace ECS
{

	class ComponentType;
	class EntityHandle;

	using EntityComponent = void;

	class IComponentPool
	{
	public:
		virtual void releaseComponent(EntityComponent* component) = 0;
		virtual EntityComponent* fetchComponent() = 0;
	};


	class ComponentType
	{
	public:
		virtual IComponentPool* getPool() = 0;
		virtual void registerComponent(EntityHandle const& handle, EntityComponent* component) {}
		virtual void unregisterComponent(EntityHandle const& handle, EntityComponent* component) {}

		uint32 mID;
	};

	class EntityHandle
	{
	public:

		enum 
		{
			ManagerIndexBits = 8 ,
			SlotIndexBits = 24,
			SerialNumberBits = 32,
		};
		bool operator == (EntityHandle const& rhs) const
		{
			return mPackedValue == rhs.mPackedValue;
		}

		bool isNull() const { return mPackedValue == 0; }

		uint32 getTypeHash() const
		{
			return HashValue(mPackedValue);
		}
		union
		{
			uint64 mPackedValue;
			struct
			{
				uint32 indexManager : ManagerIndexBits;
				uint32 indexSlot    : SlotIndexBits;
				uint32 serialNumber : SerialNumberBits;
			};
		};

		EntityHandle() :mPackedValue(0) {}


	private:
		EntityHandle(uint32 inIndexSlot, uint32 inSerialNumber, uint32 inIndexManager)
		{
			indexManager = inIndexManager;
			indexSlot = inIndexSlot;
			serialNumber = inSerialNumber;
		}
		friend class EntityManager;
	};


	class IEntityEventLister
	{
	public:
		virtual void notifyEntityCreation(EntityHandle const& handle) {}
		virtual void noitfyEntityPrevDestroy(EntityHandle const& handle) {}
		virtual void noitfyEntityPostDestroy(EntityHandle const& handle) {}

	};



	template< class T >
	class TSimpleComponentPool : public IComponentPool
	{
		virtual void releaseComponent(EntityComponent* component)
		{
			delete component;
		}
		virtual EntityComponent* fetchComponent()
		{
			return new T();
		}
	};



	enum EComponentCondiditon
	{
		Required,
		Excluded,
		Optional,
	};

	struct EntityServiceEnumer
	{
	public:
		template<typename T>
		void addComponent(EComponentCondiditon condition = EComponentCondiditon::Required)
		{
			CompSeviceDesc desc;
			desc.type = mManager->getComponentT<T>();
			desc.condition = condition;
			mDescList.push_back(desc);
		}

		struct CompSeviceDesc
		{
			ComponentType* type;
			EComponentCondiditon condition;
		};

		std::vector< CompSeviceDesc > mDescList;
		EntityManager* mManager;
	};

	class ISystemSerivce
	{
	public:
		virtual void notiyEntityRegister(EntityHandle const& handle) {}
		virtual void notiyEntityUnregister(EntityHandle const& handle) {}
		virtual void enumServedComponents(EntityServiceEnumer& enumer) {}
	};


	class EntityManager
	{
	public:

		~EntityManager()
		{
			if (mIndexSlot != INDEX_NONE)
			{
				unregisterFromList();
			}
		}
		CORE_API void registerToList();
		CORE_API void unregisterFromList();

		CORE_API static EntityManager* FromHandle(EntityHandle const& handle);
		CORE_API static EntityManager& FromHandleChecked(EntityHandle const& handle);

		static bool IsValid(EntityHandle const& handle)
		{
			EntityManager* myManager = FromHandle(handle);
			if (myManager)
			{
				return myManager->isValid(handle);
			}
			return false;
		}

		template< class T >
		class TDefaultComponentType : public ComponentType
		{
		public:
			virtual IComponentPool* getPool()
			{
				return &poolInstance;
			}
			TSimpleComponentPool< T > poolInstance;
		};

		template< class TComponent , typename TComponentType = TDefaultComponentType< TComponent > >
		void registerComponentTypeT()
		{
			CHECK(mComponentTypeMap.find(std::type_index(typeid(TComponent))) == mComponentTypeMap.end());
			static TComponentType sComponentTypeInstance;
			mComponentTypeMap.emplace(std::type_index(typeid(TComponent)), &sComponentTypeInstance);
		}

		template< class TComponent , typename TFunc >
		void visitComponent(TFunc&& func)
		{
			ComponentType* type = getComponentTypeT<TComponent>();
			for (auto index : mUsedEntityIndices)
			{
				EntityData& entityData = mEntityLists[index];
				for (auto const& componentData : entityData.components)
				{
					if (componentData.type == type)
					{
						func(entityData.handle, (T*)componentData.ptr);
					}
				}
			}
		}

		struct FreeSlotCmp
		{
			bool operator()(uint32 lhs, uint32 rhs) const
			{
				return lhs > rhs;
			}
		};

		EntityHandle createEntity();
		void  destroyEntity(EntityHandle const& handle);

		bool  isValid(EntityHandle const& handle) const;
		bool  isPadingKill(EntityHandle const& handle) const;


		template< class TComponent >
		TComponent* getComponentT(EntityHandle const& handle)
		{
			if( !isValid(handle) )
				return nullptr;
			ComponentType* type = getComponentTypeT<TComponent>();
			if (type == nullptr)
				return nullptr;

			return (TComponent*)getComponentInternal(handle, type);
		}

		template< class TComponent >
		TComponent& getComponentCheckedT(EntityHandle const& handle)
		{
			CHECK(isValid(handle));
			ComponentType* type = getComponentTypeT<TComponent>();
			CHECK(type);
			return *(TComponent*)getComponentInternal(handle, type);
		}

		template< class TComponent >
		int getComponentsT(EntityHandle const& handle, std::vector< TComponent* >& outComponents)
		{
			if( !isValid(handle) )
				return 0;

			int result = 0;
			ComponentType* type = getComponentTypeT<TComponent>();
			if (type)
			{
				for (auto componet : mEntityLists[handle.indexSlot].components)
				{
					if (componet->type == type)
					{
						outComponents.push_back(componet.ptr);
						++result;
					}
				}
			}
			return result;
		}

		int getAllComponents(EntityHandle const& handle, std::vector< EntityComponent* >& outComponents)
		{
			if (!isValid(handle))
				return 0;

			int result = 0;
			for (auto componet : mEntityLists[handle.indexSlot].components)
			{
				outComponents.push_back(componet.ptr);
				++result;
			}
			return result;
		}


		template< class TComponent >
		TComponent* addComponentT(EntityHandle const& handle)
		{
			assert(isValid(handle));
			return (TComponent*)addComponentInternal(handle, getComponentTypeT<TComponent>());
		}

		template< class T >
		bool removeComponentT(EntityHandle const& handle)
		{
			assert(isValid(handle));
			return removeComponentInternal(handle, getComponentTypeT<T>());
		}

		EntityComponent* getComponentInternal(EntityHandle const& handle, ComponentType* type);
		EntityComponent* addComponentInternal(EntityHandle const& handle, ComponentType* type);
		bool removeComponentInternal(EntityHandle const& handle, ComponentType* type);




		struct EEntityFlag
		{
			enum Type
			{
				PendingKill = 0x00000001,
				Used = 0x00000002,

			};
		};

		struct ComponentData
		{
			EntityComponent* ptr;
			ComponentType* type;
		};

		struct EntityData
		{
			uint32 flags;
			uint64 systemMask;
			EntityHandle handle;
			std::vector< ComponentData > components;
		};

		void  cleanupEntity(EntityData& entityData)
		{
			CHECK(entityData.flags & EEntityFlag::PendingKill);
			for (auto const& compentData : entityData.components)
			{
				compentData.type->getPool()->releaseComponent(compentData.ptr);
			}
			entityData.components.clear();
			entityData.components.shrink_to_fit();
			entityData.flags = 0;
		}
		int mIndexSlot = INDEX_NONE;

		std::vector< uint32 > mUsedEntityIndices;
		std::vector< uint32 > mFreeEntityIndices;
		uint32 mNextSerialNumber = 1;


		template< class TFunc >
		void visitListener(TFunc&& func)
		{
			for( auto listener : mEventListers )
			{
				func(listener);
			}
		}

		template< class TComponent >
		ComponentType* getComponentTypeT()
		{
			auto iter = mComponentTypeMap.find(std::type_index(typeid(TComponent)));
			if( iter == mComponentTypeMap.end() )
				return nullptr;
			return iter->second;
		}

		std::unordered_map< std::type_index, ComponentType* > mComponentTypeMap;
		std::vector< EntityData > mEntityLists;
		std::vector< IEntityEventLister* > mEventListers;



		template< class T >
		T* registerSystem(T* system = nullptr)
		{




		}


		void registerEntity(EntityHandle const& handle)
		{



		}

		struct ServiceData
		{
			ISystemSerivce* service;
			bool bManageed;
		};
		std::vector< ISystemSerivce* > mSystems;
	};



	class EntityProxy
	{
	public:
		EntityProxy() :mHandle(),mManagerCached(nullptr) {}
		EntityProxy(EntityHandle handle)
			:mHandle(handle)
		{
			mManagerCached = EntityManager::FromHandle(mHandle);
		}

		bool isValid() const { return mManagerCached && mManagerCached->isValid(mHandle); }

		template< typename TComponent >
		TComponent* getComponentT()
		{
			return getManager().getComponentT<TComponent>(mHandle);
		}

		template< typename TComponent >
		TComponent& getComponentCheckedT()
		{
			return getManager().getComponentCheckedT<TComponent>(mHandle);
		}

		template< typename TComponent >
		int getComponentsT(std::vector<TComponent*>& outComponents)
		{
			return getManager().getComponentsT(mHandle, outComponents);
		}

		int getAllComponents(std::vector<EntityComponent*>& outComponents)
		{
			return getManager().getAllComponents(mHandle, outComponents);
		}


		template< class T >
		T* addComponentT()
		{
			return getManager().addComponentT<T>(mHandle);
		}

		template< class T >
		void removeComponentT()
		{
			getManager().removeComponentT<T>(mHandle);
		}

		EntityManager& getManager() {  return *mManagerCached; }

		EntityHandle   mHandle;
		EntityManager* mManagerCached;
	};
}

#endif // Entity_h__
