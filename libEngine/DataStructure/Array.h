#pragma once
#ifndef Array_H_53348890_2577_41C4_9C3E_01D135F952BD
#define Array_H_53348890_2577_41C4_9C3E_01D135F952BD

#include "Core/IntegerType.h"

struct DefaultAllocator
{




};

struct DyanmicFixSizeAllocator
{

	template< class T >
	struct TArrayData
	{
		T* getHead() { return (T*)mStorage; }

		T* alloc(int size)
		{

		}
		struct Element
		{
			char s[sizeof(T)];
		};

		Element* mStorage;
	};
};

template < uint32 TotalSize >
struct FixSizeAllocator
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

		struct Element
		{
			char s[sizeof(T)];
		};

		Element  mStorage[TotalSize];
		Element* mNext;
	};
};

template< class T , class Allocator >
class TArray : private Allocator::TArrayData< T >
{
	typedef Allocator::TArrayData< T > ArrayData;

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

	void push_back(T const& val) 
	{ 
		T* ptr = ArrayData::alloc(1); 
		TypeConstructHelpler::construct(ptr, val); 
	}
	void pop_back()
	{ 
		assert(size() != 0); 
		TypeConstructHelpler::destruct(ArrayData::getEnd() - 1); 
		ArrayData::dealloc(1);
	}

};



#endif // Array_H_53348890_2577_41C4_9C3E_01D135F952BD