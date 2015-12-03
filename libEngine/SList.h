#ifndef SList_h__
#define SList_h__

#include <cassert>

template <class T>
class SList
{
	struct Node;
public:
	SList();
	~SList();

	int      size() const { return mNum; }
	bool     empty() const { return mFirst != 0; }

	void     push_front( T const& val );
	void     push_back( T const& val );
	T const& front() const {  return mFirst->data;  }
	T&       front()       {  return mFirst->data;  }
	T const& back() const  {  return mLast->data;  }
	T&       back()        {  return mLast->data;  }

	void     inverse();
	void     rotateFront();
	void     rotateBack();

public:
	class Iterator
	{
	public:
		Iterator(const SList&l):mCur( l.mFirst ){};
		bool haveMore() const { return mCur != 0; }
		T&   getValue(){ assert( mCur ); return mCur->data; }
		void next(){ mCur = mCur->link; }
	private:
		Node* mCur;//points to a node in list
	};

	Iterator getIterator(){ return Iterator( *this ); }


private:
	Node* buyNode( T const& val ){ return new T( val ); }

	struct Node
	{
		Node( T const& val ):data( val ){}
		T     data;
		Node* link;
	};
	Node*    getPrevNode( Node* node )
	{
		assert( node != mFirst );
		Node* result = mFirst;
		for(;;)
		{
			if ( result->link == node )
				return result;
			result = result->link;
		}
		assert( 0 );
		return 0;
	}

	int   mNum;
	Node* mFirst;
	Node* mLast;
};

template <class T>
SList<T>::SList()
{
	mFirst = 0;
	mLast  = 0;
	mNum   = 0;
}

template <class T>
SList<T>::~SList()
{
	Node* next;
	for(;mFirst;mFirst = next)
	{
		next = mFirst->link;
		delete mFirst;
	}
}

template <class T>
void SList<T>::push_front( T const& val )
{
	Node* newnode = buyNode( val );

	++mNum;
	if( mFirst )
	{
		newnode->link = mFirst;
		mFirst = newnode;
	}
	else
	{
		mFirst = mLast = newnode;
	}
}

template <class T>
void SList<T>::push_back( T const& val )
{
	Node* newnode = buyNode( val );
	newnode->link = 0;
	++mNum;
	if( mLast )
	{
		mLast->link = newnode;
		mLast = newnode;
	}
	else
	{
		mFirst = mLast = newnode;
	}
}

template <class T>
void SList<T>::inverse()
{
	Node* p = mFirst;
	Node* q = 0;
	while(p)
	{
		Node* r = q;
		q = p;
		p = p->link;
		q->link = r;
	}
	mFirst = q;
}

template <class T>
void SList<T>::rotateFront()
{
	if ( mFirst == mLast )
		return;

	Node* node = mFirst;
	mFirst = mFirst->link;
	mLast->link = node;
	mLast = node;
	mLast->link = 0;
}

template <class T>
void SList<T>::rotateBack()
{
	if ( mFirst == mLast )
		return;

	Node* node = getPrevNode( mLast );
	mLast->link = mFirst;
	mFirst = mLast;
	mLast = node;
	mLast->link = 0;
}

#endif // SList_h__
