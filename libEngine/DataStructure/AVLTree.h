#ifndef AVLTree_h__
#define AVLTree_h__

#include <functional>

#include <iostream>
#include <cassert>

template< class T , template< class > class CmpFun = std::less >
class TAVLTree
{
public:
	typedef  T           value_type;
	typedef  CmpFun< T > CmpType;
	TAVLTree();

	void insert( unsigned val );
	void remove( unsigned val );
	void clear(){ clear( m_root ); }
	void print( );

private:

	struct Node
	{
		Node( T const& val , Node* pNode , Node** link );
		Node*  getNode( T value ){  return CmpType()( value , val )? prev : next ;  }
		void   setParent( Node* pNode , Node** link )
		{
			assert( pNode );
			assert( *link == pNode->next || 
				    *link == pNode->prev );

			relink( link , this );
			parent = pNode;
		}
		Node** getLink()
		{ 
			if ( !parent )
				return NULL;
			return parent->next == this ? &parent->next : &parent->prev ;
		}

		T      val;
		Node** link;
		Node*  parent;
		Node*  prev;
		Node*  next;
		char   heigh;
	};

	Node* m_root;

public:
	class iterator
	{
		bool operator == ( iterator iter ){  return cur == iter.cur;  }
		bool operator != ( iterator iter ){  return !( *this == iter );  }
		iterator operator++(int)
		{ 
			iterator temp = *this ; 
			_next(); 
			return temp; 
		}
		iterator operator++()
		{ 
			_next(); 
			return *this;
		}

		T& operator*()
		{
			return cur->val;
		}

	private:
		iterator( Node* node )
			:node( node ){}

		void _next()
		{
			if ( cur->prev )
			{
				cur = cur->prev;
			}
			else if ( cur->next )
			{
				cur = cur->next;
			}
			else
			{
				Node* pNode = NULL;
				while( ( pNode = cur->parent )  && 
					     pNode->next == cur )
				{ 
					cur = pNode;
				}
				if ( !pNode )
					cur = NULL;
				else
					cur = cur->next;
			}

		}
		Node* cur;
	};

private:

	int computeHeight( Node* destNode ){  return computeHeight( m_root , destNode );  }
	int computeHeight( Node* curNode , Node* destNode );

	void  rotate( Node* oldR , Node* newR , Node** chLink );

	//void  rotateRight( Node* node ){  rotate( node , node->next , &node->next->prev );  } 
	void  balance( Node* curNode , Node* destNode );
	void  clear( Node* node );
	void  print( Node* node , int depth  );


	static void  cutlink( Node* node );
	static void  relink( Node** link , Node* node );
	static int   getHeight( Node* node ){  return ( node ) ? node->heigh : -1;  }
	static Node* createNode( unsigned val , Node* pNode , Node** link ){  return new Node( val , pNode , link );  }
	static void  destoryNode( Node* node );

};



using namespace std;

#define AVLTREE_TEMPLATE_ARG()\
	template< class T , template< class > class CmpFun >

#define AVLTREE_FUN( RetType )\
	AVLTREE_TEMPLATE_ARG()\
	RetType TAVLTree< T , CmpFun >::

AVLTREE_TEMPLATE_ARG()
TAVLTree< T, CmpFun >::Node::Node(T const& val, Node* pNode, Node** link)
	:val(val), link(link)
	, prev(NULL), next(NULL)
	, parent(pNode)
{
	assert(link);
	*link = this;
	heigh = 0;
}

AVLTREE_TEMPLATE_ARG()
TAVLTree< T, CmpFun >::TAVLTree()
	:m_root(NULL)
{

}

AVLTREE_FUN(void)
print(Node* node, int depth)
{
	if( !node )
	{
		cout << endl;
		return;
	}

	if( node->heigh == getHeight(node->next) ||
	   node->heigh == getHeight(node->prev) )
	{
		int i = 0;
	}

	if( node->prev || node->next )
		print(node->prev, depth + 1);

	for( int i = 0; i < depth; ++i )
		cout << "   ";

	cout << "[" << node->val << ": h =" << (int)node->heigh << "]" << endl;

	if( node->prev || node->next )
		print(node->next, depth + 1);
}

AVLTREE_FUN(void)
print()
{
	print(m_root, 0);
	cout << "==================" << endl;
}

