#ifndef RedBlackTree_h__
#define RedBlackTree_h__

#include <algorithm>
#include <cassert>

template< class K , class V , class Cmp = std::less< K > >
class RedBlackTree
{
public:
	typedef K KeyType;
	typedef V ValueType;


	struct Node;

	RedBlackTree(){ mRoot = 0; mCount = 0; }
	~RedBlackTree()
	{



	}
	int   size(){ return mCount; }
	Node* insert( KeyType const& key , ValueType const& value )
	{
		Node* node;

		if ( mRoot )
		{
			Node* pNode = mRoot;
			while( 1 )
			{
				if ( Cmp()( key , pNode->key ) )
				{
					if ( pNode->left )
					{
						pNode = pNode->left;
					}
					else
					{
						node = new Node( key , value );
						pNode->linkLeft( *node );
						break;
					}
				}
				else if ( key == pNode->key )
					return NULL;
				else
				{
					if ( pNode->right )
					{
						pNode = pNode->right;
					}
					else
					{
						node = new Node( key , value );
						pNode->linkRight( *node );
						break;
					}
				}
			}
		}
		else
		{
			node = new Node( key , value );
			mRoot = node;
		}

		++mCount;
		balanceInsert( node );
		
		return node;
	}



	template< class Visitor >
	void visit( Visitor v )
	{
		if ( mRoot )
			visitImpl( v , *mRoot );
	}


	class Visitor
	{
	public:
		void onEnterNode( Node& node );
		void onLeaveNode( Node& node );
		void onEnterChild( Node& node , bool beLeft );
		void onLeaveChild( Node& node , bool beLeft );
		void onNode( Node& node );
	};
	template< class Visitor >
	static void visitImpl( Visitor v , Node& node )
	{
		v.onEnterNode( node );

		if ( node.left )
		{
			v.onEnterChild( node , true );
			visitImpl( v , *node.left );
			v.onLeaveChild( node , true );
		}

		v.onNode( node );

		if ( node.right )
		{
			v.onEnterChild( node , false );
			visitImpl( v , *node.right );
			v.onLeaveChild( node , false );
		}

		v.onLeaveNode( node );
	}

private:


	Node* getSibling( Node& node )
	{
		assert( node.parent );
		if ( node.parent->left == &node )
			return node.parent->right;
		else
			return node.parent->left;
	}
	Node** getLink( Node& node )
	{
		if ( node.parent )
		{
			if ( node.parent->left == &node )
				return &node.parent->left;
			else
				return &node.parent->right;
		}
		return &mRoot;
	}
	void balanceInsert( Node* node )
	{
		assert( node );
		for(;;)
		{
			Node* pNode = node->parent;
			if ( !pNode )
			{
				markBlack( *node );
				return;
			}

			if ( isBlack( *pNode ) )
				return;

			//parent is red
			Node* gNode = pNode->parent;
			if ( gNode )
			{
				Node* uncle = getSibling( *pNode );
				if ( uncle && isRed( *uncle ) )
				{
					markBlack( *pNode );
					markBlack( *uncle );
					markRed( *gNode );

					node = gNode;
				}
				else //uncle is black
				{
					if ( node == pNode->right && pNode == gNode->left )
					{
						//      G           G
						//    /   \       /   \
						// ( P )   U     N     U
						//    \         /
						//     N       P

						rotateLeft( *pNode , &gNode->left );
						node = pNode;
						pNode = node->parent;
					}
					else if ( node == pNode->left && pNode == gNode->right )
					{
						//      G          G
						//    /   \       / \
						//   U    ( P )  U   N
						//        /           \  
						//       N             P
						rotateRight( *pNode , &gNode->right );
						node = pNode;
						pNode = node->parent;
					}


					markBlack( *pNode );
					markRed( *gNode );
					if ( node == pNode->left && pNode == gNode->left )
					{
						//   ( GB )        PB
						//    /   \       /   \
						//   PR    UB    NR    GR
						//  /                   \
						// NR                    UB
						rotateRight( *gNode , getLink( *gNode ) );
					}
					else
					{
						assert( node == pNode->right && pNode == gNode->right );

						//   ( GB )          PB
						//    /   \         /   \
						//   UB    PR     GR     NR
						//          \    / 
						//          NR  UB         
						rotateLeft( *gNode , getLink( *gNode ) );
					}
					return;
				}
			}
		}
	}

