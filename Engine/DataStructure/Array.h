#pragma once
#ifndef Array_H_53348890_2577_41C4_9C3E_01D135F952BD
#define Array_H_53348890_2577_41C4_9C3E_01D135F952BD

#include "Core/IntegerType.h"
#include "TypeMemoryOp.h"

#include <string>

template< typename T>
struct TBitwiseReallocatable
{
	static constexpr int Value = 1;
};

#define BITWISE_RELLOCATABLE_FAIL(TYPE)\
	template<>\
	struct TBitwiseReallocatable<TYPE>\
	{\
		static constexpr int Value = 0;\
	}


BITWISE_RELLOCATABLE_FAIL(std::string);
BITWISE_RELLOCATABLE_FAIL(std::wstring);

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


template< typename T >
void MoveValue(T& lhs, T& rhs, T reset = T())
{
	lhs = rhs;
	rhs = reset;
}


#define CHECK_ALLOC_PTR(PTR) if (PTR == nullptr){ std::abort(); }

template< class T >
struct TDynamicArrayData
{
	TDynamicArrayData()
	{
		mStorage = nullptr;
		mMaxSize = 0;
	}

	~TDynamicArrayData()
	{
		cleanup();
	}

	void cleanup()
	{
		if (mStorage)
		{
			FMemory::Free(mStorage);
			mStorage = nullptr;
			mMaxSize = 0;
		}
	}

	DECL_ALLOCATOR T* grow(size_t oldSize, size_t newSize)
	{
		size_t allocSize = sizeof(T) * newSize;
		if constexpr (TBitwiseReallocatable<T>::Value)
		{
			void* newAlloc = FMemory::Realloc(mStorage, allocSize);
			CHECK_ALLOC_PTR(newAlloc);
			mStorage = newAlloc;
		}
		else
		{
			if (mStorage == nullptr)
			{
				void* newAlloc = FMemory::Alloc(allocSize);
				CHECK_ALLOC_PTR(newAlloc);
				mStorage = newAlloc;
			}
			else if (FMemory::Expand(mStorage, allocSize) == nullptr)
			{
				void* newAlloc = FMemory::Alloc(allocSize);
				CHECK_ALLOC_PTR(newAlloc);
				if (oldSize)
				{
					FTypeMemoryOp::MoveSequence((T*)newAlloc, oldSize, (T*)mStorage);
				}
				FMemory::Free(mStorage);
				mStorage = newAlloc;
			}
		}


		mMaxSize = newSize;
		return (T*)mStorage;
	}

	void reserve(size_t oldSize, size_t size)
	{
		if (size > mMaxSize)
		{
			grow(oldSize, size);
		}
	}

	void swap(TDynamicArrayData& other)
	{
		using std::swap;
		swap(mStorage, other.mStorage);
		swap(mMaxSize, other.mMaxSize);
	}


	void  shrinkTofit(size_t size)
	{
		if (size == 0 || size == mMaxSize)
			return;

		if (FMemory::Expand(mStorage, size) != nullptr)
		{
			mMaxSize = size;
			return;
		}

		void* newAlloc = FMemory::Alloc(size);
		CHECK_ALLOC_PTR(newAlloc);
		FTypeMemoryOp::MoveSequence((T*)newAlloc, size, (T*)mStorage);
		FMemory::Free(mStorage);

		mStorage = newAlloc;
		mMaxSize = size;
	}

	size_t getMaxSize() const { return mMaxSize; }
	T*     getAllocation() const { return (T*)mStorage; }

	TDynamicArrayData& operator = (TDynamicArrayData&& rhs)
	{
		cleanup();
		MoveValue(mStorage, rhs.mStorage);
		MoveValue(mMaxSize, rhs.mMaxSize);
		return *this;
	}

	void*  mStorage;
	size_t mMaxSize;
};


struct DefaultAllocator
{
	template< class T >
	struct TArrayData : TDynamicArrayData<T>
	{
		bool needAlloc(size_t OldSize, size_t numNewElements)
		{
			return OldSize + numNewElements > mMaxSize;
		}

		void alloc(size_t oldSize, size_t numNewElements)
		{
			size_t growSize = oldSize + numNewElements;
			growSize += (3 * growSize) / 8;
			grow(oldSize, growSize);
		}
	};
};

struct FixedSizeAllocator
{
	template< class T >
	struct TArrayData : TDynamicArrayData<T>
	{
		bool needAlloc(size_t OldSize, size_t numNewElements)
		{
			CHECK(OldSize + numNewElements <= mMaxSize);
			return false;
		}

