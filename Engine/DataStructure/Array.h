#pragma once
#ifndef Array_H_53348890_2577_41C4_9C3E_01D135F952BD
#define Array_H_53348890_2577_41C4_9C3E_01D135F952BD

#include "Core/IntegerType.h"
#include "TypeConstruct.h"


template< class T >
struct alignas(alignof(T)) TCompatibleByte
{
	struct Data
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
struct alignas(alignof(T)) TCompatibleStorage
{
	void*       operator[](int index)       { return mData + index; }
	void const* operator[](int index) const { return mData + index; }

	~TCompatibleStorage(){}
	TCompatibleByte< T > mData[N];
};


struct DefaultAllocator
{

	template< class T >
	struct TArrayData
	{
		TArrayData()
		{
			mStorage = nullptr;
			mMaxSize = 0;
		}

		void   alloc(int OldSize, int numNewElements)
		{
			if (OldSize + numNewElements > mMaxSize)
			{
				int growSize = OldSize + numNewElements;
				growSize += (3 * growSize) / 8;
				void* newAlloc = FMemory::Realloc(mStorage, growSize);
				if (newAlloc == nullptr)
				{

				}
				if (newAlloc != mStorage)
				{

				}
				mStorage = newAlloc;
				mMaxSize = growSize;
			}
		}
		size_t getMaxSize() const { return mMaxSize; }
		T*     getAllocation() const { return (T*)mStorage; }

		void*  mStorage;
		size_t mMaxSize;
	};


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
	enum 
	{
		IsDynamic = 0,
	};

	template< class T >
	struct TArrayData
	{
		TArrayData()
		{

		}
		void   alloc(int OldSize, int numNewElements)
		{
			CHECK(OldSize + numNewElements <= TotalSize);
		}
		size_t getMaxSize() const { return TotalSize;  }
		T*     getAllocation() const { return (T*)mStorage; }

		TCompatibleByte< T >  mStorage[TotalSize];
	};
};

template< class T , class Allocator = DefaultAllocator >
class TArray : private Allocator::template TArrayData< T >
{
	using ArrayData = typename Allocator::template TArrayData< T >;

public:
	typedef T*        iterator;
	typedef T const*  const_iterator;
	typedef T         value_type;
	typedef T&        reference;
	typedef T const & const_refernece;
	typedef T*        pointer;
	typedef T const*  const_pointer;

	TArray()
	{
		mNum = 0;
	}

	~TArray()
	{
		clear();
	}

	const_refernece front() const { return *getElement(0); }
	reference       front() { return *getElement(0); }
	const_refernece back() const { return *getElement(mNum - 1); }
	reference       back() { return *getElement(mNum - 1); }

	const_refernece operator[](size_t index) const { return *getElement(index); }
	reference       operator[](size_t index) { return *getElement(index); }

	const_iterator begin() const { return getElement(0); }
	iterator       begin() { return getElement(0); }
	const_iterator end()   const { return getElement(mNum); }
	iterator       end() { return getElement(mNum); }



	template< class Q >
	void push_back(Q&& val)
	{ 
		T* ptr = addUninitialized();
		TypeDataHelper::Construct(ptr, std::forward< Q >(val));
	}

	void pop_back()
	{ 
		assert(size() != 0);
		--mNum;
		TypeDataHelper::Destruct(getElement(mNum));
	}

	iterator erase(iterator iter)
	{
		checkRange(iter);
		TypeDataHelper::Destruct(iter);

		moveToEnd(iter, iter + 1);
		return iter;
	}
	
	iterator erase(iterator from, iterator to)
	{
		checkRange(from);
		checkRange(to);
		assert(to > from);
		TypeDataHelper::Destruct(from, to - from);
		moveToEnd(from, to);
		return from;
	}

	void resize(size_t num)
	{
		if (num < size())
		{
			erase(begin() + num, end());
		}
		else
		{
			T* ptr = addUninitialized(num);
			TypeDataHelper::Construct(ptr, num);
		}
	}

	void resize(size_t num, value_type const& value)
	{
		if (num < size())
		{
			erase(begin() + num, end());
		}
		else
		{
			T* ptr = addUninitialized(num);
			TypeDataHelper::Construct(ptr, num, value);
		}
	}

	bool     empty()  const { return !mNum; }
	size_t   size()     const { return mNum; }
	void     clear() { eraseToEnd(begin()); }

	T* addUninitialized()
	{
		ArrayData::alloc(mNum, 1);

		T* result = getElement(mNum);
		++mNum;
		return result;
	}

	T* addUninitialized(size_t num)
	{
		ArrayData::alloc(mNum, num);
		T* result = getElement(mNum);
		mNum += num;
		return result;
	}

	T* getElement(int index)
	{
		return ArrayData::getAllocation() + index;
	}

	T const* getElement(int index) const
	{
		return ArrayData::getAllocation() + index;
	}

	void  moveToEnd(iterator where, iterator src)
	{
		int num = mNum - (src - getElement(0));
		if (num)
		{
			TypeDataHelper::Move(where, num, src);
		}
		mNum = mNum - (src - where);
	}


	void  eraseToEnd(iterator is)
	{
		TypeDataHelper::Destruct(is, end() - is);
		mNum = is - getElement(0);
	}

	void   checkRange(const_iterator it) const { assert(begin() <= it && it < end()); }
	int mNum;

};



#endif // Array_H_53348890_2577_41C4_9C3E_01D135F952BD