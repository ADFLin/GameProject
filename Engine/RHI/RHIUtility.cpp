#include "RHIUtility.h"
#include "RHICommand.h"

#include "DataCacheInterface.h"
#include "Serialize/DataStream.h"
#include "Image/ImageData.h"
#include "Core/HalfFlot.h"


namespace Render
{

	int CalcMipmapLevel(int size)
	{
		int result = 0;
		int value = 1;
		while (value <= size) { value *= 2; ++result; }
		return result;
	}

	ImageLoadOption ToImageLoadOption(TextureLoadOption const& option)
	{
		ImageLoadOption result;
		result.FlipV(option.bFlipV).HDR(option.bHDR).UpThreeComponentToFour(!option.isRGBTextureSupported());
		return result;
	}
	RHITexture2D* RHIUtility::LoadTexture2DFromFile(DataCacheInterface& dataCache, char const* path, TextureLoadOption const& option)
	{
		bool bConvToHalf = option.isConvertFloatToHalf();

		DataCacheKey cacheKey;
		cacheKey.typeName = "TEXTURE";
		cacheKey.version = "8AE15F61-E1CF-4639-B7D8-409CF17933F0";
		cacheKey.keySuffix.add(path, option.bHDR, option.bFlipV, option.bSRGB, option.creationFlags, option.numMipLevel, option.bAutoMipMap,
			option.isRGBTextureSupported(), bConvToHalf);

		void* pData;
		ImageData imageData;
		TArray< uint8 > cachedImageData;
		auto LoadCache = [&imageData, &cachedImageData](IStreamSerializer& serializer)-> bool
		{
			serializer >> imageData.width;
			serializer >> imageData.height;
			serializer >> imageData.numComponent;

			int dataSize;
			serializer >> dataSize;

			cachedImageData.resize(dataSize);
			serializer.read(cachedImageData.data(), dataSize);
			return true;
		};

		if (!dataCache.loadDelegate(cacheKey, LoadCache))
		{
			if (!imageData.load(path, ToImageLoadOption(option)))
				return false;

			pData = imageData.data;

			int dataSize = imageData.dataSize;

			if (bConvToHalf)
			{
				int num = imageData.width * imageData.height * imageData.numComponent;
				cachedImageData.resize(sizeof(HalfFloat) * num);

				HalfFloat* pHalf = (HalfFloat*)cachedImageData.data();
				float* pFloat = (float*)pData;
				for (int i = 0; i < num; ++i)
				{
					*pHalf = *pFloat;
					++pHalf;
					++pFloat;
				}
				pData = cachedImageData.data();
				dataSize /= 2;
			}

			dataCache.saveDelegate(cacheKey, [&imageData, pData, dataSize](IStreamSerializer& serializer)-> bool
			{
				serializer << imageData.width;
				serializer << imageData.height;
				serializer << imageData.numComponent;
				serializer << dataSize;
				serializer.write(pData, dataSize);
				return true;
			});
		}
		else
		{
			pData = cachedImageData.data();
		}

		TextureCreationFlags flags = option.creationFlags;
		if (bConvToHalf)
		{
			flags |= TCF_HalfData;
		}


		auto format = option.getFormat(imageData.numComponent);

		int numMipLevel = option.numMipLevel;
		if (option.bAutoMipMap)
		{
			numMipLevel = CalcMipmapLevel(Math::Max(imageData.width, imageData.height));
		}

		if (numMipLevel > 1)
		{
			flags |= TCF_GenerateMips;
		}


		return RHICreateTexture2D(format, imageData.width, imageData.height, numMipLevel, 1, flags, pData, 1);
	}

	RHITexture2D* RHIUtility::LoadTexture2DFromFile(char const* path, TextureLoadOption const& option)
	{
		ImageData imageData;
		if (!imageData.load(path, ToImageLoadOption(option)))
			return false;

		return CreateTexture2D(imageData, option);
	}

