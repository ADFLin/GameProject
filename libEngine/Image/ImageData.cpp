#include "ImageData.h"

#include "stb/stb_image.h"
#include "FileSystem.h"
#include <vector>

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

bool ImageData::load(char const* path, bool bHDR, bool bReverseH, bool bUpThreeCompToFour)
{
	stbi_set_flip_vertically_on_load(bReverseH);

	if (bUpThreeCompToFour)
	{
		std::vector<char> buffer;
		if (!FFileUtility::LoadToBuffer(path, buffer))
			return false;

		int comp;
		if (!stbi_info_from_memory((stbi_uc const *)buffer.data(), buffer.size(), &width, &height, &comp))
			return false;

		int req_comp = STBI_default;
		if (comp == 3)
		{
			req_comp = 4;
		}
		if (bHDR)
		{
			data = stbi_loadf_from_memory((stbi_uc const *)buffer.data(), buffer.size(), &width, &height, &numComponent, req_comp);
			dataSize = sizeof(float) * width * height;
		}
		else
		{
			data = stbi_load_from_memory((stbi_uc const *)buffer.data(), buffer.size(), &width, &height, &numComponent, req_comp);
			dataSize = sizeof(uint8) * width * height;
		}
		if (req_comp != STBI_default)
		{
			numComponent = req_comp;
		}

		dataSize *= numComponent;
	}
	else
	{
		if (bHDR)
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
	stbi_set_flip_vertically_on_load(false);
	return data != nullptr;
}
