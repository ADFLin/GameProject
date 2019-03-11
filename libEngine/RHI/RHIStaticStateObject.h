#pragma once
#ifndef RHIStaticStateObject_H_3909FF48_83F9_4C22_AEBB_258FB6C78C29
#define RHIStaticStateObject_H_3909FF48_83F9_4C22_AEBB_258FB6C78C29

#include "RHICommon.h"
#include "CoreShare.h"


class StaticRHIResourceObjectBase
{
public:
	CORE_API StaticRHIResourceObjectBase();
	CORE_API ~StaticRHIResourceObjectBase();
	virtual void restoreRHI() = 0;
	virtual void releaseRHI() = 0;
	
	StaticRHIResourceObjectBase* mNext;

	CORE_API static void ReleaseAllResource();
	CORE_API static void RestoreAllResource();
};

template< class ThisClass , class RHIResource >
class StaticRHIStateT
{
public:
	static RHIResource& GetRHI()
	{
		static TStaticRHIResourceObject< RHIResource >* sObject;
		if( sObject == nullptr )
		{
			sObject = new TStaticRHIResourceObject<RHIResource>();
			sObject->restoreRHI();
		}
		return sObject->getRHI();
	}

private:
	template< class RHIResource >
	class TStaticRHIResourceObject : public StaticRHIResourceObjectBase
	{

	public:

		RHIResource& getRHI() { return *mResource; }

		virtual void restoreRHI()
		{
			mResource = ThisClass::CreateRHI();
		}
		virtual void releaseRHI()
		{
			mResource.release();
		}

		TRefCountPtr< RHIResource > mResource;
	};


};


#endif // RHIStaticStateObject_H_3909FF48_83F9_4C22_AEBB_258FB6C78C29
