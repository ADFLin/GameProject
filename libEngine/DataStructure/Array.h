#pragma once
#ifndef Array_H_53348890_2577_41C4_9C3E_01D135F952BD
#define Array_H_53348890_2577_41C4_9C3E_01D135F952BD

#include "Core/IntegerType.h"


template< class T >
struct TCompatibleByte
{
	struct alignas(alignof(T)) Data
	{
		uint8 bytes[sizeof(T)];
	};
#if _DEBUG
	union DebugData
	{
		T     value;
		Data  pad;
		
		DebugData(){}
		~DebugData(){}
	} data;
#else
	Data pad;
#endif
	
	static int constexpr Size = sizeof(Data);
};


template< class T, int N >
struct TCompatibleStorage
{
	void*       operator[](int index)       { return mData + index; }
	void const* operator[](int index) const { return mData + index; }

	~TCompatibleStorage(){}
	TCompatibleByte< T > mData[N];
};


struct DefaultAllocator
{




};

struct DyanmicFixedAllocator
{

	template< class T >
	struct TArrayData
	{
		T* getHead() { return (T*)mStorage; }

		T* alloc(int size)
		{

		}

		TCompatibleByte< T >* mStorage;
	};
};

template < uint32 TotalSize >
struct TFixedAllocator
{
	template< class T >
	struct TArrayData
	{
		TArrayData()
		{
			mNext = mStorage;
		}
		size_t getMaxSize() const { return TotalSize;  }
		size_t getSize() const { return mNext - mStorage; }
		T* getHead() { return (T*)mStorage; }
		T* getEnd()  { return (T*)mNext; }
		T* alloc(int size)
		{
			assert(mNext + size <= mStorage + TotalSize);
			T* result = (T*)mNext;
			mNext += Size;
			return result;
		}
		void dealloc(int size)
		{
			mNext -= size;
		}

		TCompatibleByte< T >  mStroage[TotalSize];
		TCompatibleByte< T >* mNext;
	};
};

template< class T , class Allocator >
class TArray : private Allocator::template TArrayData< T >
{
	typedef typename Allocator::template TArrayData< T > ArrayData;

public:
	typedef T*        iterator;
	typedef T const*  const_iterator;
	typedef T         value_type;
	typedef T&        reference;
	typedef T const & const_refernece;
	typedef T*        pointer;
	typedef T const*  const_pointer;

	const_refernece front() const { return *ArrayData::getHead(); }
	reference       front() { return *ArrayData::getHead(); }
	const_refernece back() const { return *(ArrayData::getEnd() - 1); }
	reference       back() { return *(ArrayData::getEnd() - 1); }

	const_refernece operator[](size_t idx) const { return ArrayData::getHead()[idx]; }
	reference       operator[](size_t idx) { return ArrayData::getHead()[idx]; }

	template< class Q >
	void push_back(Q&& val)
	{ 
		T* ptr = ArrayData::alloc(1); 
		TypeDataHelper::Construct(ptr, std::forward< Q >(val)); 
	}
	void pop_back()
	{ 
		assert(size() != 0); 
		TypeDataHelper::Destruct(ArrayData::getEnd() - 1); 
		ArrayData::dealloc(1);
	}

	void    resize(size_t num)
	{


	}
	void    resize(size_t num, value_type const& value)
	{


	}

};



#endif // Array_H_53348890_2577_41C4_9C3E_01D135F952BD