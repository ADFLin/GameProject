#pragma once
#ifndef CycleQueue_H_6D751446_66FF_4899_B21F_F7F2EE0F5C1D
#define CycleQueue_H_6D751446_66FF_4899_B21F_F7F2EE0F5C1D

#include "TypeMemoryOp.h"

#include "DataStructure/Array.h"
#include <algorithm>



template< typename T, typename Allocator = DefaultAllocator >
class TCycleQueue : private Allocator::template TArrayData< T >
{
	using ArrayData = typename Allocator::template TArrayData< T >;

public:
	TCycleQueue()
	{
		mNum = 0;
		mIndexCur = 0;
		mIndexNext = 0;
	}

	~TCycleQueue()
	{
		destructElements();
	}


	void clear()
	{
		destructElements();
		mIndexCur = 0;
		mIndexNext = 0;
		mNum = 0;
	}


	template< class ...Args >
	void emplace(Args ...args)
	{
		T* ptr = addUninitialized();
		FTypeMemoryOp::Construct(ptr, std::forward<Args>(args)...);
	}

	void push_back(T const& value)
	{
		T* ptr = addUninitialized();
		FTypeMemoryOp::Construct(ptr, value);
	}

	template< typename Iter >
	void addRange(Iter itBegin, Iter itEnd)
	{
		size_t addSize = (size_t)std::distance(itBegin, itEnd);
		if (addSize == 0)
			return;

		T* ptr = addUninitialized(addSize);
		size_t indexStart = ptr - ArrayData::getAllocation();
		size_t maxSize = ArrayData::getMaxSize();
		size_t numTail = maxSize - indexStart;

		if (addSize <= numTail)
		{
			FTypeMemoryOp::ConstructSequence(ptr, addSize, itBegin, itEnd);
		}
		else
		{
			FTypeMemoryOp::ConstructSequence(ptr, numTail, itBegin, std::next(itBegin, numTail));
			FTypeMemoryOp::ConstructSequence(getElement(0), addSize - numTail, std::next(itBegin, numTail), itEnd);
		}
	}

	void pop_front()
	{
		assert(mNum);
		FTypeMemoryOp::Destruct(getElement(mIndexCur));
		--mNum;

		++mIndexCur;
		if(mIndexCur == ArrayData::getMaxSize())
			mIndexCur = 0;
	}

	T& front()
	{
		assert(mNum);
		return *getElement(mIndexCur);
	}
	T const&   front() const
	{
		assert(mNum);
		return *getElement(mIndexCur);
	}
	bool empty() const { return mNum == 0; }
	size_t size() const { return mNum;  }

	T* getElement(size_t index)
	{
		return ArrayData::getAllocation() + index;
	}

	void checkIndex()
	{
		CHECK(0 <= mIndexCur && mIndexCur < ArrayData::getMaxSize());
		CHECK(0 <= mIndexNext && mIndexNext < ArrayData::getMaxSize());
	}

	void moveFrontToBack()
	{
		if (mNum == 0)
			return;

		if (mNum == ArrayData::getMaxSize())
		{
			CHECK(mIndexCur == mIndexNext);

			++mIndexCur;
			if (mIndexCur == ArrayData::getMaxSize())
				mIndexCur = 0;

			mIndexNext = mIndexCur;
		}
		else
		{
			FTypeMemoryOp::Move(getElement(mIndexNext), getElement(mIndexCur));
			++mIndexNext;
			++mIndexCur;
			if (mIndexCur == ArrayData::getMaxSize())
				mIndexCur = 0;
			else if (mIndexNext == ArrayData::getMaxSize())
				mIndexNext = 0;
		}
		checkIndex();
	}

	T* addUninitialized(size_t addSize = 1)
	{
		if (ArrayData::needAlloc(mNum, addSize))
		{
			if (mIndexCur != 0)
			{
				ArrayData::alloc(mNum, addSize);
				size_t growSize = ArrayData::getMaxSize() - mNum;
				if (growSize >= mIndexNext)
				{
					FTypeMemoryOp::MoveSequence(getElement(mNum), mIndexCur, getElement(0));
					mIndexNext += mNum;
					if (mIndexNext >= ArrayData::getMaxSize())
						mIndexNext -= ArrayData::getMaxSize();
				}
				else
				{
					FTypeMemoryOp::MoveSequence(getElement(mNum), growSize, getElement(0));
					FTypeMemoryOp::MoveSequence(getElement(0), mIndexNext - growSize, getElement(growSize));
					mIndexNext -= growSize;
				}
			}
			else
			{
				ArrayData::alloc(mNum, addSize);
				mIndexNext = mNum;
			}
		}

		T* result = getElement(mIndexNext);
		mIndexNext += addSize;
		if (mIndexNext >= ArrayData::getMaxSize())
			mIndexNext -= ArrayData::getMaxSize();
		mNum += addSize;

		checkIndex();
		return result;
	}

private:

	void destructElements()
	{
		size_t numTail = ArrayData::getMaxSize() - mIndexCur;
		if (mNum > numTail)
		{
			FTypeMemoryOp::DestructSequence(getElement(mIndexCur), numTail);
			FTypeMemoryOp::DestructSequence(getElement(0), mNum - numTail);
		}
		else
		{
			FTypeMemoryOp::DestructSequence(getElement(mIndexCur), mNum);
		}

	}

