#include "AsmCode.h"

#include <iostream>


#if SYS_PLATFORM_WIN
#include "WindowsHeader.h"

class ExecutableHeapManager
{
public:
	ExecutableHeapManager()
	{
		mHandle = ::HeapCreate(HEAP_CREATE_ENABLE_EXECUTE, 1 * 1024 * 1204, 128 * 1024 * 1204);
	}

	~ExecutableHeapManager()
	{
		::HeapDestroy(mHandle);
	}

	static ExecutableHeapManager& Get() { static ExecutableHeapManager sInstance; return sInstance; }

	void* alloc(size_t size)
	{
		return ::HeapAlloc(mHandle, 0, size);
	}
	void* realloc(void* p, size_t size)
	{
		return ::HeapReAlloc(mHandle, 0, p, size);
	}

	void free(void* p)
	{
		::HeapFree(mHandle, 0, p);
	}

	HANDLE mHandle;
};

static void* AllocExecutableMemory(size_t size)
{
	return ExecutableHeapManager::Get().alloc(size);
}
static void* ReallocExecutableMemory(void* ptr, size_t size)
{
	return ExecutableHeapManager::Get().realloc(ptr, size);
}
static void  FreeExecutableMemory(void* ptr)
{
	return ExecutableHeapManager::Get().free(ptr);
}
#else
static void* AllocExecutableMemory(size_t size)
{
	return ::malloc(size);
}
static void* ReallocExecutableMemory(void* ptr, size_t size)
{
	return realloc(ptr, size);
}
static void  FreeExecutableMemory(void* ptr)
{
	::free(ptr);
}

#endif

AsmCodeBuffer::AsmCodeBuffer(int size /*= 0*/)
{
	mMaxCodeSize = (size) ? size : 64;
	mCode = (uint8*)AllocExecutableMemory(mMaxCodeSize);
	assert(mCode);
	clear();
}

AsmCodeBuffer::~AsmCodeBuffer()
{
	FreeExecutableMemory(mCode);
}

void AsmCodeBuffer::setCode(unsigned pos, uint8 byte)
{
	assert(pos < getCodeLength());
	mCode[pos] = byte;
}
void AsmCodeBuffer::setCode(unsigned pos, void* ptr)
{
	assert(pos < getCodeLength());
	*reinterpret_cast<void**>(mCode + pos) = ptr;
}

void AsmCodeBuffer::pushCode(uint8 byte)
{
	checkCodeSize(1);
	pushCodeInternal(byte);
}

void AsmCodeBuffer::pushCode(uint8 byte1, uint8 byte2)
{
	checkCodeSize(2);
	pushCodeInternal(byte1);
	pushCodeInternal(byte2);
}

void AsmCodeBuffer::pushCode(uint8 byte1, uint8 byte2, uint8 byte3)
{
	checkCodeSize(3);
	pushCodeInternal(byte1);
	pushCodeInternal(byte2);
	pushCodeInternal(byte3);
}

void AsmCodeBuffer::pushCode(uint8 const* data, int size)
{
	checkCodeSize(size);
	FMemory::Copy(mCodeEnd, data, size);
	mCodeEnd += size;
}

void AsmCodeBuffer::pushCodeInternal(uint8 byte)
{
	*mCodeEnd = byte;
	++mCodeEnd;
}


void AsmCodeBuffer::printCode()
{
	std::cout << std::hex;
	int num = getCodeLength();
	for (int i = 0; i < num; ++i)
	{
		int c1 = mCode[i] & 0x0f;
		int c2 = mCode[i] >> 4;
		std::cout << c1 << c2 << " ";

		if ((i + 1) % 8 == 0)
			std::cout << "| ";
		if ((i + 1) % 32 == 0)
			std::cout << std::endl;
	}
	std::cout << std::dec << std::endl;
}

void AsmCodeBuffer::checkCodeSize(int freeSize)
{
	int codeNum = getCodeLength();
	if (codeNum + freeSize <= mMaxCodeSize)
		return;

	int newSize = 2 * mMaxCodeSize;
	assert(newSize >= codeNum + freeSize);

	uint8* newPtr = (uint8*)ReallocExecutableMemory(mCode, newSize);
	if (!newPtr)
	{
		throw ExprParseException(eAllocFailed, "");
	}

	mCode = newPtr;
	mCodeEnd = mCode + codeNum;
	mMaxCodeSize = newSize;
}

void AsmCodeBuffer::clear()
{
	FMemory::Set(mCode, 0, mMaxCodeSize);
	mCodeEnd = mCode;
}
