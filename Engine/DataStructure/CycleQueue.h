#pragma once
#ifndef CycleQueue_H_6D751446_66FF_4899_B21F_F7F2EE0F5C1D
#define CycleQueue_H_6D751446_66FF_4899_B21F_F7F2EE0F5C1D

#include "TypeMemoryOp.h"

#include "DataStructure/Array.h"
#include <algorithm>



template< class T >
class TCycleQueue
{
public:
	TCycleQueue()
	{
		mStorage = nullptr;
		mNumElement = 0;
		mNumStorage = 0;
		mIndexCur = 0;
		mIndexNext = 0;
	}

	~TCycleQueue()
	{
		size_t numTail = mNumStorage - mIndexCur;
		if( mNumElement > numTail )
		{
			FTypeMemoryOp::DestructSequence(mStorage + mIndexCur, numTail);
			FTypeMemoryOp::DestructSequence(mStorage, mNumElement - numTail);
		}
		else
		{
			FTypeMemoryOp::DestructSequence(mStorage + mIndexCur, mNumElement);
		}

		if( mStorage )
		{
			::free(mStorage);
		}
	}


	void clear()
	{
		size_t numTail = mNumStorage - mIndexCur;
		if( mNumElement > numTail )
		{
			FTypeMemoryOp::DestructSequence(mStorage + mIndexCur, numTail);
			FTypeMemoryOp::DestructSequence(mStorage, mNumElement - numTail);
		}
		else
		{
			FTypeMemoryOp::DestructSequence(mStorage + mIndexCur, mNumElement);
		}

		mIndexCur = 0;
		mIndexNext = 0;
		mNumElement = 0;
	}

	void push_back(T const& value)
	{
		if( mNumElement == mNumStorage )
		{
			size_t newSize = std::max< size_t >(2 * mNumStorage, 4);
			T* newPtr = (T*)::malloc(sizeof(T) * newSize);

			if( newPtr == nullptr )
				throw std::bad_alloc();

			if( mStorage )
			{
				size_t numTail = mNumStorage - mIndexCur;
				if ( mNumElement > numTail )
				{
					FTypeMemoryOp::ConstructSequence(newPtr, numTail, mStorage + mIndexCur);
					FTypeMemoryOp::ConstructSequence(newPtr + numTail, mNumElement - numTail, mStorage);
				}
				else
				{
					FTypeMemoryOp::ConstructSequence(newPtr, mNumElement, mStorage + mIndexCur);
				}
				FTypeMemoryOp::DestructSequence(mStorage , mNumElement);
			}

			mNumStorage = newSize;
			mStorage = newPtr;
			mIndexNext = mNumElement;
		}

		FTypeMemoryOp::Construct(mStorage + mIndexNext, value);
		++mIndexNext;
		if( mIndexNext == mNumStorage )
			mIndexNext = 0;
		++mNumElement;
	}

	void pop_front()
	{
		assert(mNumElement);
		++mIndexCur;
		if( mIndexNext == mNumStorage )
			mIndexNext = 0;

		FTypeMemoryOp::Destruct(mStorage + mIndexCur);
		--mNumElement;
	}

	T& front()
	{
		assert(mNumElement);
		return *(mStorage + mIndexCur);
	}
	T const&   front() const
	{
		assert(mNumElement);
		return *( mStorage + mIndexCur );
	}
	bool empty() const { return mNumElement == 0; }
	size_t size() const { return mNumElement;  }

private:

	struct IteratorBase
	{
		IteratorBase( TCycleQueue* inQueue , size_t inIndex ):queue(inQueue),idx(inIndex){}
		TCycleQueue* queue;
		size_t       idx;
		IteratorBase& operator++(int)
		{
			++idx;
			if( idx == queue->mNumStorage )
				idx = 0;
			return *this;
		}
		IteratorBase operator++() { IteratorBase temp(*this); ++idx; return temp; }
		bool operator == (IteratorBase const& other) const { assert(queue == other.queue); return idx == other.idx; }
		bool operator != (IteratorBase const& other) const { return !this->operator==(other); }
	};

	struct Iterator : IteratorBase
	{
		using IteratorBase::IteratorBase;
		T* operator->() { return queue->mStorage + idx; }
		T& operator*() { return *(this->operator->() );  }
	};

	struct ConstIterator : IteratorBase
	{
		using IteratorBase::IteratorBase;
		T const* operator->() { return queue->mStorage + idx; }
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

	T* mStorage;
	size_t mIndexNext;
	size_t mIndexCur;
	size_t mNumElement;
	size_t mNumStorage;
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

	int  size() const { return mNum; }
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

	int mNum;
	int mIndexStart;
	TCompatibleStorage< T, N > mStorage;

};


#endif // CycleQueue_H_6D751446_66FF_4899_B21F_F7F2EE0F5C1D
