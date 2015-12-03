#ifndef axUIntHash_h__
#define axUIntHash_h__

#include <boost/pool/object_pool.hpp>
template< class T >
class axUIntHash
{
public:

#define END_ADDRESS 0xffffffff

	axUIntHash(unsigned size = 128)
		:m_hashSize(size)
	{
		m_hash = new Node*[ m_hashSize + 1];
		m_hash[ m_hashSize ] = (Node*)END_ADDRESS;
		for(int i=0;i < m_hashSize ; ++i )
		{
			m_hash[i] = NULL;
		}
	}

	~axUIntHash()
	{
		delete[] m_hash;
	}

	struct Node
	{
		T val;
		Node* next;
	};

	class iterator
	{
	public:

		iterator(Node** h, Node* c)
			:head(h),current(c){}

		T& operator*()
		{
			return current->val;
		}

		bool operator == ( iterator iter )
		{
			return head == iter.head && 
				   current == iter.current;
		}

		bool operator != ( iterator iter )
		{
			return !( *this == iter );
		}

		iterator& operator++()
		{
			if ( current->next )
			{
				current = current->next;
			}
			else
			{
				do {
					++head;
				} while ( *head == NULL );
				if ( *head == (Node*)END_ADDRESS )
					current = NULL;
				else
					current = *head;
			}
			return (*this);
		}

	private:
		Node** head;
		Node* current;

		friend axUIntHash;
	};


	void push( T const& val)
	{
		unsigned index = getIndex( val );

		Node* node = m_hash[ index ];

		Node* newNode = allocNode();
		newNode->val = val;
		newNode->next = node;

		m_hash[index] = newNode;
	}

	void  erase( iterator iter )
	{
		Node* parent = *iter.head;
		while( parent->next != iter.current )
		{
			parent = parent->next;
		}
		parent->next = iter.current->next;
		freeNode( iter.current );
	}

	void clear()
	{
		for(int i=0;i < m_hashSize ; ++i )
		{
			if ( m_hash[i] )
			{
				destoryNode( m_hash[i] );
				m_hash[i] = NULL;
			}
		}
	}

	iterator begin()
	{
		Node** head = &m_hash[0];
		while ( *head == NULL )
		{
			++head;
		}

		if ( *head == (Node*)END_ADDRESS )
			return end();
		else
			return iterator( head , *head );
	}
	iterator end()
	{ 
		return iterator(&m_hash[m_hashSize] , NULL ); 
	}
	template < class FindNodeFun >
	inline iterator find_if( FindNodeFun fun )
	{
		unsigned index = hash_value( fun.getState() );
		index %= m_hashSize;

		Node* node =  m_hash[index];

		while ( node )
		{
			if ( fun( node->val ) )
				return iterator( &m_hash[index] , node );
			node = node->next;
		}
		return end();
	}

protected:
	unsigned getIndex(T const& val)
	{
		return hash_value(val) % m_hashSize;
	}

	void  destoryNode(Node* node)
	{
		while( node != NULL )
		{
			Node* nextNode = node->next;
			freeNode( node );
			node = nextNode;
		}
	}
	Node* allocNode()
	{ 
		Node* ptr = m_pool.malloc();
		return ptr;
	}
	void freeNode(Node* ptr)
	{ 
		if( ptr ) m_pool.free( ptr ); 
	}

	boost::object_pool<Node> m_pool;
	unsigned m_collision;
	unsigned m_hashSize;
	Node** m_hash;
};


#endif //axUIntHash_h__