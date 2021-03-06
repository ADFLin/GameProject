#include "LazyObject.h"

#include <cassert>

LazyObjectPtrBase::LazyObjectPtrBase()
{
	mResolveSlot = INDEX_NONE;
	mPtr = nullptr;
}

LazyObjectPtrBase::~LazyObjectPtrBase()
{
	if( mResolveSlot != INDEX_NONE )
	{
		LazyObjectManager::Get().unregisterObject(*this);
	}
}

void LazyObjectManager::registerResolveCallback(LazyObjectPtrBase& object, ResolveCallback callback)
{
	Mutex::Locker locker(mMutex);

	auto iter = mObjectNodeMap.find(&object);

	if( iter != mObjectNodeMap.end() )
	{
		ObjectResolveInfo& info = *iter->second;
		info.callback = callback;
	}
}

void LazyObjectManager::unregisterObject(LazyObjectPtrBase& object)
{
	Mutex::Locker locker(mMutex);

	assert(object.mResolveSlot != INDEX_NONE);
	auto iterNode = mObjectNodeMap.find(&object);
	assert(iterNode != mObjectNodeMap.end());
	mResolveMap[object.mResolveSlot].erase(iterNode->second);
	mObjectNodeMap.erase(iterNode);
}

void LazyObjectManager::registerObject(uint64& id, LazyObjectPtrBase& object)
{
	Mutex::Locker locker(mMutex);

	if( id == -1 )
	{
		id = mCurId;
		++mCurId;
	}
	if( object.mResolveSlot != INDEX_NONE)
	{
		auto iterNode = mObjectNodeMap.find(&object);
		mResolveMap[object.mResolveSlot].erase(iterNode->second);
	}
	auto& resolveList = mResolveMap[id];
	ObjectResolveInfo info;
	info.object = &object;
	resolveList.push_front(info);
	mObjectNodeMap[&object] = resolveList.begin();
	object.mResolveSlot = id;
}

void LazyObjectManager::resolveObject(uint64 id, void* object)
{
	Mutex::Locker locker(mMutex);

	auto iter = mResolveMap.find(id);
	if( iter == mResolveMap.end() )
		return;

	for(  ObjectResolveInfo& info : iter->second )
	{
		LazyObjectPtrBase* objectPtr = info.object;
		(*objectPtr).mPtr = object;
		(*objectPtr).mResolveSlot = INDEX_NONE;
		if( info.callback )
			info.callback(*objectPtr);

		auto iterRemove = mObjectNodeMap.find(objectPtr);
		if( iterRemove != mObjectNodeMap.end() )
		{
			mObjectNodeMap.erase(iterRemove);
		}
	}
	mResolveMap.erase(iter);
}
