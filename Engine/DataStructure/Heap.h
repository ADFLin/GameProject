#ifndef Heap_h__
#define Heap_h__

#include "DataStructure/Array.h"

template< class KT , class CmpFunc = std::less< KT > >
class TBinaryHeap
{
public:
	typedef KT     KeyType;
	typedef CmpFunc CompareFunType;

	typedef KeyType Node;

	~TBinaryHeap()
	{
		clear();
	}

	void clear()
	{
		for( int i = 0 ; i < mTreeMap.size() ; ++i )
		{
			freeNode( mTreeMap[i] );
		}
		mTreeMap.clear();
	}

	KeyType&   top(){ return *mTreeMap[0]; }

	void pop()
	{
		assert( size() != 0 ); 
		remove(0);
	}

	void push(const KeyType& Val)
	{
		Node* node = allocNode( Val );
		mTreeMap.push_back( node );
		filterUp(size()-1);
	}

	bool empty(){ return  mTreeMap.empty(); }
	void remove(int index)
	{
		//assert( index >= 0 && index < size() );
		Node* node = mTreeMap[ index ];
		freeNode( node );

		std::swap( mTreeMap[index] , mTreeMap.back() );
		mTreeMap.pop_back();
		if ( !empty() )
		{
			filterDown( index );
		}
	}
	size_t size() const { return mTreeMap.size(); }
	size_t capacity() const { return mTreeMap.capacity(); }

private:

	Node* getNode(int index){ return mTreeMap[index]; }
	void  setNode(int index, Node* node ){  mTreeMap[index] = node; }

	static int ParentIndex(int index){ return (index - 1) / 2;}
	static int RightChildIndex(int index){ return 2 * (index + 1); }
	static int LeftChildIndex(int index){ return 2 * index + 1; }

	void filterDown( int idxStart )
	{
		Node* target = getNode(idxStart);

		int idxRChild;

		int idxCur = idxStart;
		int idxChild = LeftChildIndex(idxCur);

		while ( idxChild < size() )
		{
			idxRChild = idxChild + 1;
			if ( idxRChild < size() && compareNode(getNode(idxRChild),getNode(idxChild)) )
				idxChild = idxRChild;

			if ( compareNode(target, getNode(idxChild)) )
				break;

			setNode( idxCur , getNode(idxChild) );

			idxCur = idxChild;
			idxChild = LeftChildIndex(idxCur);
		}
		setNode(idxCur,target);
	}

	void filterUp(int idxStart )
	{
		Node* Target= getNode(idxStart);

		int idxCur = idxStart;
		while (idxCur != 0)
		{
			int idxParent = ParentIndex(idxCur);
			if ( compareNode( getNode(idxParent), Target ) )
				break;

			setNode(idxCur,getNode(idxParent));
			idxCur = idxParent;
		}
		setNode(idxCur,Target);
	}

	bool  compareNode(Node* a, Node* b) { return CompareFunType()(*a, *b); }
	void  freeNode(Node* node) { delete node; }
	Node* allocNode(KeyType const& key) { return new Node(key); }
	TArray< Node* > mTreeMap;
};


class TreeHeapBase
{
protected:
	template< class T >
	struct ListHook
	{
		T* next;
		T* prev;

		void init()
		{
			prev = next = static_cast< T* >( this );
		}

		void linkBefore( ListHook* node )
		{
			assert( next && prev );
			prev->next = static_cast< T* >( node );
			node->prev = prev;
			node->next = static_cast< T* >( this );
			this->prev = static_cast< T* >( node );
		}

		void linkNext( ListHook* node )
		{
			assert( next && prev );
			next->prev = static_cast< T* >( node );
			node->next = next;
			node->prev = static_cast< T* >( this );
			this->next = static_cast< T* >( node );
		}

		void unLink()
		{
			assert( next != this && prev != this );
			prev->next = next;
			next->prev = prev;
		}