	RHITextureCube* RHIUtility::LoadTextureCubeFromFile(char const* paths[], TextureLoadOption const& option)
	{
		bool bConvToHalf = option.isConvertFloatToHalf();
		ImageData imageDatas[ETexture::FaceCount];

		void* data[ETexture::FaceCount];
		for (int i = 0; i < ETexture::FaceCount; ++i)
		{
			if (!imageDatas[i].load(paths[i], ToImageLoadOption(option)))
				return false;

			data[i] = imageDatas[i].data;
		}

		TextureCreationFlags flags = option.creationFlags;
		if (option.bAutoMipMap)
		{
			option.numMipLevel;
		}

		int numMipLevel = option.numMipLevel;
		if (option.bAutoMipMap)
		{
			numMipLevel = CalcMipmapLevel(imageDatas[0].width);
		}
		if (numMipLevel > 1)
		{
			flags |= TCF_GenerateMips;
		}

		return RHICreateTextureCube(option.getFormat(imageDatas[0].numComponent), imageDatas[0].width, numMipLevel, flags, data);
	}

	RHITexture2D* RHIUtility::CreateTexture2D(ImageData const& imageData, TextureLoadOption const& option)
	{
		auto format = option.getFormat(imageData.numComponent);

		bool bConvToHalf = option.isConvertFloatToHalf();


		TextureCreationFlags flags = option.creationFlags;

		int numMipLevel = option.numMipLevel;
		if (option.bAutoMipMap)
		{
			numMipLevel = CalcMipmapLevel(Math::Max(imageData.width, imageData.height));
		}

		if (option.numMipLevel > 1)
		{
			flags |= TCF_GenerateMips;
		}

		if (bConvToHalf)
		{
			TArray< HalfFloat > halfData(imageData.width * imageData.height * imageData.numComponent);
			float* pFloat = (float*)imageData.data;
			for (int i = 0; i < halfData.size(); ++i)
			{
				halfData[i] = *pFloat;
				++pFloat;
			}
			return RHICreateTexture2D(format, imageData.width, imageData.height, numMipLevel, 1, flags | TCF_HalfData, halfData.data(), 1);

		}
		else
		{
			return RHICreateTexture2D(format, imageData.width, imageData.height, numMipLevel, 1, flags, imageData.data, 1);
		}
	}

	ETexture::Format TextureLoadOption::getFormat(int numComponent) const
	{
#define FORCE_USE_FLOAT_TYPE 0

		if (bHDR)
		{
			if (bUseHalf)
			{
				switch (numComponent)
				{
				case 1: return ETexture::R16F;
				case 2: return ETexture::RG16F;
				case 3: return ETexture::RGB16F;
				case 4: return ETexture::RGBA16F;
				}
			}
			else
			{
				switch (numComponent)
				{
				case 1: return ETexture::R32F;
				case 2: return ETexture::RG32F;
				case 3: return ETexture::RGB32F;
				case 4: return ETexture::RGBA32F;
				}
			}

		}
		else
		{
			//#TODO
			switch (numComponent)
			{
			case 1: return ETexture::R8;
			case 3: return bSRGB ? ETexture::SRGB : ETexture::RGB8;
			case 4: return bSRGB ? ETexture::SRGBA : ETexture::RGBA8;
			}
		}
		return ETexture::R8;
	}


	bool TextureLoadOption::isRGBTextureSupported() const
	{
		auto systemName = GRHISystem->getName();
		if (systemName == RHISystemName::OpenGL ||
			systemName == RHISystemName::Vulkan)
			return true;

		if (systemName == RHISystemName::D3D11 ||
			systemName == RHISystemName::D3D12)
		{
			if (bHDR)
			{
				//no rgb16f format and rgb32f only can use of point sampler in pixel shader
				return false;
			}
			if (bSRGB)
			{
				return false;
			}

			return false;
		}
		return true;
	}

	bool TextureLoadOption::isNeedConvertFloatToHalf() const
	{
		auto systemName = GRHISystem->getName();
		if (systemName == RHISystemName::D3D11 ||
			systemName == RHISystemName::D3D12 ||
			systemName == RHISystemName::Vulkan)
			return true;

		return false;
	}



}
