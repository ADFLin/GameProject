

#ifndef TQuadTreeTile_h__
#define TQuadTreeTile_h__

#include "TQuadTree.h"
#include "TVector2.h"

typedef TVector2< int > PosType;


struct TileData
{
	PosType  pos;
	unsigned length;


	PosType getCenterPos()
	{
		unsigned halfLen = length / 2;
		return pos + PosType( halfLen , halfLen );
	}
};

struct LeafData 
{
	int value;

};

class TQuadTreeTile : public TQuadTree< TQuadTreeTile , LeafData , TileData >
{
public:
	typedef TVector2< int > PosType;

	TQuadTreeTile(){ }

	void checkSplit( LeafNode* node , int* map )
	{
		PosType const& pos = node->pos;
		int val = getValue( node );

		for(int i = pos.x ; i < pos.x + node->length ; ++i )
		for(int j = pos.y ; j < pos.y + node->length ; ++j )
		{
			int index = i + j * m_MapSize;
			if ( map[index] != val )
			{
				GrayNode* grayNode = splitNode( node );
				onSplitNodeWithData( grayNode , map );
				return;
			}
		}
	}

	static LeafNode* getNode( Node* node , PosType& pos , int length )
	{
		if ( node->isLeaf() )
			return static_cast< LeafNode* >( node );

		GrayNode* grayNode = static_cast< GrayNode* >( node );

		int subLength = length / 2;

		if ( pos.x >= subLength )
		{
			pos.x -= subLength;
			if ( pos.y >= subLength )
			{
				pos.y -= subLength;
				return getNode( grayNode->child[ND_NE] , pos , subLength);
			}
			else
				return getNode( grayNode->child[ND_SE] , pos , subLength);
		}
		else
		{
			if ( pos.y >= subLength )
			{
				pos.y -= subLength;
				return getNode( grayNode->child[ND_NW] , pos , subLength);
			}
			else
				return getNode( grayNode->child[ND_SW] , pos , subLength);
		}
	}

	LeafNode* getNode(PosType const& pos )
	{
		if ( 0 <= pos.x && pos.x < m_MapSize &&
			 0 <= pos.y && pos.y < m_MapSize)
		{
			PosType temp = pos;
			return getNode( getRoot() , temp , m_MapSize );
		}
		return NULL;
	}

	void buildMap( int* map , unsigned size )
	{
		m_MapSize = size;

		mMap = map;

		assert( getRoot()->isLeaf() );

		LeafNode* root = ( LeafNode* ) getRoot();
		root->length = size;
		root->pos.x = 0;
		root->pos.y = 0;

		checkSplit( root , map );

	}


	void onSplitNodeWithData( GrayNode* node , int* map )
	{
		int value = static_cast< LeafNode* >( node->child[0] )->value;

		node->length = node->child[0]->length;
		node->pos    = node->child[0]->pos;

		PosType const& pos = node->pos;
		long subLen = node->length / 2;

		LeafNode* child;

#define SETUP_NODE( DIR , PX , PY )   \
		child = static_cast< LeafNode* >( node->getChild( DIR ) );\
		child->value  = 0;                 \
		child->length = subLen;            \
		child->pos    = PosType( PX , PY );\
		checkSplit( child , map );          \

		SETUP_NODE( ND_SW , pos.x , pos.y );
		SETUP_NODE( ND_SE , pos.x + subLen , pos.y );
		SETUP_NODE( ND_NW , pos.x , pos.y + subLen );
		SETUP_NODE( ND_NE , pos.x + subLen , pos.y + subLen );

#undef SETUP_NODE
	}

	int  getValue( Node* node )
	{
		return mMap[ node->pos.x + m_MapSize * node->pos.y ];
	}

	void setValue( PosType const& pos , int value )
	{
		LeafNode* node = getNode( pos );
		if ( node == NULL )
			return;

		if ( node->length == 1 )
		{
			node->value = value;
		}
		else
		{
			//GrayNode* splitNode = splitNode( node );
		}
	}


	int*      mMap;
	unsigned  m_MapSize;
	int       m_NumNode;

};

#endif // TQuadTreeTile_h__
