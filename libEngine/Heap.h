#ifndef Heap_h__
#define Heap_h__


template< class KeyType >
class BinaryHeap
{






};

template < class KeyType >
class BinomailHeap
{

	struct Node
	{
		KeyType key;
		Node*   parent;
		Node*   child;
		Node*   next;
		Node*   prev;
		int     degree;

		void removeLink()
		{
			assert( next && prev );
			prev->next = next;
			next->prev = prev;
		}
	};


	Node* insert( KeyType const& key )
	{
		if ( mNodeMin == nullptr )
		{
			Node* node = allocNode();
			node->key = key;
			node->child = nullptr;
			node->parent = nullptr;
			node->next = node;
			node->prev = node;
			node->degree = 0;

			mNodeMin = node;
			return node;
		}
		

		Node* node = allocNode();
		node->key = key;
		node->next = mNodeMin;
		node->next->prev = node;
		node->prev = mNodeMin->prev;
		node->prev->next = node;
		node->child = nullptr;
		node->parent = nullptr;

		if ( compare( node , mNodeMin ) )
			mNodeMin = node;

		return node;
	}


	bool compare( Node* n1 , Node* n2 ){ return n1->key > n2->key; }
	void  destroyNode( Node* node ){ delete node; }
	Node* allocNode(){ return new Node; }


	Node* mergeTree( Node* n1 , Node* n2 )
	{
		assert( n1->degree == n2->degree );
		if ( !compare( n1 , n2 ) ) std::swap( n1 , n2 );

		n2->removeLink();
		n2->next = n1->child;
		n2->next->prev = n2;
		n2->prev = n1->child->prev;
		n2->prev->next = n2;
		n2->parent = n1;
		n1->child = n2;
		n1->degree += 1;
		return n1;
	}
	void removeMin()
	{
		if ( mNodeMin == nullptr )
			return;

		Node* start = mNodeMin->next;
		destroyNode( mNodeMin );


		
		Node* degreeMap[];
		int maxDegree;
		for( int i = 0 ; i < maxDegree ; ++i )
			degreeMap[i] = nullptr;

		Node* cur = start;
		do
		{
			if ( degreeMap[ cur->degree ] )
			{
				degreeMap[i] = mergeTree( degreeMap[i] , cur );
			}
			else
			{
				degreeMap[ cur->degree ] = cur;
			}
			cur = cur->next;
		}
		while( cur != start );

	}

	Node* mNodeMin;
	int   mNumNode;
};

#endif // Heap_h__
