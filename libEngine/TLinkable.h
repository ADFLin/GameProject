#ifndef TLinkable_h__
#define TLinkable_h__

template< class T >
class TLinkable
{
public:
	TLinkable()
		:mNext( NULL )
		,mPrev( NULL ){}

	class Iterator
	{
	public:
		bool haveMore(){  return mPtr != NULL; }
		void goNext()  {  assert( mPtr != NULL ); mPtr = mPtr->mNext; }
		T*   getElement(){  return mPtr; }

	private:
		friend class TLinkable;
		Iterator( T* ptr ):mPtr(ptr){}
		T* mPtr;
	};

	Iterator getIterator(){ return Iterator( static_cast< T*>(this) ); }


	T* getNextLink(){ return mNext; }
	T* getPrevLink(){ return mPrev; }


	bool isLinked(){ return mNext != NULL || mPrev != NULL; }
	void unlink()
	{
		if ( mPrev )
			mPrev->mNext = mNext;

		if ( mNext )
			mNext->mPrev = mPrev;

		mNext = NULL;
		mPrev = NULL;
	}

	void removeFront()
	{
		if ( mNext )
			mNext->mPrev = NULL;
		mNext = NULL;
	}

	void removeBack()
	{
		if ( mPrev )
			mPrev->mNext = NULL;
		mPrev = NULL;
	}

	void insertFront( T* link )
	{
		assert( link );

		if ( mNext )
			mNext->mPrev = link;

		link->mNext = mNext;
		link->mPrev = static_cast< T*>( this );
		mNext = link;
	}

	void insertBack( T* link )
	{
		assert( link && !link->isLinked() );

		if ( mPrev )
			mPrev->mNext = link;

		link->mNext = static_cast< T*>( this );
		link->mPrev = mPrev;
		mPrev = link;
	}


	void  clearLink()
	{
		mPrev = mNext = NULL;
	}


private:
	T*     mNext;
	T*     mPrev;
};


template< class T , size_t offset >
class TLinkableList
{
public:
	void size() const { return mSize;  }
	T&   front(){ return *static_cast< T* >( mFrist );  }
	T&   back() { return *static_cast< T* >( mLast );  }
	bool empty(){ return mSize == 0;  }

	class iterator
	{




	};

private:
	typedef TLinkable< T > Node;
	Node*   mFrist;
	Node*   mLast;
	size_t  mSize;
};

#endif // TLinkable_h__