#include "ImageData.h"

#include "FileSystem.h"
#include "DataStructure/Array.h"

#include "stb/stb_image.h"

ImageData::ImageData()
{
	data = nullptr;
}

ImageData::~ImageData()
{
	if( data )
	{
		stbi_image_free(data);
	}
}

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
	if (option.bUpThreeComponentToFour)
	{
		TArray<uint8> buffer;
		if (!FFileUtility::LoadToBuffer(path, buffer))
			return false;

		return loadFromMemory(buffer.data(), buffer.size(), option);
	}
	else
	{
		ScopedSetFlipVertically scoped(option.bFlipV);
		if (option.bHDR)
		{
			data = stbi_loadf(path, &width, &height, &numComponent, STBI_default);
			dataSize = sizeof(float) * numComponent * width * height;
		}
		else
		{
			data = stbi_load(path, &width, &height, &numComponent, STBI_default);
			dataSize = sizeof(uint8) * numComponent * width * height;
		}
	}

	return data != nullptr;
}

bool ImageData::loadFromMemory(void* inData, int size, ImageLoadOption const& option)
{
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
		dataSize = sizeof(float) * width * height;
	}
	else
	{
		data = stbi_load_from_memory((stbi_uc const *)inData, size, &width, &height, &numComponent, req_comp);
		dataSize = sizeof(uint8) * width * height;
	}

	if (req_comp != STBI_default)
	{
		numComponent = req_comp;
	}
	dataSize *= numComponent;

	return data != nullptr;
}
