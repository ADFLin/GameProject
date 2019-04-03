#pragma once
#ifndef RHIStaticStateObject_H_3909FF48_83F9_4C22_AEBB_258FB6C78C29
#define RHIStaticStateObject_H_3909FF48_83F9_4C22_AEBB_258FB6C78C29

#include "RHICommon.h"
#include "CoreShare.h"

namespace Render
{


	class ShaderCompileOption;
	class MaterialShaderProgram;

	CORE_API extern class MaterialMaster* GDefalutMaterial;
	CORE_API extern class MaterialShaderProgram GSimpleBasePass;
	

	CORE_API bool InitGlobalRHIResource();
	CORE_API void ReleaseGlobalRHIResource();

	
	class GlobalRHIResourceBase
	{
	public:
		CORE_API GlobalRHIResourceBase();
		virtual ~GlobalRHIResourceBase() {}
		virtual void restoreRHI() = 0;
		virtual void releaseRHI() = 0;

		GlobalRHIResourceBase* mNext;

		CORE_API static void ReleaseAllResource();
		CORE_API static void RestoreAllResource();
	};
	
	template< class RHIResourceType >
	class TGlobalRHIResource : public GlobalRHIResourceBase
	{
	public:
		bool isValid() const { return mResource.isValid(); }
		bool initialize(RHIResourceType* resource)
		{
			mResource = resource;
			return mResource.isValid();
		}
		virtual void restoreRHI() override {}
		virtual void releaseRHI() override { mResource.release(); }

		RHIResourceType* getResource() { return mResource; }

		operator RHIResourceType* (void) { return mResource.get(); }
		operator RHIResourceType* (void) const { return mResource.get(); }
		RHIResourceType*       operator->() { return mResource.get(); }
		RHIResourceType const* operator->() const { return mResource.get(); }
		RHIResourceType&       operator *(void) { return *mResource.get(); }
		RHIResourceType const& operator *(void) const { return *mResource.get(); }

		TRefCountPtr< RHIResourceType > mResource;
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
		class TStaticRHIResourceObject : public GlobalRHIResourceBase
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

	CORE_API extern TGlobalRHIResource<RHITexture2D>    GDefaultMaterialTexture2D;
	CORE_API extern TGlobalRHIResource<RHITexture1D>    GWhiteTexture1D;
	CORE_API extern TGlobalRHIResource<RHITexture1D>    GBlackTexture1D;
	CORE_API extern TGlobalRHIResource<RHITexture2D>    GWhiteTexture2D;
	CORE_API extern TGlobalRHIResource<RHITexture2D>    GBlackTexture2D;
	CORE_API extern TGlobalRHIResource<RHITexture3D>    GWhiteTexture3D;
	CORE_API extern TGlobalRHIResource<RHITexture3D>    GBlackTexture3D;
	CORE_API extern TGlobalRHIResource<RHITextureCube>  GWhiteTextureCube;
	CORE_API extern TGlobalRHIResource<RHITextureCube>  GBlackTextureCube;

}//namespace Render


#endif // RHIStaticStateObject_H_3909FF48_83F9_4C22_AEBB_258FB6C78C29