		void merge( ListHook& list )
		{
			ListHook* other = list.next;
			assert( other != &list );
			T* thisPrev = this->prev;
			T* otherPrev = list.prev;
			thisPrev->next = static_cast< T* >( other );
			other->prev = thisPrev;
			otherPrev->next = static_cast< T* >( this );
			this->prev = otherPrev;
		}

		template< class TFunc >
		void visitList( TFunc func )
		{
			T* node = next;
			while( node != this )
			{
				func(node);
				node = node->next;
			}
		}

	};

};


template < class KT , class CmpFunc = std::less< KT > >
class TFibonacciHeap : public TreeHeapBase
{
public:

	TFibonacciHeap()
	{
		mNumNode = 0;
		mNodeMin = nullptr;
		mRoot.init();
	}

	~TFibonacciHeap()
	{
		cleanupNode();
	}


	struct Node;
	typedef KT     KeyType;
	typedef CmpFunc CompareFunType;
	typedef Node*  NodeHandle;

	static NodeHandle EmptyHandle() { return nullptr; }

	struct Node : ListHook< Node >
	{
		KeyType key;
		ListHook< Node > children;
		Node*   parent;
		uint8   degree;
		bool    marked;
	};

	void clear()
	{
		cleanupNode();
		mNumNode = 0;
		mNodeMin = nullptr;
		mRoot.init();
	}

	NodeHandle push( KeyType const& key )
	{
		Node* node = allocNode();
		node->key = key;
		node->degree = 0;
		node->marked = false;
		node->children.init();
		node->parent = nullptr;

		mRoot.linkNext( node );

		if ( mNodeMin == nullptr )
		{
			mNodeMin = node;
		}
		else
		{
			if ( compareKey( node , mNodeMin ) )
				mNodeMin = node;
		}
		++mNumNode;
		return node;
	}

	void  update(NodeHandle handle)
	{
		Node* nodeCur = handle;
		for( ;;)
		{
			Node* parent = nodeCur->parent;
			if( parent == nullptr )
			{
				if( nodeCur != mNodeMin && compareKey(nodeCur, mNodeMin) )
				{
					mNodeMin = nodeCur;
				}
				return;
			}
			if( !compareKey(nodeCur->key, parent->key) )
				return;

			using std::swap;
			swap(nodeCur->key, parent->key);
			nodeCur = parent;
		}
	}

	void  update(NodeHandle handle, KeyType const& key)
	{
		assert(compareKey(key, handle->key));
		handle->key = key;
		update(handle);
	}

	bool  compareKey(KeyType const& key1, KeyType const& key2) const { return CompareFunType()(key1, key2); }
	bool  compareKey( Node* n1 , Node* n2 ) const { return CompareFunType()( n1->key , n2->key ); }


	int   size() const { return mNumNode; }
	bool  empty() const { return mNodeMin == nullptr; }
	KeyType const& top(){ assert( mNodeMin ); return mNodeMin->key; }

	void pop()
	{
		if ( mNodeMin == nullptr )
			return;
		
		mNodeMin->unLink();
		bool bOneNodeOnly = (mRoot.next == &mRoot);

		{
			ListHook< Node >* children = &mNodeMin->children;
			Node* child = children->next;
			if ( child != children )
			{
				do
				{
					child->parent = nullptr;
					child = child->next;
				}
				while( child != children );
				mRoot.merge( mNodeMin->children );
			}
		}

		--mNumNode;
		freeNode(mNodeMin);

		if ( bOneNodeOnly )
		{
			mNodeMin = findMinNode();
		}
		else
		{
			//int maxDegree = 1 + int( log( double(mNumNode + 1) ) / log( 2.0 ) );
			int const MaxNodeMapNum = sizeof( size_t ) * 8;
			//assert( maxDegree < MaxNodeMapNum );
			Node* nodeMap[ MaxNodeMapNum ];
			std::fill_n( nodeMap , MaxNodeMapNum , (Node*)nullptr );

			Node* cur = mRoot.next;
			mNodeMin = cur;
			nodeMap[ cur->degree ] = cur;
			cur = cur->next;
			while( cur != &mRoot )
			{
				Node* next = cur->next;
				Node* mergeNode = nodeMap[ cur->degree ];
				while ( mergeNode )
				{
					assert( mergeNode != cur );

					nodeMap[cur->degree] = nullptr;
					if ( compareKey( cur , mergeNode ) == false )
						std::swap( cur , mergeNode );

					mergeNode->unLink();
					cur->children.linkBefore( mergeNode );
					mergeNode->parent = cur;
					++cur->degree;

					mergeNode = nodeMap[ cur->degree ];
				}
				if ( compareKey( cur , mNodeMin ) )
					mNodeMin = cur;

				nodeMap[ cur->degree ] = cur;
				cur = next;
			}
		}
	}

