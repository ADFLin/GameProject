#ifndef Entity_h__
#define Entity_h__

#include "Core/TypeHash.h"
#include "MarcoCommon.h"

#include <vector>
#include <unordered_map>
#include <typeindex>
#include <algorithm>
#include <cassert>


namespace ECS
{

	class ComponentType;
	class EntityHandle;

	using EnitiyComponent = void;

	class IComponentPool
	{
	public:
		virtual void releaseComponent(EnitiyComponent* component) = 0;
		virtual EnitiyComponent* fetchComponent() = 0;
	};


	class ComponentType
	{
	public:
		virtual IComponentPool* getPool() = 0;
		virtual void registerComponent(EntityHandle const& handle, EnitiyComponent* component) {}
		virtual void unregisterComponent(EntityHandle const& handle, EnitiyComponent* component) {}
	};

	class EntityHandle
	{
	public:
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
				uint32 indexManager : 8;
				uint32 indexSlot    : 24;
				uint32 serialNumber : 32;
			};
		};

		EntityHandle() :mPackedValue(0) {}


	private:
		EntityHandle(uint32 inIndexSlot, uint32 inSerialNumber, uint32 inIndexManager)
		{
			inIndexManager = inIndexManager;
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
		virtual void releaseComponent(EnitiyComponent* component)
		{
			delete component;
		}
		virtual EnitiyComponent* fetchComponent()
		{
			return new T();
		}
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
		void registerToList();
		void unregisterFromList();

		static EntityManager* FromHandle(EntityHandle const& handle);
		static EntityManager& FromHandleChecked(EntityHandle const& handle);

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

		template< class TComponent >
		void registerDefaultComponentTypeT()
		{
			static TDefaultComponentType< TComponent > sComponentTypeInstance;
			mComponentTypeMap.emplace(std::type_index( typeid(TComponent) ), &sComponentTypeInstance);
		}

		template< class TComponent , typename TComponentType >
		void registerComponentTypeT()
		{


		}

		template< class TComponent , typename TFunc >
		void visitComponent(TFunc&& func)
		{
			ComponentType* type = getComponentTypeT<TComponent>();
			for (auto indexSlot : mUsedSlotIndices)
			{
				EntityData& entityData = mEntityLists[indexSlot];
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
			return getComponentInternal(handle, type);
		}

		template< class TComponent >
		int getAllComponentsT(EntityHandle const& handle, std::vector< TComponent* >& outComponents)
		{
			if( !isValid(handle) )
				return 0;
			ComponentType* type = getComponentTypeT<TComponent>();
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

		template< class T >
		T* addComponentT(EntityHandle const& handle)
		{
			assert(isValid(handle));
			return (T*)addComponentInternal(handle, getComponentTypeT<T>());
		}

		template< class T >
		bool removeComponentT(EntityHandle const& handle)
		{
			assert(isValid(handle));
			return removeComponentInternal(handle, getComponentTypeT<T>());
		}

		EnitiyComponent* getComponentInternal(EntityHandle const& handle, ComponentType* type);
		int getAllComponentInternal(EntityHandle const& handle, ComponentType* type, std::vector< EnitiyComponent* >& outComponents);
		EnitiyComponent* addComponentInternal(EntityHandle const& handle, ComponentType* type);
		bool removeComponentInternal(EntityHandle const& handle, ComponentType* type);




		struct EEntityFlag
		{
			enum Type
			{
				PaddingKill = 0x00000001,
				Used = 0x00000002,

			};
		};

		struct ComponentData
		{
			void* ptr;
			ComponentType* type;
		};

		struct EntityData
		{
			uint32 flags;
			EntityHandle handle;
			std::vector< ComponentData > components;
		};

		int mIndexSlot = INDEX_NONE;

		std::vector< uint32 > mUsedSlotIndices;
		std::vector< uint32 > mFreeSlotIndices;
		uint32 mNextSerialNumber = 1;


		template< class TFunc >
		void notifyEvent(TFunc&& func)
		{
			for( auto listener : mEventListers )
			{
				func(listener);
			}
		}

		template< class T >
		ComponentType* getComponentTypeT()
		{
			auto iter = mComponentTypeMap.find(std::type_index(typeid(T)));
			if( iter == mComponentTypeMap.end() )
				return nullptr;
			return iter->second;
		}

		std::unordered_map< std::type_index, ComponentType* > mComponentTypeMap;
		std::vector< EntityData > mEntityLists;
		std::vector< IEntityEventLister* > mEventListers;
	};



	class EntityProxy
	{
	public:
		EntityProxy() :mHandle() {}
		EntityProxy(EntityHandle handle)
			:mHandle(handle)
		{

		}

		bool isValid() const { return EntityManager::IsValid(mHandle); }

		template< class T >
		T* getComponentT()
		{
			return getManager().getComponentT<T>(mHandle);
		}
		template< class T >
		int getAllComponentsT(std::vector< T* >& outComponents)
		{
			return getManager().getAllComponentsT(mHandle, outComponents);
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

		EntityManager& getManager() {  return EntityManager::FromHandleChecked(mHandle); }
		EntityHandle mHandle;
	};


	class ISystemSerivce
	{
	public:
		virtual void tickSystem(float deltaTime) {}
		virtual void notiyEntityRegister(EntityHandle const& handle) {}
		virtual void notiyEntityUnregister(EntityHandle const& handle) {}
	};


	struct SystemServiceType
	{


	};
	class SystemAdmin
	{
	public:
		template< class T >
		T* getSystem();


		template< class T >
		void registerSystem(T* system = nullptr)
		{




		}


		void registerEntity(EntityHandle const& handle)
		{



		}

		static SystemAdmin& Get();
	};

}

#endif // Entity_h__
