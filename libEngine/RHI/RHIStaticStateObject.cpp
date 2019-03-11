#include "RHIStaticStateObject.h"


#if CORE_SHARE_CODE

StaticRHIResourceObjectBase* GHead = nullptr;
StaticRHIResourceObjectBase::StaticRHIResourceObjectBase()
{
	mNext = GHead;
	GHead = this;
}

StaticRHIResourceObjectBase::~StaticRHIResourceObjectBase()
{

}

void StaticRHIResourceObjectBase::ReleaseAllResource()
{
	StaticRHIResourceObjectBase* cur = GHead;

	while( cur )
	{
		cur->releaseRHI();
		cur = cur->mNext;
	}
}

void StaticRHIResourceObjectBase::RestoreAllResource()
{
	StaticRHIResourceObjectBase* cur = GHead;

	while( cur )
	{
		cur->restoreRHI();
		cur = cur->mNext;
	}
}

#endif