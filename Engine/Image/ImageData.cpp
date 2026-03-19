#include "ImageData.h"

#include "FileSystem.h"
#include "DataStructure/Array.h"
#include "Core/IntegerType.h"
#include "Core/Memory.h"
#include "CoreShare.h"


ImageData::ImageData()
{
	data = nullptr;
}

ImageData::~ImageData()
{
	if( data )
	{
		FMemory::Free(data);
	}
}

#if CORE_SHARE_CODE

#define STBI_MALLOC(sz)           FMemory::Alloc(sz)
#define STBI_REALLOC(p,newsz)     FMemory::Realloc(p,newsz)
#define STBI_FREE(p)              FMemory::Free(p)
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#include "webp/decode.h"

#pragma comment (lib, "../OtherLib/webp/lib/libwebp.lib")

struct ScopedSetFlipVertically
{
	ScopedSetFlipVertically(bool bFlipV)
	{
		stbi_set_flip_vertically_on_load(bFlipV);
	}

	~ScopedSetFlipVertically()
	{
		stbi_set_flip_vertically_on_load(false);
	}
};

bool ImageData::load(char const* path, ImageLoadOption const& option)
{
	TArray<uint8> buffer;
	if (!FFileUtility::LoadToBuffer(path, buffer))
		return false;

	return loadFromMemory(buffer.data(), (int)buffer.size(), option);
}

bool ImageData::loadFromMemory(void* inData, int size, ImageLoadOption const& option)
{
	uint8 const* dataPtr = (uint8 const*)inData;
	if (size >= 12 && memcmp(dataPtr, "RIFF", 4) == 0 && memcmp(dataPtr + 8, "WEBP", 4) == 0)
	{
		WebPBitstreamFeatures features;
		if (WebPGetFeatures(dataPtr, size, &features) == VP8_STATUS_OK)
		{
			width = features.width;
			height = features.height;
			if (option.bUpThreeComponentToFour || features.has_alpha)
			{
				numComponent = 4;
				dataSize = (uint64)width * height * 4;
				data = FMemory::Alloc(dataSize);
				if (!WebPDecodeRGBAInto(dataPtr, size, (uint8*)data, dataSize, width * 4))
				{
					FMemory::Free(data);
					data = nullptr;
					return false;
				}
			}
			else
			{
				numComponent = 3;
				dataSize = (uint64)width * height * 3;
				data = FMemory::Alloc(dataSize);
				if (!WebPDecodeRGBInto(dataPtr, size, (uint8*)data, dataSize, width * 3))
				{
					FMemory::Free(data);
					data = nullptr;
					return false;
				}
			}

			if (option.bFlipV)
			{
				int stride = width * numComponent;
				TArray<uint8> temp;
				temp.resize(stride);
				uint8* p1 = (uint8*)data;
				uint8* p2 = (uint8*)data + (height - 1) * stride;
				for (int i = 0; i < height / 2; ++i)
				{
					FMemory::Copy(temp.data(), p1, stride);
					FMemory::Copy(p1, p2, stride);
					FMemory::Copy(p2, temp.data(), stride);
					p1 += stride;
					p2 -= stride;
				}
			}
			return true;
		}
	}

	ScopedSetFlipVertically scoped(option.bFlipV);

	int comp;
	if (!stbi_info_from_memory((stbi_uc const *)inData, size, &width, &height, &comp))
		return false;

	int req_comp = STBI_default;
	if (option.bUpThreeComponentToFour && comp == 3)
	{
		req_comp = 4;
	}
	if (option.bHDR)
	{
		data = stbi_loadf_from_memory((stbi_uc const *)inData, size, &width, &height, &numComponent, req_comp);
		dataSize = (uint64)sizeof(float) * width * height;
	}
	else
	{
		data = stbi_load_from_memory((stbi_uc const *)inData, size, &width, &height, &numComponent, req_comp);
		dataSize = (uint64)sizeof(uint8) * width * height;
	}

	if (req_comp != STBI_default)
	{
		numComponent = req_comp;
	}
	dataSize *= numComponent;

	return data != nullptr;
}

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"
#include "FileSystem.h"

bool ImageData::SaveImage(char const* path, int w, int h, int numComponent, void const* data, bool bHDR)
{
	FFileSystem::CreateDirectory(FFileUtility::GetDirectory(path).toCString());
	if (bHDR)
	{
		return stbi_write_hdr(path, w, h, numComponent, (float const*)data) != 0;
	}
	else
	{
		return stbi_write_png(path, w, h, numComponent, data, w * numComponent) != 0;
	}
}

#endif