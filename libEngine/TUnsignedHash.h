#ifndef TUnsignedHash_h__
#define TUnsignedHash_h__

#include <algorithm>

#define END_ADDRESS ((void*)-1)

namespace Private
{
	template< class T >
	struct Node
	{
		T val;
		Node* next;
	};
}


template< class T , template < class > class AllocatePolicy >
class TUnisgnedHash : private AllocatePolicy< Private::Node< T > >
{
public:
	typedef Private::Node< T > Node;
	typedef AllocatePolicy< Private::Node< T > > AllocatePolicy;

	TUnisgnedHash( unsigned size = 128 )
		:m_hashSize(size)
	{
		m_hash = new Node*[ m_hashSize + 1];
		m_hash[ m_hashSize ] = (Node*)END_ADDRESS;

		std::fill_n( m_hash , m_hashSize , (Node*) NULL );
	}

	~TUnisgnedHash()
	{
		delete[] m_hash;
	}

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

		template< class T >
		friend class TUnisgnedHash;
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
		Node* ptr = AllocatePolicy::alloc();
		return ptr;
	}
	void freeNode(Node* ptr)
	{ 
		if( ptr )  
			AllocatePolicy::free( ptr ); 
	}

	unsigned m_collision;
	unsigned m_hashSize;
	Node**   m_hash;
};


#endif // TUnsignedHash_h__