#include "HashString.h"

#include "CString.h"
#include "FixString.h"
#include "Core/TypeHash.h"
#include "TypeConstruct.h"
#include "Meta/MetaBase.h"

#include <functional>
#include <cassert>
#include <type_traits>

#include "Core/FNV1a.h"


template< class T, int ChunkNum, int ChunkElementNum >
class TChunkArray
{
public:
	TChunkArray()
	{
		std::fill_n(mChunkStorage, ChunkNum, nullptr);
	}

	~TChunkArray()
	{
		release();
	}

	void* addUninitialized()
	{
		int index = (int)mNumElement;
		int idxChunk = index / ChunkElementNum;
		int idxElement = index % ChunkElementNum;
		if( mChunkStorage[idxChunk] == nullptr )
		{
			mChunkStorage[idxChunk] = ::malloc(sizeof(T) * ChunkElementNum);
		}
		++mNumElement;
		return (T*)(mChunkStorage[idxChunk]) + idxElement;
	}
	T& operator [](int index)
	{
		int idxChunk = index / ChunkElementNum;
		int idxElement = index % ChunkElementNum;
		return *((T*)(mChunkStorage[idxChunk]) + idxElement);
	}

	size_t size() const { return mNumElement; }

private:

	template< class Q = T >
	auto release() -> typename std::enable_if< std::is_trivially_destructible<Q>::value >::type
	{
		if( mNumElement )
		{
			int numChunk = (mNumElement - 1) / ChunkElementNum + 1;
			for( int i = 0; i < numChunk; ++i )
			{
				::free(mChunkStorage[i]);
			}
		}
	}

	template< class Q = T >
	auto release() -> typename std::enable_if< !std::is_trivially_destructible<Q>::value >::type
	{
		if( mNumElement )
		{
			int numChunk = (mNumElement - 1) / ChunkElementNum + 1;
			for( int i = 0; i < numChunk - 1; ++i )
			{
				for( int n = 0; n < ChunkElementNum; ++n )
				{
					T* ptr = (T*)(mChunkStorage[i]) + n;
					ptr->~T();
				}
				::free(mChunkStorage[i]);
			}

			int idxChunk = numChunk - 1;

			int num = mNumElement % ChunkElementNum;
			for( int n = 0; n < num; ++n )
			{
				T* ptr = (T*)(mChunkStorage[idxChunk]) + n;
				ptr->~T();
			}

			::free(mChunkStorage[idxChunk]);
		}
	}

	void*  mChunkStorage[ChunkNum];
	size_t mNumElement;
};

int const NameSlotHashBucketSize = 0xffff;
int const MaxHashStringLength = 2048;
int const NameSlotChunkNum = 256;

struct NameSlot
{
	FixString< MaxHashStringLength > str;
	uint32    hashValue;
	uint32    index;
	NameSlot* next;
	static NameSlot* sHashHead[NameSlotHashBucketSize];
	static NameSlot* sHashTail[NameSlotHashBucketSize];

	static TChunkArray< NameSlot, 2 * 1024 * 4, NameSlotChunkNum > sNameSlots;

	int compare(char const* other, bool bCaseSensitive)
	{
		if( bCaseSensitive )
			return str.compare( other );
		return FCString::CompareIgnoreCase(str, other);
	}

	int compareN(char const* other, int len , bool bCaseSensitive)
	{
		if( bCaseSensitive )
			return str.compareN(other, len);
		return FCString::CompareIgnoreCaseN(str, other, len);
	}

};

struct HashStringInternal
{
	static TChunkArray< NameSlot, 2 * 1024 * 4, NameSlotChunkNum > sNameSlots;

};

TChunkArray< NameSlot, 2 * 1024 * 4, NameSlotChunkNum > HashStringInternal::sNameSlots;

NameSlot* NameSlot::sHashHead[NameSlotHashBucketSize];
NameSlot* NameSlot::sHashTail[NameSlotHashBucketSize];

#if CORE_SHARE_CODE

