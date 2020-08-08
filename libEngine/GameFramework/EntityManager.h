#ifndef Entity_h__
#define Entity_h__

#include "Core/TypeHash.h"

#include <vector>
#include <unordered_map>
#include <typeindex>
#include <algorithm>
#include <cassert>

namespace ECS
{

	class ComponentType;
	class EntityHandle;

	class EnitiyComponent
	{
	public:
		virtual void registerComponent(EntityHandle const& handle) {}
		virtual void unregisterComponent(EntityHandle const& handle) {}
		ComponentType* type;
	};

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
	};

	class EntityHandle
	{
	public:
		bool operator == (EntityHandle const& rhs) const
		{
			return mPackedValue == rhs.mPackedValue;
		}

		uint32 getTypeHash() const
		{
			return HashValue(mPackedValue);
		}
		union
		{
			uint32 mPackedValue;
			struct
			{
				uint32 indexSlot : 12;
				uint32 serialNumber : 20;
			};
		};

		EntityHandle() :mPackedValue(0) {}


	private:
		EntityHandle(uint32 inIndexSlot, uint32 inSerialNumber)
		{
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
		static EntityManager& Get()
		{
			static EntityManager sInstance;
			return sInstance;
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

		template< class T >
		void registerDefaultComponentTypeT()
		{
			static TDefaultComponentType< T > sComponentTypeInstance;
			mComponentTypeMap.emplace(std::type_index( typeid(T) ), &sComponentTypeInstance);
		}
		template< class T >
		void registerComponentTypeT()
		{


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
		bool  isVaild(EntityHandle const& handle) const;
		bool  isPadingKill(EntityHandle const& handle) const;


		template< class T >
		T* getComponentT(EntityHandle const& handle)
		{
			if( !isVaild(handle) )
				return nullptr;
			ComponentType* type = getComponentTypeT<T>();
			return getComponentInternal(handle, type);
		}

		template< class T >
		int getAllComponentsT(EntityHandle const& handle, std::vector< T* >& outComponents)
		{
			if( !isVaild(handle) )
				return 0;
			ComponentType* type = getComponentTypeT<T>();
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
			assert(isVaild(handle));
			return (T*)addComponentInternal(handle, getComponentTypeT<T>());
		}

		template< class T >
		bool removeComponentT(EntityHandle const& handle)
		{
			assert(isVaild(handle));
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

		struct EntityData
		{
			uint32 flags;
			uint32 serialNumber;
			std::vector< EnitiyComponent* > components;
		};

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

		template< class T >
		T* getComponentT()
		{
			return EntityManager::Get().getComponentT<T>(mHandle);
		}
		template< class T >
		int getAllComponentsT(std::vector< T* >& outComponents)
		{
			return EntityManager::Get().getAllComponentsT(mHandle, outComponents);
		}
		template< class T >
		T* addComponentT()
		{
			return EntityManager::Get().addComponentT<T>(mHandle);
		}
		template< class T >
		void removeComponentT()
		{
			EntityManager::Get().removeComponentT<T>(mHandle);
		}

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
