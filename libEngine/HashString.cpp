#include "HashString.h"

#include "FixString.h"
#include "TypeHash.h"
#include "MetaBase.h"

#include <functional>
#include <cassert>
#include <type_traits>

struct FString
{
	static void Stricpy( char * dest , char const* src )
	{
		assert(dest && src);
		while( *src )
		{
			int c = ::tolower(*src);
			*dest = c;
			++dest;
			++src;
		}
	}

	static uint32 StriHash(char const* str)
	{
		uint32 result = 5381;
		while( *str )
		{
			uint32 c = (uint32)tolower(*str);
			result = ((result << 5) + result) + c; /* hash * 33 + c */
			++str;
		}
		return result;
	}
};

int const NameSlotHashBucketSize = 0xffff;
int const MaxHashStringLength = 2048;
struct NameSlot
{
	FixString< MaxHashStringLength > str;
	uint32    index;
	NameSlot* next;
	static NameSlot* sHashHead[NameSlotHashBucketSize];
	static NameSlot* sHashTail[NameSlotHashBucketSize];

	int compare(char const* other, bool bCaseSensitive)
	{
		if( bCaseSensitive )
			return str.compare( other );
		return stricmp(str, other);
	}
};

template< class T , int ChunkNum , int ChunkElementNum >
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
			mChunkStorage[idxChunk] = ::malloc( sizeof(T) * ChunkElementNum );
		}
		++mNumElement;
		return (T*)(mChunkStorage[idxChunk]) + idxElement;
	}
	T& operator [](int index)
	{
		int idxChunk = index / ChunkElementNum;
		int idxElement = index % ChunkElementNum;
		return *( (T*)(mChunkStorage[idxChunk]) + idxElement );
	}

	size_t size() const { return mNumElement; }

private:
	
	template< class Q = T >
	auto release() -> typename std::enable_if< std::is_trivially_destructible<Q>::value >::type
	{
		if ( mNumElement )
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
		if ( mNumElement )
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

TChunkArray< NameSlot, 2 * 1024 * 4 , 256 > gNameSlots;

NameSlot* NameSlot::sHashHead[NameSlotHashBucketSize];
NameSlot* NameSlot::sHashTail[NameSlotHashBucketSize];

void HashString::Initialize()
{
	std::fill_n(NameSlot::sHashHead, NameSlotHashBucketSize, nullptr);
	std::fill_n(NameSlot::sHashTail, NameSlotHashBucketSize, nullptr);
	{
		HashString name( EName::None , "None" );
	}
}

HashString::HashString(char const* str , bool bCaseSensitive )
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

	uint32 idxHash = FString::StriHash(str) % NameSlotHashBucketSize;
	NameSlot* slot = NameSlot::sHashHead[idxHash];
	for( ; slot; slot = slot->next )
	{
		if ( slot->compare( str , bCaseSensitive ) == 0)
			break;
	}

	if( slot == nullptr )
	{
		int idx = gNameSlots.size();
		void* ptr = gNameSlots.addUninitialized();

		slot = new (ptr) NameSlot;
		slot->str = str;
		slot->index = idx;
		slot->next = nullptr;

		if ( NameSlot::sHashTail[idxHash] )
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


HashString::HashString(EName name, char const* str )
	:HashString(str)
{
	assert((mIndex >> 1 )== (uint32)name);
}

char const* HashString::toString() const
{
	if( mIndex == 0 )
		return "";

	NameSlot& slot = gNameSlots[getSlotIndex()];
	return slot.str;
}

bool HashString::operator==(char const* str) const
{
	NameSlot& slot = gNameSlots[getSlotIndex()];
	return slot.compare(str, isCastSensitive()) == 0;
}

