#include "HashString.h"
#include "GameConfig.h"
#include "RefCount.h"

#include "RHI/RHICommon.h"
#include "RHI/RHIGlobalResource.h"

#include <unordered_map>


namespace Render
{
	class RenderTargetPool;
	
	class RHITexture2D;

	class TINY_API TextureShowManager
	{
	public:
		void registerTexture(HashString const& name, RHITextureBase* texture);

		void handleShowTexture(char const* texName);

		void registerRenderTarget(RenderTargetPool& renderTargetPool);

		struct TextureHandle : RefCountedObjectT< TextureHandle >
		{
			RHITextureRef   texture;
		};
		using TextureHandleRef = TRefCountPtr< TextureHandle >;
		std::unordered_map< HashString, TextureHandleRef > mTextureMap;

		
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
