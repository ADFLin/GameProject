#include "ImageData.h"

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

bool ImageData::load(char const* path, bool bHDR, bool bReverseH)
{
	stbi_set_flip_vertically_on_load(bReverseH);
	if( bHDR )
	{
		data = stbi_loadf(path, &width, &height, &numComponent, STBI_default);
		dataSize = sizeof(float) * numComponent * width * height;
	}
	else
	{
		data = stbi_load(path, &width, &height, &numComponent, STBI_default);
		dataSize = sizeof(uint8) * numComponent * width * height;
	}
	stbi_set_flip_vertically_on_load(false);
	return data != nullptr;
}
