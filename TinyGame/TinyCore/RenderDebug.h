#pragma once
#ifndef RenderDebug_H_450A4626_DBE6_461F_B368_5DDF50CAAEF5
#define RenderDebug_H_450A4626_DBE6_461F_B368_5DDF50CAAEF5

#include "Renderer/SceneDebug.h"
#include "GameConfig.h"

#include "RHI/RHIGlobalResource.h"


namespace Render
{
	class RenderTargetPool;
	
	class TINY_API TextureShowManager : public ITextureShowManager
	{
	public:
		void registerTexture(HashString const& name, RHITextureBase* texture);

		void handleShowTexture(char const* texName);

		void registerRenderTarget(RenderTargetPool& renderTargetPool);

		std::unordered_map< HashString, TextureHandleRef > mTextureMap;

		virtual std::unordered_map< HashString, TextureHandleRef > const& getTextureMap()
		{
			return mTextureMap;
		}
		void releaseRHI();
	};

	class TINY_API GlobalTextureShowManager : public TextureShowManager
		                                    , public IGlobalRenderResource
	{
	public:
		GlobalTextureShowManager();

		void restoreRHI() override;
		void releaseRHI() override;
	};

	extern TINY_API GlobalTextureShowManager GTextureShowManager;
}

#endif // RenderDebug_H_450A4626_DBE6_461F_B368_5DDF50CAAEF5
