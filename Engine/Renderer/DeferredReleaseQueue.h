#pragma once
#ifndef DeferredReleaseQueue_h__
#define DeferredReleaseQueue_h__

#include "RenderThread.h"
#include "DataStructure/CycleQueue.h"
#include "RHI/RHICommon.h"
#include "DataStructure/Array.h"

// Simple thread-safe queue for resource cleanup
class DeferredReleaseQueue
{
public:
	static DeferredReleaseQueue& Get()
	{
		static DeferredReleaseQueue Instance;
		return Instance;
	}

	void enqueue(Render::RHIResource* resource)
	{
		// Called from Any Thread (usually Game Thread)
		Mutex::Locker lock(mMutex);
		mPendingResources.push_back(resource);
	}

	void process()
	{
		// Called from Render Thread
		TArray<Render::RHIResource*> processing;
		{
			Mutex::Locker lock(mMutex);
			if (mPendingResources.empty())
				return;
			processing.swap(mPendingResources);
		}

		for (auto resource : processing)
		{
			if (resource)
			{
				resource->releaseResource();
				delete resource;
			}
		}
	}

	void setActive(bool isActive)
	{
		Mutex::Locker lock(mMutex);
		mIsActive = isActive;
	}

	bool isActive()
	{
		Mutex::Locker lock(mMutex);
		return mIsActive;
	}

private:
	Mutex mMutex;
	TArray<Render::RHIResource*> mPendingResources;
	bool mIsActive = false;
};

#endif // DeferredReleaseQueue_h__