		void alloc(size_t OldSize, size_t numNewElements)
		{

		}
	};
};

template < size_t TotalSize >
struct TInlineAllocator
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
		bool   needAlloc(size_t OldSize, size_t numNewElements)
		{
			CHECK(OldSize + numNewElements <= TotalSize);
			return false;
		}
		void   alloc(size_t OldSize, size_t numNewElements)
		{
		}
		size_t getMaxSize() const { return TotalSize;  }
		T*     getAllocation() const { return (T*)mStorage; }

		void reserve(size_t size)
		{
			CHECK(size <= TotalSize);
		}
		void shrinkTofit(size_t size)
		{

		}
		TCompatibleByte< T >  mStorage[TotalSize];
	};
};

template< class T , class Allocator = DefaultAllocator >
class TArray : private Allocator::template TArrayData< T >
{
	using ArrayData = typename Allocator::template TArrayData< T >;

public:
	using iterator = T*;
	using const_iterator = T const* ;
	using reverse_iterator = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;

	typedef T         value_type;
	typedef T&        reference;
	typedef T const & const_refernece;
	typedef T*        pointer;
	typedef T const*  const_pointer;

	TArray()
	{
		mNum = 0;
	}

	TArray(std::initializer_list<T> list)
	{
		mNum = 0;
		addRange(list.begin(), list.end());
	}

	explicit TArray(size_t size)
	{
		mNum = 0;
		T* ptr = addUninitialized(size);
		FTypeMemoryOp::ConstructSequence(ptr, size);
	}

	TArray(size_t size, T const& value)
	{
		mNum = 0;
		T* ptr = addUninitialized(size);
		FTypeMemoryOp::ConstructSequence(ptr, size, value);
	}

	TArray(TArray const& rhs)
	{
		mNum = 0;
		addRange(rhs.begin(), rhs.end());
	}

	TArray(TArray&& rhs)
	{
		MoveValue(mNum, rhs.mNum);
		ArrayData::operator = (std::move(rhs));
	}

	template< typename Iter , TEnableIf_Type< TIsIterator<Iter>::Value , bool > = true >
	TArray(Iter itBegin, Iter itEnd)
	{
		mNum = 0;
		addRange(itBegin, itEnd);
	}

	TArray& operator = (TArray const& rhs)
	{
		CHECK(this != &rhs);
		clear();
		addRange(rhs.begin(), rhs.end());
		return *this;
	}

	TArray& operator = ( TArray&& rhs )
	{
		CHECK(this != &rhs);
		clear();
		MoveValue(mNum, rhs.mNum);
		ArrayData::operator = ( std::move(rhs) );
		return *this;
	}

	~TArray()
	{
		clear();
	}

	bool operator == (TArray const& rhs) const
	{
		if (mNum != rhs.mNum)
			return false;

		for (int i = 0; i < mNum; ++i)
		{
			if ( !(*getElement(i) == *rhs.getElement(i)) )
				return false;
		}
		return true;
	}

