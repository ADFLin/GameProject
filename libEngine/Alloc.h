#pragma once
#ifndef Alloc_H_3A9B397B_C105_465A_ABFA_86C1D021B8FE
#define Alloc_H_3A9B397B_C105_465A_ABFA_86C1D021B8FE


void* AllocMemory(size_t size)
{
	return ::malloc(size);
}

void FreeMemory(void* ptr)
{
	return ::free(ptr);
}

void* AllocMemoryAligned( size_t size , size_t alginment )
{
	size_t alginedSize = size;
	return AllocMemory(size);
}
#endif // Alloc_H_3A9B397B_C105_465A_ABFA_86C1D021B8FE
