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
	bool     empty() const { return mHead != 0; }

	void     push_front( T const& val );
	void     push_back( T const& val );
	T const& front() const {  return mHead->data;  }
	T&       front()       {  return mHead->data;  }
	T const& back() const  {  return mTail->data;  }
	T&       back()        {  return mTail->data;  }

	void     inverse();
	void     rotateFront();
	void     rotateBack();

public:
	class Iterator
	{
	public:
		Iterator(const SList&l):mCur( l.mHead ){};
		bool haveMore() const { return mCur != 0; }
		T&   getValue(){ assert( mCur ); return mCur->data; }
		void next(){ mCur = mCur->link; }
	private:
		Node* mCur;//points to a node in list
	};

	Iterator createIterator(){ return Iterator( *this ); }


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
		assert( node != mHead );
		Node* result = mHead;
		for(;;)
		{
			if ( result->link == node )
				return result;
			result = result->link;
		}
		assert( 0 );
		return nullptr;
	}

	int   mNum;
	Node* mHead;
	Node* mTail;
};

template <class T>
SList<T>::SList()
{
	mHead  = nullptr;
	mTail  = nullptr;
	mNum   = 0;
}

template <class T>
SList<T>::~SList()
{
	Node* next;
	for(;mHead;mHead = next)
	{
		next = mHead->link;
		delete mHead;
	}
}

template <class T>
void SList<T>::push_front( T const& val )
{
	Node* newnode = buyNode( val );

	++mNum;
	if( mHead )
	{
		newnode->link = mHead;
		mHead = newnode;
	}
	else
	{
		mHead = mTail = newnode;
	}
}

template <class T>
void SList<T>::push_back( T const& val )
{
	Node* newnode = buyNode( val );
	newnode->link = nullptr;
	++mNum;
	if( mTail )
	{
		mTail->link = newnode;
		mTail = newnode;
	}
	else
	{
		mHead = mTail = newnode;
	}
}

template <class T>
void SList<T>::inverse()
{
	Node* p = mHead;
	Node* q = nullptr;
	while(p)
	{
		Node* r = q;
		q = p;
		p = p->link;
		q->link = r;
	}
	mHead = q;
}

template <class T>
void SList<T>::rotateFront()
{
	if ( mHead == mTail )
		return;

	Node* node = mHead;
	mHead = mHead->link;
	mTail->link = node;
	mTail = node;
	mTail->link = nullptr;
}

template <class T>
void SList<T>::rotateBack()
{
	if ( mHead == mTail )
		return;

	Node* node = getPrevNode( mTail );
	mTail->link = mHead;
	mHead = mTail;
	mTail = node;
	mTail->link = nullptr;
}

#endif // SList_h__