	bool operator != (TArray const& rhs) const
	{
		return !this->operator == (rhs);
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

	const_reverse_iterator rbegin() const { return const_reverse_iterator(getElement(mNum)); }
	reverse_iterator rbegin() { return reverse_iterator(getElement(mNum)); }
	const_reverse_iterator rend() const { return const_reverse_iterator(getElement(0)); }
	reverse_iterator rend() { return reverse_iterator(getElement(0)); }


	T const* data() const { return getElement(0); }
	T*       data()       { return getElement(0); }


	bool isValidIndex(int index) const
	{
		return 0 <= index && index < size();
	}

	void reserve(size_t size)
	{
		ArrayData::reserve(mNum, size);
	}

	template< typename ...TArgs >
	void emplace_back(TArgs ...args)
	{
		T* ptr = addUninitialized();
		FTypeMemoryOp::Construct(ptr, std::forward<TArgs>(args)...);
	}

	void push_back(T const& val)
	{
		T* ptr = addUninitialized();
		FTypeMemoryOp::Construct(ptr, val);
	}

	void push_back(T&& val)
	{
		T* ptr = addUninitialized();
		FTypeMemoryOp::Construct(ptr, std::move(val));
	}

	void insert(iterator where, T const& val)
	{
		int indexPos = int(where - begin());
		insertAt(indexPos, val);
	}

	void insert(iterator where, size_t num, T const& val)
	{
		int indexPos = int(where - begin());
		insertAt(indexPos, num, val);
	}

	template< typename Iter, TEnableIf_Type< TIsIterator<Iter>::Value, bool > = true >
	void insert(iterator where, Iter itBegin, Iter itEnd)
	{
		int indexPos = int(where - begin());
		size_t numMove = end() - where;
		size_t addSize = std::distance(itBegin, itEnd);
		T* ptr = addUninitialized(addSize);
		if (numMove)
		{
			FTypeMemoryOp::MoveRightOverlap(getElement(indexPos + addSize), numMove, getElement(indexPos));
		}

		FTypeMemoryOp::ConstructSequence(getElement(indexPos), addSize, itBegin, itEnd);
	}

	void insertAt(int indexPos, T const& val)
	{
		size_t numMove = size() - size_t(indexPos);
		T* ptr = addUninitialized(1);
		if (numMove)
		{
			FTypeMemoryOp::MoveRightOverlap(getElement(indexPos + 1), numMove, getElement(indexPos));
		}

		FTypeMemoryOp::Construct(getElement(indexPos), val);
	}

	void insertAt(int indexPos, size_t num, T const& val)
	{
		size_t numMove = size() - size_t(indexPos);
		T* ptr = addUninitialized(num);
		if (numMove)
		{
			FTypeMemoryOp::MoveRightOverlap(getElement(indexPos + num), numMove, getElement(indexPos));
		}

		FTypeMemoryOp::ConstructSequence(getElement(indexPos), num, val);
	}

	bool addUnique(T const& val)
	{
		if (findIndex(val) != INDEX_NONE)
			return false;
		push_back(val);
		return true;
	}

	void append(std::initializer_list<T> list)
	{
		addRange(list.begin(), list.end());
	}

	void append(TArray const& rhs)
	{
		addRange(rhs.begin(), rhs.end());
	}

	template< typename Iter, TEnableIf_Type< TIsIterator<Iter>::Value, bool > = true >
	void append(Iter itBegin, Iter itEnd)
	{
		addRange(itBegin, itEnd);
	}

	void append(int size, T const& value)
	{
		T* ptr = addUninitialized(size);
		FTypeMemoryOp::ConstructSequence(ptr, size, value);
	}

	void pop_back()
	{ 
		assert(size() != 0);
		--mNum;
		FTypeMemoryOp::Destruct(getElement(mNum));
	}

	iterator erase(iterator iter)
	{
		checkRange(iter);
		FTypeMemoryOp::Destruct(iter);

		moveToEnd(iter, iter + 1);
		return iter;
	}
	
	iterator erase(iterator from, iterator to)
	{
		checkRange(from);
		checkRange(to - 1);
		assert(to > from);
		FTypeMemoryOp::DestructSequence(from, to - from);
		moveToEnd(from, to);
		return from;
	}

	void removeIndex(int index)
	{
		erase(begin() + index);
	}

	void removeIndexSwap(int index)
	{
		checkRange( begin() + index);
		--mNum;
		if (index != mNum)
		{
			FTypeMemoryOp::Move(getElement(index), getElement(mNum));
		}
		FTypeMemoryOp::Destruct(getElement(mNum));
	}

	int findIndex(T const& value) const
	{
		for (int index = 0; index < mNum; ++index)
		{
			if (*getElement(index) == value)
				return index;
		}
		return INDEX_NONE;
	}

	template< typename TFunc >
	int findIndexPred(TFunc&& func) const
	{
		for (int index = 0; index < mNum; ++index)
		{
			if (func(*getElement(index)))
				return index;
		}
		return INDEX_NONE;
	}

	bool remove(T const& value)
	{
		int index = findIndex(value);
		if (index == INDEX_NONE)
			return false;

		removeIndex(index);
		return true;
	}

	template< typename TFunc >
	bool removePred(TFunc&& func)
	{
		int index = findIndexPred(std::forward<TFunc>(func));
		if (index == INDEX_NONE)
			return false;

		removeIndex(index);
		return true;
	}

	template< typename TFunc >
	int removeAllPred(TFunc&& func)
	{
		int index = 0;
		for (int i = 0; i < mNum; ++i)
		{
			T* ptr = getElement(i);
			if (func(*ptr))
			{
				FTypeMemoryOp::Destruct(ptr);
			}
			else 
			{
				if (index != i)
				{
					FTypeMemoryOp::Move(getElement(index), ptr);
				}
				++index;
			}
		}
		int result = int(mNum) - index;
		mNum = index;
		return result;
	}

	void removeChecked(T const& value)
	{
		int index = findIndex(value);
		CHECK(index != INDEX_NONE);
		removeIndex(index);
	}

	bool removeSwap(T const& value)
	{
		int index = findIndex(value);
		if (index != INDEX_NONE)
		{
			removeIndexSwap(index);
			return true;
		}
		return false;
	}

	bool isUnique() const
	{
		for (int i = 0; i < mNum; ++i)
		{
			for (int j = i + 1; j < mNum; ++j)
			{
				if (*getElement(i) == *getElement(j))
					return false;
			}
		}
		return true;
	}

	void makeUnique(int idxStart = 0)
	{
		int idxLast = int(mNum) - 1;
		for (int i = idxStart; i <= idxLast; ++i)
		{
			for (int j = 0; j < i; ++j)
			{
				if (*getElement(i) == *getElement(j))
				{
					if (i != idxLast)
					{
						std::swap(*getElement(i), *getElement(idxLast));
						--i;
					}
					--idxLast;
				}
			}
		}
		if (idxLast + 1 != mNum)
		{
			resize(idxLast + 1);
		}
	}

	void resize(size_t num)
	{
		if (num == mNum)
			return;

		if (num < size())
		{
			erase(begin() + num, end());
		}
		else
		{
			size_t delta = num - mNum;
			T* ptr = addUninitialized(delta);
			FTypeMemoryOp::ConstructSequence(ptr, delta);
		}
	}

	void resize(size_t num, value_type const& value)
	{
		if (num == mNum)
			return;

		if (num < mNum)
		{
			erase(begin() + num, end());
		}
		else
		{
			size_t delta = num - mNum;
			T* ptr = addUninitialized(delta);
			FTypeMemoryOp::ConstructSequence(ptr, delta, value);
		}
	}

	template< typename Iter, TEnableIf_Type< TIsIterator<Iter>::Value, bool > = true >
	void assign(Iter itBegin, Iter itEnd)
	{
		eraseToEnd(begin());
		addRange(itBegin, itEnd);
	}

	template< typename Iter, TEnableIf_Type< TIsIterator<Iter>::Value, bool > = true >
	void addRange(Iter itBegin, Iter itEnd)
	{
		size_t num = std::distance(itBegin , itEnd);
		if (num)
		{
			T* ptr = addUninitialized(num);
			FTypeMemoryOp::ConstructSequence(ptr, num, itBegin, itEnd);
		}
	}

	bool     empty()  const { return !mNum; }
	size_t   size()     const { return mNum; }
	void     clear() 
	{ 
		eraseToEnd(begin()); 
	}
	void shrink_to_fit()
	{
		ArrayData::shrinkTofit(mNum);
	}

	void swap(TArray& other)
	{
		using std::swap;
		swap(mNum, other.mNum);
		ArrayData::swap(other);
	}


	T* addUninitialized(size_t num = 1)
	{
		if (ArrayData::needAlloc(mNum, num))
		{
			ArrayData::alloc(mNum, num);
		}

		T* result = getElement(mNum);
		mNum += num;
		return result;
	}

	T* addDefault(size_t num = 1)
	{
		T* result = addUninitialized(num);
		FTypeMemoryOp::ConstructSequence(result, num);
		return result;
	}

	T* getElement(size_t index)
	{
		return ArrayData::getAllocation() + index;
	}

	T const* getElement(size_t index) const
	{
		return ArrayData::getAllocation() + index;
	}

	void  moveToEnd(iterator where, iterator src)
	{
		size_t num = mNum - (src - getElement(0));
		if (num)
		{
			FTypeMemoryOp::MoveSequence(where, num, src);
		}
		mNum = mNum - size_t(src - where);
	}


	int  eraseToEnd(iterator is)
	{
		int numRemoved = end() - is;
		FTypeMemoryOp::DestructSequence(is, numRemoved);
		mNum = size_t(is - getElement(0));
		return numRemoved;
	}

	void   checkRange(const_iterator it) const { assert(begin() <= it && it < end()); }
	size_t mNum;

};

template< typename T >
void* operator new (std::size_t count, TArray<T>& ar) 
{ 
	return ar.addUninitialized(); 
}




#endif // Array_H_53348890_2577_41C4_9C3E_01D135F952BD