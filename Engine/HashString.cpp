#include "HashString.h"

#include "CString.h"
#include "InlineString.h"
#include "Core/TypeHash.h"
#include "TypeMemoryOp.h"
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
			mChunkStorage[idxChunk] = FMemory::Alloc(sizeof(T) * ChunkElementNum);
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

	void release()
	{
		if( mNumElement == 0 )
			return;

		int numChunk = (mNumElement + ChunkElementNum - 1) / ChunkElementNum;
		if constexpr ( std::is_trivially_destructible_v<T> )
		{
			for( int i = 0; i < numChunk; ++i )
			{
				FMemory::Free(mChunkStorage[i]);
			}
		}
		else
		{
			for (int i = 0; i < numChunk - 1; ++i)
			{
				FTypeMemoryOp::DestructSequence((T*)mChunkStorage[i], ChunkElementNum);
				FMemory::Free(mChunkStorage[i]);
			}

			int idxChunk = numChunk - 1;
			int num = mNumElement % ChunkElementNum;
			FTypeMemoryOp::DestructSequence((T*)mChunkStorage[idxChunk], num);
			FMemory::Free(mChunkStorage[idxChunk]);
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
	InlineString< MaxHashStringLength > str;
	uint32    hashValue;
	uint32    index;
	NameSlot* next;

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

#if CORE_SHARE_CODE

namespace HashStringInternal
{
	static TChunkArray< NameSlot, 2 * 1024 * 4, NameSlotChunkNum > GNameSlots;
	static NameSlot* GHashHead[NameSlotHashBucketSize];
	static NameSlot* GHashTail[NameSlotHashBucketSize];
};

CORE_API void* DebugHashStringSlot = &HashStringInternal::GNameSlots;
void HashString::Initialize()
{
	std::fill_n(HashStringInternal::GHashHead, NameSlotHashBucketSize, nullptr);
	std::fill_n(HashStringInternal::GHashTail, NameSlotHashBucketSize, nullptr);
	{
		HashString name( EName::None , "None" );
	}
}

bool HashString::Find(StringView const& str, bool bCaseSensitive, HashString& outValue)
{
	uint32 hashValue;
	if (bCaseSensitive)
	{
		hashValue = FCString::StrHash(str.data(), str.length());
	}
	else
	{
		hashValue = FCString::StriHash(str.data(), str.length());
	}
	uint32 idxHash = hashValue % NameSlotHashBucketSize;
	NameSlot* slot = HashStringInternal::GHashHead[idxHash];
	for (; slot; slot = slot->next)
	{
		if (hashValue == slot->hashValue && slot->compareN(str.data(), str.length(), bCaseSensitive) == 0)
			break;
	}

	if (slot == nullptr)
	{
		return false;
	}

	outValue.mIndex = slot->index << 1;
	if (!bCaseSensitive)
		outValue.mIndex |= 0x1;
	outValue.mNumber = 0;
	return true;
}

void HashString::init(char const* str, bool bCaseSensitive)
{
	if( str == nullptr || *str == 0 )
	{
		mIndex = 0;
		mNumber = 0;
		return;
	}

	uint32 hashValue;
	if (bCaseSensitive)
	{
		hashValue = FCString::StrHash(str);
	}
	else
	{
		hashValue = FCString::StriHash(str);
	}
	uint32 idxHash = hashValue % NameSlotHashBucketSize;
	NameSlot* slot = HashStringInternal::GHashHead[idxHash];
	for( ; slot; slot = slot->next )
	{
		if( hashValue == slot->hashValue && slot->compare(str, bCaseSensitive) == 0 )
			break;
	}

	if( slot == nullptr )
	{
		int idx = HashStringInternal::GNameSlots.size();
		void* ptr = HashStringInternal::GNameSlots.addUninitialized();

		slot = new (ptr) NameSlot;
		slot->hashValue = hashValue;
		slot->str = str;
		slot->index = idx;
		slot->next = nullptr;

		//#TODO : thread-safe
		if(HashStringInternal::GHashTail[idxHash] )
			HashStringInternal::GHashTail[idxHash]->next = slot;

		HashStringInternal::GHashTail[idxHash] = slot;

		if(HashStringInternal::GHashHead[idxHash] == nullptr )
			HashStringInternal::GHashHead[idxHash] = slot;
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

	uint32 hashValue;
	if (bCaseSensitive)
	{
		hashValue = FCString::StrHash(str, len);
	}
	else
	{
		hashValue = FCString::StriHash(str, len);
	}
	uint32 idxHash = hashValue % NameSlotHashBucketSize;
	NameSlot* slot = HashStringInternal::GHashHead[idxHash];
	for( ; slot; slot = slot->next )
	{
		if( hashValue == slot->hashValue && slot->compareN(str, len, bCaseSensitive) == 0 )
			break;
	}

	if( slot == nullptr )
	{
		int idx = HashStringInternal::GNameSlots.size();
		void* ptr = HashStringInternal::GNameSlots.addUninitialized();

		slot = new (ptr) NameSlot;
		slot->hashValue = hashValue;
		slot->str = StringView(str, len);
		slot->index = idx;
		slot->next = nullptr;

		//#TODO : thread-safe
		if(HashStringInternal::GHashTail[idxHash] )
			HashStringInternal::GHashTail[idxHash]->next = slot;

		HashStringInternal::GHashTail[idxHash] = slot;

		if(HashStringInternal::GHashHead[idxHash] == nullptr )
			HashStringInternal::GHashHead[idxHash] = slot;
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

	NameSlot& slot = HashStringInternal::GNameSlots[getSlotIndex()];
	return slot.str;
}

uint32 HashString::getTypeHash() const
{
	NameSlot& slot = HashStringInternal::GNameSlots[getSlotIndex()];
	return slot.hashValue;
}

bool HashString::operator==(char const* str) const
{
	NameSlot& slot = HashStringInternal::GNameSlots[getSlotIndex()];
	return slot.compare(str, isCastSensitive()) == 0;
}

#endif