	Node* findMinNode()
	{
		if( mRoot.next == &mRoot )
			return nullptr;

		Node* result = mRoot.next;
		Node* cur = result->next;
		while( cur != &mRoot )
		{
			if ( compareKey( cur , result ) )
				result = cur;
			cur = cur->next;
		}
		return result;
	}

	void cleanupNode()
	{
		Node* cur = mRoot.next;
		while( cur != &mRoot )
		{
			Node* next = cur->next;
			destroyTree( cur );
			cur = next;
		}
	}

	void destroyTree( Node* node )
	{
		Node* cur = node->children.next;
		while( cur != &node->children )
		{
			Node* next = cur->next;
			destroyTree( cur );
			cur = next;
		}
		freeNode( node );
	}


private:
	void  freeNode( Node* node ){  delete node; }
	Node* allocNode(){ return new Node; }

	ListHook< Node > mRoot;
	Node* mNodeMin;
	int   mNumNode;
};


template< class KT , class CmpFunc = std::less< KT > >
class TPairingHeap : public TreeHeapBase
{
public:

	struct Node;
	typedef KT    KeyType;
	typedef Node* HandleType;
	typedef CmpFunc CompareFuncType;

	struct Node : ListHook< Node >
	{
		KeyType key;
		ListHook< Node > children;
	};

	TPairingHeap()
	{
		mRoot = nullptr;
		mNodeNum = 0;
	}
	HandleType push( KeyType const& key )
	{
		Node* node = allocNode();
		node->key = key;
		node->children.init();
		node->init();

		++mNodeNum;

		if ( mRoot )
		{
			mRoot = mergeNode( mRoot , node );
		}
		else
		{
			mRoot = node;
		}
		return node;
	}

	void pop()
	{
		if ( mRoot == nullptr )
			return;

		Node* cur = mRoot->children.next;

		Node* temp = mRoot;
		if ( cur == &mRoot->children )
		{
			mRoot = nullptr;
		}
		else
		{
			mRoot = mergeList( mRoot->children );
		}
		freeNode( temp );
		--mNodeNum;
	}

	KeyType const& top(){ assert( mRoot ); return mRoot->key; }
	bool empty() const { return mRoot == nullptr; }
	int  size() const { return mNodeNum; }

	Node* mergeList( ListHook< Node >& nodeList )
	{
		Node* cur = mergeFirstPair( nodeList.next , &nodeList );
		if ( cur->next == &nodeList )
			return cur;
		
		do
		{
			cur = cur->next;
			cur = mergeFirstPair( cur , &nodeList );
		}
		while ( cur->next != &nodeList );
		return mergeList( nodeList );
	}

	Node* mergeFirstPair( Node* node , ListHook< Node >* end )
	{
		assert( node != end );
		Node* next = node->next;
		if ( next == end )
			return node;
		if ( compareKey( next , node ) )
			std::swap( node , next );

		next->unLink();
		node->children.linkNext( next );
		return node;
	}


	Node* mergeNode( Node* a , Node* b )
	{
		if ( compareKey( b , a )  )
			std::swap( a , b );
		a->children.linkNext( b );
		return a;
	}

	bool  compareKey( Node* n1 , Node* n2 ){ return CompareFuncType()( n1->key , n2->key ); }
	void  freeNode( Node* node ){  delete node; }
	Node* allocNode(){ return new Node; }
	Node* mRoot;
	int   mNodeNum;
};

#endif // Heap_h__
