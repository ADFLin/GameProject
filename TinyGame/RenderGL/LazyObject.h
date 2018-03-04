#pragma once
#ifndef LazyObject_H_F54F230F_D87C_4DC1_8B4C_F91175F696A9
#define LazyObject_H_F54F230F_D87C_4DC1_8B4C_F91175F696A9

#include "Singleton.h"
#include "Core/IntegerType.h"

#include <unordered_map>
#include <typeindex>
#include <functional>

class LazyObjectPtrBase
{
public:
	LazyObjectPtrBase();
	~LazyObjectPtrBase();
	uint64 mId;
	void*  mPtr;

	friend class LazyObjectManager;
};

typedef std::function< void(LazyObjectPtrBase&) > ResolveCallback;
class LazyObjectManager : public SingletonT< LazyObjectManager >
{
public:
	LazyObjectManager()
	{
		mCurId = 0;
	}

	void registerResolveCallback(LazyObjectPtrBase& object, ResolveCallback callback);

	template< class T >
	void registerDefalutObject(T* object)
	{
		mDefaultMap[typeid(T)] = object;
	}
	template< class T >
	T* getDefaultObject()
	{
		return static_cast<T*>(mDefaultMap[typeid(T)]);
	}
	void unregisterObject(LazyObjectPtrBase& object);
	void registerObject(uint64& id, LazyObjectPtrBase& object);
	void resolveObject(uint64 id, void* object);

	uint64 mCurId;

	struct ObjectResolveInfo
	{
		LazyObjectPtrBase* object;
		ResolveCallback callback;
	};
	typedef std::list< ObjectResolveInfo > ObjectResolveList;
	std::unordered_map< uint64, ObjectResolveList > mResolveMap;
	std::unordered_map< LazyObjectPtrBase*, ObjectResolveList::iterator > mObjectNodeMap;
	std::unordered_map< std::type_index, void* > mDefaultMap;

};

template< class T >
class TLazyObjectPtr;

template< class T >
class TLazyObjectGuid
{
public:
	TLazyObjectGuid()
	{
		mObject = nullptr;
		mId = -1;
	}
	TLazyObjectGuid(T* object)
	{
		mObject = object;
		mId = -1;
	}

	void resolveOrRegister(TLazyObjectPtr<T>& object) const
	{
		if( mObject )
		{
			object.mPtr = mObject;
			return;
		}
		LazyObjectManager::Get().registerObject(mId, object);
		object.mPtr = LazyObjectManager::Get().getDefaultObject< T >();
	}
	TLazyObjectGuid& operator = (T* obj)
	{
		mObject = obj;
		if( mId != -1 )
		{
			LazyObjectManager::Get().resolveObject(mId, mObject);
			mId = -1;
		}
		return *this;
	}
	operator T& () { return *get(); }
	operator T* () { return get(); }
	T* get()
	{
		if( mObject )
			return mObject;
		return LazyObjectManager::Get().getDefaultObject< T >();
	}

	mutable uint64 mId;
	T*     mObject;
};


//#TODO : Rename
template< class T >
class TLazyObjectPtr : public LazyObjectPtrBase
{
public:
	TLazyObjectPtr()
		:LazyObjectPtrBase()
	{

	}
	TLazyObjectPtr(T* obj)
	{
		mPtr = obj;
		mId = -1;
	}
	TLazyObjectPtr(TLazyObjectGuid< T > const& objGuid)
	{
		objGuid.resolveOrRegister(*this);
	}
	TLazyObjectPtr& operator = (TLazyObjectGuid< T >& objGuid)
	{
		objGuid.resolveOrRegister(*this);
		return *this;
	}
	operator T* () { return static_cast<T*>(mPtr); }
	T* operator->()
	{
		return static_cast<T*>(mPtr);
	}
	T const* operator->() const
	{
		return static_cast<T*>(mPtr);
	}
};

#endif // LazyObject_H_F54F230F_D87C_4DC1_8B4C_F91175F696A9
