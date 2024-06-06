#pragma once
#ifndef RHIUtility_H_8A433929_9272_4335_BE79_FFFC6B4429EE
#define RHIUtility_H_8A433929_9272_4335_BE79_FFFC6B4429EE

#include "RHICommon.h"

class DataCacheInterface;
struct ImageData;

namespace Render
{
	struct TextureLoadOption
	{
		bool   bHDR = false;
		bool   bSRGB = false;
		bool   bUseHalf = true;
		bool   bFlipV = false;
		bool   bAutoMipMap = false;
		int    numMipLevel = 0;
		TextureCreationFlags creationFlags = TCF_DefalutValue;


		ETexture::Format getFormat(int numComponent) const;

		bool isConvertFloatToHalf() const
		{
			return bHDR && (bUseHalf && isNeedConvertFloatToHalf());
		}
		bool isRGBTextureSupported() const;
		bool isNeedConvertFloatToHalf() const;

		TextureLoadOption& FlipV(bool value = true) { bFlipV = value; return *this; }
		TextureLoadOption& HDR(bool value = true) { bHDR = value; return *this; }
		TextureLoadOption& SRGB(bool value = true) { bSRGB = value; return *this; }
		TextureLoadOption& AutoMipMap(bool value = true) { bAutoMipMap = value;  return *this; }
		TextureLoadOption& MipLevel(int value = 0) { numMipLevel = value;  return *this; }
	};


	class RHIUtility
	{
	public:
		static RHITexture2D* LoadTexture2DFromFile(char const* path, TextureLoadOption const& option = TextureLoadOption());
		static RHITexture2D* LoadTexture2DFromFile(DataCacheInterface& dataCache, char const* path, TextureLoadOption const& option);
		static RHITextureCube* LoadTextureCubeFromFile(char const* paths[], TextureLoadOption const& option = TextureLoadOption());
		static RHITexture2D* CreateTexture2D(ImageData const& imageData, TextureLoadOption const& option);
	};

}
#endif // RHIUtility_H_8A433929_9272_4335_BE79_FFFC6B4429EE