CORE_API void* DebugHashStringSlot = &HashStringInternal::sNameSlots;
void HashString::Initialize()
{
	std::fill_n(NameSlot::sHashHead, NameSlotHashBucketSize, nullptr);
	std::fill_n(NameSlot::sHashTail, NameSlotHashBucketSize, nullptr);
	{
		HashString name( EName::None , "None" );
	}
}


void HashString::init(char const* str, bool bCaseSensitive)
{
	if( str == nullptr )
	{
		mIndex = 0;
		mNumber = 0;
		return;
	}

#if 0
	FixString< MaxHashStringLength > temp;
	if( !bCaseSensitive )
	{
		FString::Stricpy(temp, str);
		str = temp;
	}
#endif

	uint32 hashValue = FCString::StriHash(str);
	uint32 idxHash = hashValue % NameSlotHashBucketSize;
	NameSlot* slot = NameSlot::sHashHead[idxHash];
	for( ; slot; slot = slot->next )
	{
		if( hashValue == slot->hashValue && slot->compare(str, bCaseSensitive) == 0 )
			break;
	}

	if( slot == nullptr )
	{
		int idx = HashStringInternal::sNameSlots.size();
		void* ptr = HashStringInternal::sNameSlots.addUninitialized();

		slot = new (ptr) NameSlot;
		slot->hashValue = hashValue;
		slot->str = str;
		slot->index = idx;
		slot->next = nullptr;

		//#TODO : thread-safe
		if( NameSlot::sHashTail[idxHash] )
			NameSlot::sHashTail[idxHash]->next = slot;

		NameSlot::sHashTail[idxHash] = slot;

		if( NameSlot::sHashHead[idxHash] == nullptr )
			NameSlot::sHashHead[idxHash] = slot;
	}

	mIndex = slot->index << 1;
	if( !bCaseSensitive )
		mIndex |= 0x1;
	mNumber = 0;
}

void HashString::init(char const* str, int len, bool bCaseSensitive /*= true*/)
{
	if( str == nullptr || len == 0 )
	{
		mIndex = 0;
		mNumber = 0;
		return;
	}

#if 0
	FixString< MaxHashStringLength > temp;
	if( !bCaseSensitive )
	{
		FString::Stricpy(temp, str);
		str = temp;
	}
#endif

	uint32 hashValue = FCString::StriHash(str, len);
	uint32 idxHash = hashValue % NameSlotHashBucketSize;
	NameSlot* slot = NameSlot::sHashHead[idxHash];
	for( ; slot; slot = slot->next )
	{
		if( hashValue == slot->hashValue && slot->compareN(str, len, bCaseSensitive) == 0 )
			break;
	}

	if( slot == nullptr )
	{
		int idx = HashStringInternal::sNameSlots.size();
		void* ptr = HashStringInternal::sNameSlots.addUninitialized();

		slot = new (ptr) NameSlot;
		slot->hashValue = hashValue;
		slot->str = StringView(str, len);
		slot->index = idx;
		slot->next = nullptr;

		//#TODO : thread-safe
		if( NameSlot::sHashTail[idxHash] )
			NameSlot::sHashTail[idxHash]->next = slot;

		NameSlot::sHashTail[idxHash] = slot;

		if( NameSlot::sHashHead[idxHash] == nullptr )
			NameSlot::sHashHead[idxHash] = slot;
	}

	mIndex = slot->index << 1;
	if( !bCaseSensitive )
		mIndex |= 0x1;
	mNumber = 0;
}

HashString::HashString(EName name, char const* str)
	:HashString(str)
{
	assert((mIndex >> 1) == (uint32)name);
}

char const* HashString::c_str() const
{
	if( mIndex == 0 )
		return "";

	NameSlot& slot = HashStringInternal::sNameSlots[getSlotIndex()];
	return slot.str;
}

uint32 HashString::getHash() const
{
	NameSlot& slot = HashStringInternal::sNameSlots[getSlotIndex()];
	return slot.hashValue;
}

bool HashString::operator==(char const* str) const
{
	NameSlot& slot = HashStringInternal::sNameSlots[getSlotIndex()];
	return slot.compare(str, isCastSensitive()) == 0;
}

#endif




