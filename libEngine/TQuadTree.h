#ifndef TQuadTree_h__
#define TQuadTree_h__


enum NodeFlag
{
	ND_NW = 0,
	ND_NE = 1,
	ND_SW = 2,
	ND_SE = 3,

	NS_NORTH = 0 ,
	NS_EAST  = 1 ,
	NS_SOUTH = 2 ,
	NS_WEST  = 3 ,
};

int const DirAdj[4][4] =
{
	{1,1,0,0},
	{0,1,0,1},
	{0,0,1,1},
	{1,0,1,0},
};

int const DirReflect[4][4] = 
{
	ND_SW , ND_SE , ND_NW , ND_NE ,
	ND_NE , ND_NW , ND_SE , ND_SW ,
	ND_SW , ND_SE , ND_NW , ND_NE ,
	ND_NE , ND_NW , ND_SE , ND_SW ,
};

int const OpQuad[ 4 ] =
{
	ND_SE , ND_SW , ND_NE , ND_NW ,
};


struct NullData{};

template< class T , class LeafData , class CommonData = NullData >
class TQuadTree
{
	T* _this(){ return static_cast< T* >( this ); }
public:
	TQuadTree()
	{
		mHead.setChild( 0 , createLeafNode() );
	}
	~TQuadTree()
	{
		clearNode( mHead.child[0] );
	}

	class GrayNode;
	class Node : public CommonData
	{
	public:
		GrayNode* getParent(){ return parent; }
		bool      isLeaf()   { return beLeaf; }
		char      getDir()   { return dir; }
	protected:
		bool      beLeaf;
		char      dir;
		GrayNode* parent;
		friend class GrayNode;
		friend class TQuadTree;
	};


	class GrayNode : public Node
	{
	public:
		GrayNode(){ beLeaf = false; }
		Node*     child[4];
		Node*     getChild( int dir ){ return child[ dir ]; }
		void      setChild( int dir , Node* node )
		{
			child[ dir ] = node;
			node->parent = this;
			node->dir    = dir;
		}
	};

	class LeafNode : public Node , public LeafData
	{
	public:
		LeafNode(){ beLeaf = true; }
	};


//////////override function//////////////
protected:
	void onMergeNode( GrayNode* node ){}
	void onClearNode( Node* node ){}

	GrayNode* createGrayNode(){  return new GrayNode;  }
	LeafNode* createLeafNode(){  return new LeafNode;  }
	void      destoryNode( LeafNode* node ){  delete node;  }
	void      destoryNode( GrayNode* node ){  delete node;  }
////////////////////////////////

public:
	Node*      getRoot(){ return mHead.getChild(0); }
	GrayNode*  splitNode( LeafNode* node );

	void       mergeNode( GrayNode* node );
	void       clearNode( Node* node );

	void      _mergeNode( Node* node )
	{
		if ( node->isLeaf() )
			return;
		mergeNode( (GrayNode*) node );
	}

	template< int Side >
	static Node* getNeighborAdjNode( Node* node );

	template< int Side , class TFunc >
	static void visitNode( Node* node , TFunc& fun );

	template< class TFunc >
	static void  visitNeighborNode( Node* node, TFunc& fun );

	GrayNode  mHead;
};


#define TEMPLATE_PARAM \
	template< class I , class L , class C >

#define CLASS_PARAM I,L,C

TEMPLATE_PARAM
template< int Side , class TFunc >
void TQuadTree< CLASS_PARAM >::visitNode( Node* node , TFunc& func )
{
	if ( node->isLeaf() )
	{
		func( node );
		return;
	}

	if ( DirAdj[Side][ 0 ] )
		visitNode< Side >( static_cast< GrayNode* >( node )->getChild(0) , func );
	if ( DirAdj[Side][ 1 ] )
		visitNode< Side >( static_cast< GrayNode* >( node )->getChild(1) , func );
	if ( DirAdj[Side][ 2 ] )
		visitNode< Side >( static_cast< GrayNode* >( node )->getChild(2) , func );
	if ( DirAdj[Side][ 3 ] )
		visitNode< Side >( static_cast< GrayNode* >( node )->getChild(3) , func );
}

TEMPLATE_PARAM
template< int Side >
typename TQuadTree<CLASS_PARAM>::Node* 
TQuadTree<CLASS_PARAM>::getNeighborAdjNode( Node* node )
{
	Node* refNode = node->parent;
	if ( refNode && DirAdj[ Side ][ node->dir ] )
		refNode = getNeighborAdjNode< Side >( refNode );

	if ( refNode && !refNode->isLeaf() )
	{
		return static_cast< GrayNode* >( refNode )->child[ DirReflect[ Side ][ node->dir ] ];
	}

	return refNode;
}

TEMPLATE_PARAM
template< class TFunc >
void TQuadTree<CLASS_PARAM>::visitNeighborNode( Node* node, TFunc& func )
{
	Node* p;

	p = getNeighborAdjNode< NS_NORTH >( node );
	if ( p ) visitNode< NS_SOUTH >( p ,func );

	p = getNeighborAdjNode< NS_EAST >( node );
	if ( p ) visitNode< NS_WEST >(  p ,func );

	p = getNeighborAdjNode< NS_SOUTH >( node );
	if ( p ) visitNode< NS_NORTH >( p , func );

	p = getNeighborAdjNode< NS_WEST >( node );
	if ( p ) visitNode< NS_EAST >( p , func );
}


TEMPLATE_PARAM
void TQuadTree<CLASS_PARAM>::mergeNode( GrayNode* node )
{
	for( int i = 0 ; i < 4 ; ++i )
	{
		if ( !node->child[i]->isLeaf() )
			mergeNode( ( GrayNode*)node->child[i] );
	}

	_this()->onMergeNode( node );

	_this()->destoryNode( (LeafNode*)node->child[1] );
	_this()->destoryNode( (LeafNode*)node->child[2] );
	_this()->destoryNode( (LeafNode*)node->child[3] );

	node->parent->setChild( node->dir , node->child[0] );
	_this()->destoryNode( node );
}

TEMPLATE_PARAM
void TQuadTree<CLASS_PARAM>::clearNode( Node* node )
{
	onClearNode( node );

	if ( node->isLeaf() )
	{
		_this()->destoryNode( (LeafNode*)node );
	}
	else
	{
		GrayNode* grayNode = static_cast< GrayNode* >( node );
		for( int i = 0 ; i < 4 ; ++i )
			clearNode( grayNode->getChild( i ) );

		_this()->destoryNode( grayNode );
	}
}

TEMPLATE_PARAM
typename TQuadTree<CLASS_PARAM>::GrayNode*  
TQuadTree<CLASS_PARAM>::splitNode( LeafNode* node )
{
	GrayNode* grayNode = createGrayNode();
	node->getParent()->setChild( node->getDir() , grayNode );

	grayNode->setChild( 0 , node );

	LeafNode* child;
	child = _this()->createLeafNode();
	grayNode->setChild( 1 , child );

	child = _this()->createLeafNode();
	grayNode->setChild( 2 , child );

	child = _this()->createLeafNode();
	grayNode->setChild( 3 , child );

	return grayNode;
}


#undef TEMPLATE_PARAM
#undef CLASS_PARAM

#endif // TQuadTree_h__