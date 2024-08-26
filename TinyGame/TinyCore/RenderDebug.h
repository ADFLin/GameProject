#pragma once
#ifndef RenderDebug_H_450A4626_DBE6_461F_B368_5DDF50CAAEF5
#define RenderDebug_H_450A4626_DBE6_461F_B368_5DDF50CAAEF5

#include "Renderer/SceneDebug.h"
#include "GameConfig.h"

#include "RHI/RHIGlobalResource.h"


namespace Render
{
	class RenderTargetPool;
	
	class TextureShowManager : public ITextureShowManager
	{
	public:
		virtual ~TextureShowManager(){}

		TINY_API void registerTexture(HashString const& name, RHITextureBase* texture);
		TINY_API void handleShowTexture(char const* texName);
		TINY_API void registerRenderTarget(RenderTargetPool& renderTargetPool);

		std::unordered_map< HashString, TextureHandleRef > mTextureMap;

		virtual std::unordered_map< HashString, TextureHandleRef > const& getTextureMap()
		{
			return mTextureMap;
		}
		TINY_API void releaseRHI();
	};

	class GlobalTextureShowManager : public TextureShowManager
		                           , public IGlobalRenderResource
	{
	public:
		TINY_API GlobalTextureShowManager();

		TINY_API void restoreRHI() override;
		TINY_API void releaseRHI() override;
	};

	extern TINY_API GlobalTextureShowManager GTextureShowManager;
}

#endif // RenderDebug_H_450A4626_DBE6_461F_B368_5DDF50CAAEF5
