#include "HashString.h"
#include "GameConfig.h"
#include "RefCount.h"

#include "RHI/RHICommon.h"

#include <unordered_map>

namespace Render
{
	class RenderTargetPool;
	
	class RHITexture2D;

	class TINY_API TextureShowManager
	{
	public:
		void registerTexture(HashString const& name, RHITexture2D* texture);

		void handleShowTexture();

		void registerRenderTarget(RenderTargetPool& renderTargetPool);

		struct TextureHandle : RefCountedObjectT< TextureHandle >
		{
			RHITexture2DRef texture;
		};
		using TextureHandleRef = TRefCountPtr< TextureHandle >;
		std::unordered_map< HashString, TextureHandleRef > mTextureMap;

		void releaseRHI();
	};
}