	struct IteratorBase
	{
		IteratorBase( TCycleQueue* inQueue , size_t inIndex ):queue(inQueue),index(inIndex){}
		TCycleQueue* queue;
		size_t       index;
		IteratorBase& operator++(int)
		{
			++index;
			if( index == queue->ArrayData::getMaxSize())
				index = 0;
			return *this;
		}
		IteratorBase operator++() 
		{ 
			IteratorBase temp(*this); 
			++index; 
			if (index == queue->ArrayData::getMaxSize())
				index = 0; 
			return temp;
		}
		bool operator == (IteratorBase const& other) const { assert(queue == other.queue); return index == other.index; }
		bool operator != (IteratorBase const& other) const { return !this->operator==(other); }
	};

	struct Iterator : IteratorBase
	{
		using IteratorBase::IteratorBase;
		T* operator->() { return queue->getElement(index); }
		T& operator*() { return *(this->operator->() );  }
	};

	struct ConstIterator : IteratorBase
	{
		using IteratorBase::IteratorBase;
		T const* operator->() { return queue->getElement(index); }
		T const& operator*() { return *(this->operator->()); }
	};

	typedef Iterator iterator;
	typedef ConstIterator const_iterator;

public:
	iterator begin() { return iterator(this, mIndexCur); }
	iterator end()   {  return iterator(this, mIndexNext); }
	const_iterator cbegin() { return const_iterator(this, mIndexCur); }
	const_iterator cend() { return const_iterator(this, mIndexNext); }
private:

	size_t mIndexNext;
	size_t mIndexCur;
	size_t mNum;
};


template< class T, int N >
class TCycleBuffer
{

public:
	TCycleBuffer()
	{
		mIndexStart = 0;
		mNum = 0;
	}

	template< class ...Args >
	void emplace(Args ...args)
	{
		if (mNum == N)
		{
			FTypeMemoryOp::Assign((T*)mStorage[mIndexStart], std::forward<Args>(args)...);
			++mIndexStart;
			if (mIndexStart == N)
				mIndexStart = 0;
		}
		else
		{
			FTypeMemoryOp::Construct<T>(mStorage[(mIndexStart + mNum) % N], std::forward<Args>(args)...);
			++mNum;
		}

	}

	template< typename Iter, TEnableIf_Type< TIsIterator<Iter>::Value, bool > = true >
	void append(Iter itBegin, Iter itEnd)
	{
		addRange(itBegin, itEnd);
	}

	template< typename Iter, TEnableIf_Type< TIsIterator<Iter>::Value, bool > = true >
	void addRange(Iter itBegin, Iter itEnd)
	{
		for (; itBegin != itEnd; ++itBegin)
		{
			push_back(*itBegin);
		}
	}

	template< class Q >
	void push_back(Q&& value)
	{
		if (mNum == N)
		{
			FTypeMemoryOp::Assign((T*)mStorage[mIndexStart], std::forward<Q>(value));
			++mIndexStart;
			if (mIndexStart == N)
				mIndexStart = 0;

		}
		else
		{
			FTypeMemoryOp::Construct((T*)mStorage[(mIndexStart + mNum) % N], std::forward<Q>(value));
			++mNum;
		}
	}

	void pop_back()
	{
		assert(mNum);
		int index = (mIndexStart + mNum - 1) % N;
		FTypeMemoryOp::Destruct((T*)mStorage[index]);
		--mNum;
	}

	void pop_front()
	{
		assert(mNum);
		FTypeMemoryOp::Destruct((T*)mStorage[mIndexStart]);
		++mIndexStart;
		if (mIndexStart == N)
			mIndexStart = 0;
		--mNum;
	}

	size_t size() const { return mNum; }
	bool empty() const { return mNum == 0; }
	void clear()
	{
		size_t numTail = N - mIndexStart;
		if (mNum > numTail)
		{
			FTypeMemoryOp::DestructSequence(mStorage.mData + mIndexStart, numTail);
			FTypeMemoryOp::DestructSequence(mStorage.mData, mNum - numTail);
		}
		else
		{
			FTypeMemoryOp::DestructSequence(mStorage.mData + mIndexStart, mNum);
		}
		mNum = 0;
		mIndexStart = 0;
	}

	struct IteratorBase
	{
		IteratorBase(TCycleBuffer* inQueue, int inCount) :buffer(inQueue), count(inCount) {}
		TCycleBuffer* buffer;
		size_t        count;
		IteratorBase& operator++(int) { ++count;  return *this; }
		IteratorBase& operator+= (int offset) { count += offset; return *this; }
		IteratorBase  operator++() { IteratorBase temp(*this); ++count; return temp; }

		T* getElement() { return (T*)buffer->mStorage[(buffer->mIndexStart + count) % N]; }
		bool operator == (IteratorBase const& other) const { assert(buffer == other.buffer); return count == other.count; }
		bool operator != (IteratorBase const& other) const { return !this->operator==(other); }
	};

	struct Iterator : IteratorBase
	{
		using IteratorBase::IteratorBase;
		T* operator->() { return getElement(); }
		T& operator*() { return *(this->operator->()); }
	};

	struct ConstIterator : IteratorBase
	{
		using IteratorBase::IteratorBase;
		T const* operator->() { return getElement(); }
		T const& operator*() { return *(this->operator->()); }
	};

	typedef Iterator iterator;
	typedef ConstIterator const_iterator;

	iterator begin() { return Iterator(this, 0); }
	iterator end() { return Iterator(this, mNum); }

	const_iterator cbegin() const { return ConstIterator(this, 0); }
	const_iterator cend() const { return ConstIterator(this, mNum); }

	T const& operator [](int index) const { return *(T*)mStorage[(index + mIndexStart) % mNum]; }

	size_t mNum;
	size_t mIndexStart;
	TCompatibleStorage< T, N > mStorage;

};


#endif // CycleQueue_H_6D751446_66FF_4899_B21F_F7F2EE0F5C1D