AVLTREE_FUN(void)
remove(unsigned val)
{
	Node** link = &m_root;

	Node* parent = NULL;
	Node* pn = NULL;

	while( pn = *link )
	{
		if( CmpType()(val, pn->val) )
		{
			parent = pn;
			link = &pn->prev;
			continue;
		}
		if( CmpType()(pn->val, val) )
		{
			parent = pn;
			link = &pn->next;
			continue;
		}
		break;
	}

	if( !pn )
		return;

	*link = NULL;
	Node* upNode = NULL;

	Node* rNode;
	Node* mNode;

	if( !parent || parent->next == pn )
	{
		rNode = pn->next;
		mNode = pn->prev;
	}
	else
	{
		rNode = pn->prev;
		mNode = pn->next;
	}

	if( rNode )
	{
		Node** link = pn->getLink();
		relink(link, rNode);
		if( mNode )
		{
			Node** moveLink;
			if( rNode == pn->next )
			{
				moveLink = &rNode->prev;
				while( *moveLink )
				{
					moveLink = &(*moveLink)->prev;
				}
			}
			else
			{
				moveLink = &rNode->next;
				while( *moveLink )
				{
					moveLink = &(*moveLink)->next;
				}
			}
			relink(moveLink, mNode);
			upNode = mNode;
		}
		else
		{
			upNode = rNode;
		}

	}
	else if( mNode )
	{
		relink(pn->getLink(), mNode);
		upNode = mNode;
	}
	else
	{
		relink(pn->getLink(), NULL);
		upNode = parent;
	}

	destoryNode(pn);
	if( upNode )
	{
		computeHeight(upNode);
		balance(m_root, upNode);
	}
}

AVLTREE_FUN(void)
insert(unsigned val)
{
	Node** link = &m_root;
	Node*  parent = NULL;

	while( Node* pn = *link )
	{
		if( CmpType()(val, pn->val) )
		{
			parent = pn;
			link = &pn->prev;
			continue;
		}
		if( CmpType()(pn->val, val) )
		{
			parent = pn;
			link = &pn->next;
			continue;
		}
		return;
	}

	Node* newNode = createNode(val, parent, link);

	computeHeight(newNode);
	balance(m_root, newNode);
}

AVLTREE_FUN(void)
balance(Node* curNode, Node* destNode)
{
	Node* chNode = curNode->getNode(destNode->val);

	if( chNode )
		balance(chNode, destNode);

	Node* prev = curNode->prev;
	Node* next = curNode->next;

	int factor = getHeight(next) - getHeight(prev);

	//assert ( abs( factor ) <= 2 );
	if( factor <= -2 )
	{
		assert(prev);
		Node* nodeLL = prev->prev;
		Node* nodeLR = prev->next;
		if( getHeight(nodeLL) >= getHeight(nodeLR) )
		{
			rotate(curNode, prev, &prev->next);
		}
		else
		{
			rotate(prev, nodeLR, &nodeLR->prev);
			rotate(curNode, nodeLR, &nodeLR->next);
			prev->heigh -= 1;
			computeHeight(nodeLR, nodeLR);
		}
		computeHeight(curNode);
	}
	else if( factor >= 2 )
	{
		assert(next);
		Node* nodeRL = next->prev;
		Node* nodeRR = next->next;
		if( getHeight(nodeRR) >= getHeight(nodeRL) )
		{
			rotate(curNode, next, &next->prev);
		}
		else
		{
			rotate(next, nodeRL, &nodeRL->next);
			rotate(curNode, nodeRL, &nodeRL->prev);
			next->heigh -= 1;
			computeHeight(nodeRL, nodeRL);
		}
		computeHeight(curNode);
	}
}

AVLTREE_FUN(void)
rotate(Node* oldR, Node* newR, Node** chLink)
{
	Node** link = newR->getLink();
	Node* chNode = *chLink;
	relink(oldR->getLink(), newR);
	newR->parent = oldR;
	relink(chLink, oldR);
	oldR->parent = newR;
	relink(link, chNode);
	if( chNode )
		chNode->parent = oldR;
}

AVLTREE_FUN(int)
computeHeight(Node* curNode, Node* destNode)
{
	assert(curNode);

	if( curNode != destNode )
		computeHeight(curNode->getNode(destNode->val), destNode);

	curNode->heigh = max(getHeight(curNode->next),
						 getHeight(curNode->prev)) + 1;
	return curNode->heigh;
}


AVLTREE_FUN(void)
cutlink(Node* node)
{
	Node** link = node->getLink();
	if( link )
		*(link) = NULL;
}

AVLTREE_FUN(void)
clear(Node* node)
{
	if( node->prev )
		clear(node->prev);
	if( node->next )
		clear(node->next);

	relink(node->link, NULL);
	destoryNode(node);
}

AVLTREE_FUN(void)
destoryNode(Node* node)
{
	assert(*node->getLink() != node);
	delete node;
}

AVLTREE_FUN(void)
relink(Node** link, Node* node)
{
	if( link )
		*link = node;
}

#undef AVLTREE_FUN
#undef AVLTREE_TEMPLATE_ARG

#endif // AVLTree_h__