#pragma once
#ifndef ImageData_H_DB4CAB9E_DC3B_48D7_9C93_9C673825862C
#define ImageData_H_DB4CAB9E_DC3B_48D7_9C93_9C673825862C

#include "Core/IntegerType.h"

struct ImageData
{
	ImageData();

	~ImageData();
	int    width;
	int    height;
	int    numComponent;
	uint64 dataSize;
	void*  data;

	bool load(char const* path, bool bHDR = false , bool bReverseH = false);
};

#endif // ImageData_H_DB4CAB9E_DC3B_48D7_9C93_9C673825862C
