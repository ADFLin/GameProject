#ifndef SceneDebug_h__
#define SceneDebug_h__

#include "RHI/RHICommon.h"

#include "HashString.h"
#include "RefCount.h"
#include <unordered_map>


namespace Render
{
	class ITextureShowManager
	{
	public:

		virtual ~ITextureShowManager() = default;
		struct TextureHandle : RefCountedObjectT< TextureHandle >
		{
			RHITextureRef   texture;
		};
		using TextureHandleRef = TRefCountPtr< TextureHandle >;
		virtual std::unordered_map< HashString, TextureHandleRef > const& getTextureMap() = 0;
	};
}

#endif // SceneDebug_h__