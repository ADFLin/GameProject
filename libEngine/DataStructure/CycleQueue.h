#pragma once
#ifndef CycleQueue_H_6D751446_66FF_4899_B21F_F7F2EE0F5C1D
#define CycleQueue_H_6D751446_66FF_4899_B21F_F7F2EE0F5C1D

#include "TypeConstruct.h"

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
			TypeConstructHelpler::destruct(mStorage + mIndexCur, numTail);
			TypeConstructHelpler::destruct(mStorage, mNumElement - numTail);
		}
		else
		{
			TypeConstructHelpler::destruct(mStorage + mIndexCur, mNumElement);
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
			TypeConstructHelpler::destruct(mStorage + mIndexCur, numTail);
			TypeConstructHelpler::destruct(mStorage, mNumElement - numTail);
		}
		else
		{
			TypeConstructHelpler::destruct(mStorage + mIndexCur, mNumElement);
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
					TypeConstructHelpler::construct(newPtr, numTail, mStorage + mIndexCur);
					TypeConstructHelpler::construct(newPtr + numTail, mNumElement - numTail, mStorage);
				}
				else
				{
					TypeConstructHelpler::construct(newPtr, mNumElement, mStorage + mIndexCur);	
				}
				TypeConstructHelpler::destruct(mStorage , mNumElement);
			}

			mNumStorage = newSize;
			mStorage = newPtr;
			mIndexNext = mNumElement;
		}

		TypeConstructHelpler::construct(mStorage + mIndexNext, value);
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

		TypeConstructHelpler::destruct(mStorage + mIndexCur);
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

#endif // CycleQueue_H_6D751446_66FF_4899_B21F_F7F2EE0F5C1D