	void  removeNode( Node& node )
	{
		Node* rNode;
		if ( node.left == 0 || node.right == 0 )
			rNode = &node;
		else
		{
			rNode = node.right;
			while( rNode->left )
			{
				rNode = rNode->left;
			}
		}

		Node* child = ( rNode->left ) ? rNode->left : rNode->right;
		if ( child )
		{
			assert( getSibling( *child ) == 0 );
			Node** link = getLink( *rNode );
			*link = child;
			child->parent = rNode->parent;
		}

		if ( isBlack( *rNode ) )
		{
			balanceErase( child , rNode );
		}

		if ( rNode != &node )
		{
			Node** link = getLink( node );
			*link = rNode;
			rNode->parent = node.parent;

			rNode->left  = node.left;
			if ( node.left )
				node.left->parent = rNode;

			rNode->right = node.right;
			if ( node.right )
				node.right->parent = rNode;
		}
	}

	void  balanceErase( Node* node , Node* rNode , Node** rNodeLink )
	{
		assert( isBlack( *rNode ) );

		if ( isRed( *node ) )
		{
			markBlack( *node );
		    return;
		}

		if ( !node->parent )
			return;

		// n & remove are black 
		Node* pNode = node->parent;
		if ( pNode->left == node )
		{
			Node* sNode = pNode->right;

			if ( isRed( sNode ) )
			{
				//   ( PB )         SB
				//    /  \         /  
				//   NB   SR     PR    
				//              /
				//             NB
				markRed( *pNode );
				markBlack( *sNode );
				rotateLeft( *pNode , getLink( *pNode ) );

				sNode = pNode->right;
			}

			if ( !sNode )
				return;
			if ( isBlack( sNode->left ) && isBlack( sNode->right ) )
			{
				if ( isRed( *pNode ) && isBlack( *sNode ) )
				{
					markRed( *sNode );
					markBlack( *pNode );
				}




			}
				
		}
		else
		{





		}

	}

	Node* findNode( KeyType const& key )
	{
		Node* node = mRoot;
		while( node )
		{
			if ( Cmp()( key , node->key ) )
			{
				node = node->left;
			}
			else if ( node->key == key )
			{
				return node;
			}
			else
			{
				node = node->right;
			}
		}
		return 0;
	}

	void rotateRight( Node& node , Node** link )
	{
		//     N             L       //
		//    / \           / \      //
		//   L   R    =>   Ll  N     //
		//  / \               / \    //
		// Ll  Lr            Lr  R   //
		Node* left = node.left;

		left->parent = node.parent;
		assert( link );
		*link = left;

		Node* leftR = left->right;
		node.left = leftR;
		if ( leftR )
			leftR->parent = &node;

		left->linkRight( node );
	}

	void rotateLeft( Node& node , Node** link )
	{
		//     N             R      //
		//    / \           / \     //
		//   L   R    =>   N   Rr   //
		//      / \       / \       //
		//     Rl  Rr    L   Rl     //

		Node* right = node.right;

		right->parent = node.parent;

		assert( link );
		*link = right;

		Node* rightL = right->left;
		node.right = rightL;
		if ( rightL )
			rightL->parent = &node;

		right->linkLeft( node );

	}


	struct Node
	{
		KeyType   key;
		ValueType value;
		Node*     left;
		Node*     right;
		Node*     parent;

		bool      bBlack;

		void linkLeft( Node& node )
		{
			left = &node;
			node.parent = this;
		}
		void linkRight( Node& node )
		{
			right = &node;
			node.parent = this;
		}
		Node( KeyType const& k , ValueType const& v )
			:key( k ),value( v )
		{
			left = right = parent = 0;
			bBlack = false;
		}
	};

	static void markRed( Node& node ){ node.bBlack = false; }
	static void markBlack( Node& node ){ node.bBlack = true; }
	static bool isRed( Node& node ){ return !isBlack( node ); }
	static bool isBlack( Node& node ){ return node.bBlack; }
	static bool isRed( Node* node ){ return node && !node->bBlack; }
	static bool isBlack( Node* node ){ return !isRed( node ); }


	Node* mRoot;
	int   mCount;
};

#endif // RedBlackTree_h__
