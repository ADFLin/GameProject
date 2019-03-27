#pragma once
#ifndef RHIStaticStateObject_H_3909FF48_83F9_4C22_AEBB_258FB6C78C29
#define RHIStaticStateObject_H_3909FF48_83F9_4C22_AEBB_258FB6C78C29

#include "RHICommon.h"
#include "CoreShare.h"

namespace Render
{
	CORE_API extern RHITexture2DRef    GDefaultMaterialTexture2D;
	CORE_API extern RHITexture2DRef    GWhiteTexture2D;
	CORE_API extern RHITexture2DRef    GBlackTexture2D;
	CORE_API extern RHITextureCubeRef  GWhiteTextureCube;
	CORE_API extern RHITextureCubeRef  GBlackTextureCube;

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
		bool Initialize(RHIResourceType* resource)
		{
			mResource = resource;
			return isValid();
		}
		virtual void restoreRHI() override {}
		virtual void releaseRHI() override { mResource.release(); }
	
		RHIResourceType* getRHI() { return mResource; }
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


}//namespace Render


#endif // RHIStaticStateObject_H_3909FF48_83F9_4C22_AEBB_258FB6C78C29
