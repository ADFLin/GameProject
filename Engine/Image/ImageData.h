#pragma once
#ifndef ImageData_H_DB4CAB9E_DC3B_48D7_9C93_9C673825862C
#define ImageData_H_DB4CAB9E_DC3B_48D7_9C93_9C673825862C

#include "Core/IntegerType.h"

struct ImageLoadOption
{
	bool   bHDR = false;
	bool   bFlipV = false;
	bool   bUpThreeComponentToFour = false;

	ImageLoadOption& FlipV(bool value = true) { bFlipV = value; return *this; }
	ImageLoadOption& HDR(bool value = true) { bHDR = value; return *this; }
	ImageLoadOption& UpThreeComponentToFour(bool value = true) { bUpThreeComponentToFour = value; return *this; }
};

struct ImageData
{
	ImageData();
	~ImageData();

	int    width;
	int    height;
	int    numComponent;
	uint64 dataSize;
	void*  data;

	bool load(char const* path, ImageLoadOption const& option = ImageLoadOption());
	bool loadFromMemory(void* inData, int size, ImageLoadOption const& option = ImageLoadOption());
};

#endif // ImageData_H_DB4CAB9E_DC3B_48D7_9C93_9C673825862C
