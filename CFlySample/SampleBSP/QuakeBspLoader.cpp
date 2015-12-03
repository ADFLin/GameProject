#include "QuakeBspLoader.h"

#include "CFlyHeader.h"

#include "FileSystem.h"

#include <fstream>
#include <algorithm>

#define MAKE_( a , b ) ( ( a << 8 ) | b )
#define MAKE_ID( a , b , c , d ) ( (uint32) MAKE_( MAKE_( MAKE_( uint8( d ) , uint8( c ) ) , uint8( b ) ) , uint8( a )) )

char* AllocFileData( char const* path )
{
	using namespace std;

	ifstream fs( path , ios::binary );
	if ( !fs.is_open() )
		return nullptr;

	fs.seekg( 0 , ios::beg );
	unsigned size = fs.tellg();
	fs.seekg( 0 , ios::end );
	size = (unsigned) fs.tellg() - size;

	char* data = new char[ size ];

	fs.seekg( 0 , ios::beg );
	fs.read( data  , size  );

	return data;
}

void FreeFileData( char* data )
{
	if ( data )
		delete [] data;
}

int nextNumberOfPow2( int n )
{
	assert( sizeof(n) == 4 );

	n--;
	n |= n >> 1;   
	n |= n >> 2;
	n |= n >> 4;
	n |= n >> 8;
	n |= n >> 16;
	n++;
	return n;
}

bool ImageMergeHelper::addImage( int id , int w , int h )
{
	mRect.w = w;
	mRect.h = h;
	mRect.x = 0;
	mRect.y = 0;

	Node* node = insertNode( mRoot );
	if ( !node )
		return false;

	if ( id >= mImageNodeMap.size() )
		mImageNodeMap.resize( id + 1 , nullptr );

	assert( node->imageID == ErrorImageID );
	node->imageID = id;
	mImageNodeMap[ id ] = node;
	return true;
}

ImageMergeHelper::Node* ImageMergeHelper::insertNode( Node* curNode )
{
	assert( curNode );

	if ( curNode->imageID != ErrorImageID )
		return nullptr;

	if ( !curNode->isLeaf() )
	{
		Node* newNode = insertNode( curNode->children[0]  );
		if ( !newNode )
			newNode = insertNode( curNode->children[1] );
		return newNode;
	}

	Rect& curRect = curNode->rect;

	if ( mRect.w == curRect.w && mRect.h == curRect.h )
		return curNode;

	if ( mRect.w > curRect.w || mRect.h > curRect.h )
		return nullptr;

	int dw = curRect.w - mRect.w ;
	int dh = curRect.h - mRect.h ;

	if ( dw > dh )
	{
		curNode->children[0] = createNode( curRect.x , curRect.y , mRect.w , curRect.h );
		curNode->children[1] = createNode( curRect.x + mRect.w , curRect.y , dw , curRect.h);
	}
	else
	{
		curNode->children[0] = createNode( curRect.x , curRect.y , curRect.w , mRect.h );
		curNode->children[1] = createNode( curRect.x , curRect.y + mRect.h , curRect.w , dh );
	}

	return insertNode( curNode->children[0] );
}

ImageMergeHelper::Node* ImageMergeHelper::createNode( int x , int y , int w , int h )
{
	Node* node = new Node;
	node->children[0] = node->children[1] = nullptr;
	node->rect.x = x;
	node->rect.y = y;
	node->rect.w = w;
	node->rect.h = h;
	node->imageID = ErrorImageID;
	return node;
}

ImageMergeHelper::ImageMergeHelper( int w , int h )
{
	mRoot = createNode( 0 , 0 , w , h );
}

ImageMergeHelper::~ImageMergeHelper()
{
	destoryNode( mRoot );
}

void ImageMergeHelper::destoryNode( Node* node )
{
	if ( node->children[0] )
		destoryNode( node->children[0] );
	if ( node->children[1] )
		destoryNode( node->children[1] );
	delete node;
}

Wad3FileData::Wad3FileData()
{
	mData = nullptr;
}

Wad3FileData::~Wad3FileData()
{
	FreeFileData( mData );
}

bool Wad3FileData::load( char const* path )
{
	mData = AllocFileData( path );

	if ( !mData )
		return false;

	WAD3::Header* header = getHeader();

	if ( header->identification != MAKE_ID('W','A','D','3' ) )
		return false;

	mLumps = reinterpret_cast< WAD3::LumpInfo* >( mData + header->infoTableOffset );
	return true;
}

Texture* Wad3FileData::createTexture( World* world , char const* name )
{
	for( int i = 0; i < getHeader()->numLumps ; ++i )
	{
		WAD3::LumpInfo* lump = mLumps + i;
		if ( strcmp( name , lump->name ) == 0 )
			return createTexture( world , lump );
	}
	return nullptr;
}

Texture* Wad3FileData::createTexture( World* world , WAD3::LumpInfo* lumpInfo )
{
	switch( lumpInfo->type )
	{
	case WAD3::TYP_MIPTEX:
		{
			WAD3::MipTexInfo* texInfo = reinterpret_cast< WAD3::MipTexInfo* >( mData + lumpInfo->fileOffset );
			int const minMapLevel = 0;

			if ( !texInfo->mipMapOffset[0] )
				return nullptr;

			unsigned char* idxData = reinterpret_cast< unsigned char* >( 
				reinterpret_cast< char* >( texInfo ) + texInfo->mipMapOffset[0] );

			return createMipTexture( world , reinterpret_cast< char* >( texInfo ) , texInfo , minMapLevel );
		}
		break;

	}

	return nullptr;
}

Texture* Wad3FileData::createMipTexture( World* world , char* data , WAD3::MipTexInfo* texInfo , int const minMapLevel /*= 0 */ )
{
	if ( !texInfo->mipMapOffset[0] )
		return nullptr;

	unsigned char* idxData = reinterpret_cast< unsigned char* >( 
		data + texInfo->mipMapOffset[0] );

	uint32 length[ 4 ] =
	{
		( texInfo->width ) * ( texInfo->height ) ,
		( texInfo->width / ( 1 << 1 ) ) * ( texInfo->height / ( 1 << 1  ) ),
		( texInfo->width / ( 1 << 2 ) ) * ( texInfo->height / ( 1 << 2  ) ),
		( texInfo->width / ( 1 << 3 ) ) * ( texInfo->height / ( 1 << 3  ) ),
	};

	std::vector< uint8 > rawData( 4 * length[ minMapLevel ] );
	WAD3::ColorKey* pal = reinterpret_cast< WAD3::ColorKey* >(
		data + sizeof( WAD3::MipTexInfo ) + length[0] + length[1] + length[2] + length[3] + 2 );

	bool useColorKey = fillImageDataARGB( &rawData[0] , length[minMapLevel] , idxData , pal );

	MaterialManager* manager = world->_getMaterialManager();
	
	Texture* tex = manager->createTexture2D( 
		texInfo->name , CF_TEX_FMT_ARGB32 , &rawData[0] , 
		texInfo->width , texInfo->height , 
		useColorKey ? &Color3ub(0,0,255) : nullptr );

	return tex;
}

bool Wad3FileData::fillImageDataARGB( unsigned char* data , unsigned length , unsigned char* idxData, WAD3::ColorKey* pal )
{
	bool useColorKey = false;
	int  colorKeyIdx = -1;
	for( int i = 0 ; i < 256 ; ++i )
	{
		if ( pal[i].r == 0 && pal[i].g == 0  && pal[i].b == 0xff )
		{
			colorKeyIdx = i;
			break;
		}
	}

	if ( colorKeyIdx != -1 )
	{
		for( int i = 0 ; i < length ; ++i )
		{
			if ( idxData[i] == colorKeyIdx )
			{
				*(data++) =  0;
				*(data++) =  0;
				*(data++) =  0;
				*(data++) =  0;
			}
			else
			{
				WAD3::ColorKey& key = pal[ idxData[i] ];
				*(data++) =  key.b;
				*(data++) =  key.g;
				*(data++) =  key.r;
				*(data++) =  255;
			}
		}

		return true;
	}
	else
	{
		for( int i = 0 ; i < length ; ++i )
		{
			WAD3::ColorKey& key = pal[ idxData[i] ];
			*(data++) =  key.b;
			*(data++) =  key.g;
			*(data++) =  key.r;
			*(data++) =  255;
		}
		return false;
	}

	return false;
}

void HLBspFileDataV30::cleanup()
{
	FreeFileData( mData );
	mData = nullptr;

	for( Wad3FileList::iterator iter = mWadLoader.begin();
		iter != mWadLoader.end() ; ++iter )
	{
		delete *iter;
	}
	mWadLoader.clear();
	delete mHelper;
	mHelper = nullptr;

	mTexEmpty.release();
	mTexLightMap.release();
}

bool HLBspFileDataV30::load( char const* path )
{
	assert( mScene );

	mData = AllocFileData( path );

	if ( !mData )
		return false;

	if ( getHeader()->version != 30 )
		return false;

	mHeader  = getHeader();
	mEntity  = getLumpData( BspV30::LUMP_ENTITIES );

	mFaces   = reinterpret_cast< BspV30::face_t* >( getLumpData( BspV30::LUMP_FACES ) );
	mPlanes  = reinterpret_cast< BspV30::plane_t* >( getLumpData( BspV30::LUMP_PLANES ) );
	mEdges   = reinterpret_cast< BspV30::edge_t* >( getLumpData( BspV30::LUMP_EDGES ) );
	mVertexs = reinterpret_cast< BspV30::vertex_t* >( getLumpData( BspV30::LUMP_VERTICES ) );
	mTexInfo = reinterpret_cast< BspV30::texinfo_t* >( getLumpData( BspV30::LUMP_TEXINFO ) );
	mNodes   = reinterpret_cast< BspV30::node_t* >( getLumpData( BspV30::LUMP_NODES ) );
	mLeaves  = reinterpret_cast< BspV30::leaf_t* >( getLumpData( BspV30::LUMP_LEAVES ) );
	mLightMaps = reinterpret_cast< char* >( getLumpData( BspV30::LUMP_LIGHTING ) );
	mModels  = reinterpret_cast< BspV30::model_t* >( getLumpData( BspV30::LUMP_MODELS ) );

	mTexLumpHeader = reinterpret_cast< BspV30::texture_lump_header* >( getLumpData( BspV30::LUMP_TEXTURES ) );
	mMipTex        = reinterpret_cast< BspV30::miptex_t* >( getLumpData( BspV30::LUMP_TEXTURES ) );

	int numLeaf = getLumpDataLength( BspV30::LUMP_LEAVES ) / sizeof( BspV30::leaf_t );
	int numVertex = getLumpDataLength( BspV30::LUMP_VERTICES ) / sizeof( BspV30::vertex_t );

	//mVertexList.assign( 
	//	reinterpret_cast< Vector3* >( mVertexs ) , 
	//	reinterpret_cast< Vector3* >( mVertexs ) + numVertex );

	loadEntity();

	{
		Wad3FileData* loader = new Wad3FileData;
		loader->load( "../Data/HL/cs_dust.wad" );
		mWadLoader.push_back( loader );
	}
	{
		Wad3FileData* loader = new Wad3FileData;
		loader->load( "../Data/HL/decals.wad" );
		mWadLoader.push_back( loader );
	}
	{
		Wad3FileData* loader = new Wad3FileData;
		loader->load( "../Data/HL/halflife.wad" );
		mWadLoader.push_back( loader );
	}

	mTextureMap.resize( mTexLumpHeader->numMiptex );

	DWORD color = 0xffffffff;
	mTexEmpty = mWorld->_getMaterialManager()->createTexture2D( "empty" , CF_TEX_FMT_ARGB32 , (BYTE*)&color , 1 , 1 );

	buildLightMap();

	//obj->createLines( nullptr , CF_LINE_SEGMENTS , mVertexList.size() , (float*)mVertexList[0] );
	return true;
}

void HLBspFileDataV30::buildLightMap()
{
	int lightMapLen = getLumpDataLength( BspV30::LUMP_LIGHTING ) / 3;

	int mapSize = nextNumberOfPow2( (int)Math::Sqrt( lightMapLen  ) );

	mHelper = new BspLightMapMergeHelper( mapSize ,  mapSize );

	int failCount = 0;

	int numFace = getLumpDataLength( BspV30::LUMP_FACES ) / sizeof( BspV30::face_t );
	for( int i = 0;i < numFace ; ++i )
	{
		BspV30::face_t* face = mFaces + i;

		if( face->lightStyles[0] != 0 )
			continue;

		BspV30::texinfo_t* texInfo = mTexInfo + face->texinfo;

		int lw,lh;
		float min[2];
		float max[2];

		calcTextureBoundRect( face , texInfo , min , max );
		// compute lightmap size

		int size[2];
		for( int c = 0; c < 2; c++) 
		{
			float tmin = (float) floor(min[c]/16.0f);
			float tmax = (float) ceil(max[c]/16.0f);

			size[c] = (int) (tmax-tmin);
		}
		lw = size[0]+1;
		lh = size[1]+1;

		int lsz = lw*lh*3;
		// generate lightmaps texture
		for(int c = 0; c < 1 ; c++) 
		{
			if( face->lightStyles[c] == -1) 
				break;

			int id = i + c * numFace;

			if ( mHelper->addImage( id , lw , lh ) )
			{
				uint8* src = (uint8*)(mLightMaps + face->lightOffset + ( lsz * c ) );
				mHelper->fillImageData( id ,src , 50 );
			}
			else
			{
				++failCount;
			}		
		}
	}

	mTexLightMap = mHelper->createTexture( mWorld );
#if 1
	Object* obj = mScene->createObject();
	Material* mat = mWorld->createMaterial();
	mat->addTexture( 0,0,mTexLightMap );
	obj->createPlane( mat , 100 , 100 );
#endif
}

void HLBspFileDataV30::calcTextureBoundRect( BspV30::face_t* face , BspV30::texinfo_t* texInfo , float min[2] , float max[2] )
{
	min[0]=min[1]= 100000;
	max[0]=max[1]=-100000;

	BspV30::surfedge* surfEdgeList = 
		reinterpret_cast< BspV30::surfedge* >( getLumpData( BspV30::LUMP_SURFEDGES ) ) + face->firstEdge;

	for(int c = 0; c < face->numEdges; c++) 
	{
		int edgeIdx = surfEdgeList[c];

		Vector3* v;
		if ( edgeIdx >= 0 )
		{
			BspV30::edge_t* edge = mEdges + edgeIdx; 
			v = reinterpret_cast< Vector3* >( mVertexs + edge->vertex[0] );
		}
		else
		{
			BspV30::edge_t* edge = mEdges - edgeIdx; 
			v = reinterpret_cast< Vector3* >( mVertexs + edge->vertex[1] );
		}

		// compute extents
		for(int x = 0; x < 2; x++) 
		{
			float d = VectorDot( *v , &texInfo->m[x][0] );

			if(d < min[x]) min[x] = d;
			if(d > max[x]) max[x] = d;
		}            
	}
}

Texture* HLBspFileDataV30::getTexture( BspV30::texinfo_t* texInfo )
{
	//return mTexEmpty;

	BspV30::miptex_t*  mipTex = reinterpret_cast< BspV30::miptex_t* >( 
		getLumpData( BspV30::LUMP_TEXTURES ) + mTexLumpHeader->miptexOffset[texInfo->miptex ] );

	Texture* tex = mTextureMap[ texInfo->miptex ];
	if ( tex )
		return tex;

	if ( mipTex->offsets[0] )
	{
		tex = Wad3FileData::createMipTexture(
			mWorld , 
			reinterpret_cast< char* >( mipTex ) , 
			reinterpret_cast< WAD3::MipTexInfo*>( mipTex ) , 0 );

		if ( tex ) 
			goto sucess;
	}


	tex = createTextureFromWAD( mipTex->name );

	if ( tex ) 
		goto sucess;

	tex = mTexEmpty;

sucess:
	mTextureMap[ texInfo->miptex ] = tex;
	return tex;
}

void HLBspFileDataV30::loadLeaf( unsigned idxLeaf , BspLeaf& bspLeaf , int numLeaf )
{
	BspV30::leaf_t* leaf = &mLeaves[ idxLeaf ];

	loadPVS( leaf , bspLeaf , numLeaf );
	bspLeaf.object = mScene->createObject();
	loadSurface( leaf->firstMarkSurface , leaf->numMarkSurfaces , bspLeaf.object );
}

Texture* HLBspFileDataV30::createTextureFromWAD( char const* name )
{
	for( int i = 0 ; i < mWadLoader.size();++i )
	{
		Texture* tex = mWadLoader[i]->createTexture( mWorld , name );
		if ( tex )
			return tex;
	}
	return nullptr;
}

void HLBspFileDataV30::setScene( Scene* scene )
{
	mScene = scene;
	mWorld = scene->getWorld();
}


void HLBspFileDataV30::loadPVS( BspV30::leaf_t* leaf , BspLeaf& bspLeaf , int numLeaf )
{
	int visOffset = leaf->visOffset;
	int visLumpSize = getLumpDataLength( BspV30::LUMP_VISIBILITY );
	assert ( leaf );

	int nRowBytes = (( numLeaf ) + 7) >> 3;
	// Pointer to compressed cluster visibility data
	bspLeaf.pvs.resize( nRowBytes , 0 );
	if ( visOffset < 0 )
		return;

	uint8* pCompVis = reinterpret_cast< uint8*>( getLumpData( BspV30::LUMP_VISIBILITY ) ) + visOffset;

	assert( visOffset < visLumpSize );

	int nCompMax = visLumpSize - visOffset;

	// Decompress visibility data into single cluster row
	// RLE encoding
	for (int n=0, nDComp=0; n<nRowBytes && nDComp<nCompMax; n++, nDComp++)
	{
		if (pCompVis[nDComp] == 0)
		{
			n += pCompVis[++nDComp] - 1;
		}
		else
		{
			bspLeaf.pvs[n] = pCompVis[nDComp];
		}
	}
}

VertexType const baseType = CFVT_XYZ | CFVF_TEX1( 2 );
VertexType const lightMapType = CFVT_XYZ | CFVF_TEX2( 2 , 2 );
void HLBspFileDataV30::loadModelSurface( uint16 fristFace , uint16 numFace , Object* obj )
{
	MeshBuilder builder( baseType );
	for( int i = 0; i < numFace ; ++i )
	{
		int idxFace = fristFace + i;
		loadFace( idxFace , builder , obj );
	}
}

void HLBspFileDataV30::loadSurface( uint16 fristSurface , uint16 numSurfaces , Object* obj )
{
	BspV30::marksurface* markSurfaceList = 
		reinterpret_cast< BspV30::marksurface* >( getLumpData( BspV30::LUMP_MARKSURFACES ) ) + fristSurface;

	MeshBuilder builder( baseType );
	for( int i = 0; i < numSurfaces ; ++i )
	{
		int idxFace = markSurfaceList[ i ];
		loadFace( idxFace , builder , obj );

	}//for( int i = 0; i < leaf->numMarkSurfaces ; ++i )
}


void HLBspFileDataV30::loadFace( int idxFace, MeshBuilder& builder , Object* obj )
{
	BspV30::face_t* face = mFaces + idxFace;
	BspV30::surfedge* surfEdgeList = 
		reinterpret_cast< BspV30::surfedge* >( getLumpData( BspV30::LUMP_SURFEDGES ) ) + face->firstEdge;
	Vector3  color;

	BspV30::texinfo_t* texInfo = mTexInfo + face->texinfo;

	BspV30::miptex_t*  mipTex = reinterpret_cast< BspV30::miptex_t* >( 
		getLumpData( BspV30::LUMP_TEXTURES ) + mTexLumpHeader->miptexOffset[ texInfo->miptex ] );

	float is = 1.0f/(float)mipTex->width;
	float it = 1.0f/(float)mipTex->height;

	int lw,lh;
	float min[2];
	float max[2];

	builder.setVertexType( baseType );

	Material* mat = mWorld->createMaterial();
	mat->addTexture( 0 , 0 , getTexture( texInfo ) );

	if( face->lightStyles[0] == 0 ) 
	{
		calcTextureBoundRect( face , texInfo , min , max );
		// compute lightmap size

		int size[2];
		for( int c = 0; c < 2; c++) 
		{
			float tmin = (float) floor(min[c]/16.0f);
			float tmax = (float) ceil(max[c]/16.0f);

			size[c] = (int) (tmax-tmin);
		}
		lw = size[0]+1;
		lh = size[1]+1;

		int lsz = lw*lh*3;
		// generate lightmaps texture
		for(int c = 0; c < 1 ; c++) 
		{
			if( face->lightStyles[c] == -1) 
				break;

			mat->addTexture( 0 , c + 1 , mTexLightMap );
			mat->getTextureLayer( c + 1 ).setUsageTexcoord( 1 );
			mat->getTextureLayer( c + 1 ).setTexOperator( CFT_LIGHT_MAP );
		}

		builder.setVertexType( lightMapType );
	}


	int idx0 , idx1 ,idx2;
	if ( surfEdgeList[0] < 0 )
	{
		BspV30::edge_t* edge = mEdges - surfEdgeList[0];
		idx0 = edge->vertex[1];
		idx1 = edge->vertex[0];
	}
	else
	{
		BspV30::edge_t* edge = mEdges + surfEdgeList[0];
		idx0 = edge->vertex[0];
		idx1 = edge->vertex[1];

	}

	builder.resetBuffer();

	for( int j = 1 ; j < face->numEdges ; ++j )
	{
		if ( surfEdgeList[j] < 0 )
		{
			bool beOppositeDir = true;
			BspV30::edge_t* edge = mEdges - surfEdgeList[j];
			idx2 = edge->vertex[0];
			assert( edge->vertex[1] == edge->vertex[0] || idx2 != idx1 );
		}
		else
		{
			BspV30::edge_t* edge = mEdges + surfEdgeList[j];
			idx2 = edge->vertex[1];
			assert( edge->vertex[1] == edge->vertex[0] || idx2 != idx1 );
		}

		Vector3& v0 = *reinterpret_cast< Vector3* >( mVertexs + idx1 );
		Vector3& v1 = *reinterpret_cast< Vector3* >( mVertexs + idx0 );
		Vector3& v2 = *reinterpret_cast< Vector3* >( mVertexs + idx2 );

#if 0
		if ( j == 1 )
		{
			Vector3 normal = ( v1 - v0 ).cross( v2 - v0 );

			float len = sqrt( normal.dot(normal) );
			float c = ( 2 + (normal.y + normal.z)/len )/ 4 ;

			color.setValue( 
				(float)rand() / RAND_MAX ,
				(float)rand() / RAND_MAX ,
				(float)rand() / RAND_MAX );
			//builder->setColor( color );
			//color.setValue( c , c , c );
		}
#endif

		BspV30::edge_t* edge;

		int idx = builder.getVertexNum();

		if( mat && face->lightStyles[0] == 0 ) 
		{

			float mid_poly_s = (min[0] + max[0])/2.0f;
			float mid_poly_t = (min[1] + max[1])/2.0f;
			float mid_tex_s = (float)lw / 2.0f;
			float mid_tex_t = (float)lh / 2.0f;

			float u , v;
			float ls , lt;

#define LIGHT_MAP_TEXCOORD( idx )\
	ls = ( mid_tex_s + (u - mid_poly_s) / 16.0f ) / float( lw );\
	lt = ( mid_tex_t + (v - mid_poly_t) / 16.0f ) / float( lh );\
	mHelper->convertTexcoord( idxFace , ls , lt );\
	builder.setTexCoord( idx , ls , lt );

#define ADD_VERTEX( POS )\
	builder.setPosition( POS );\
	calcTextureCoordinate( POS , texInfo->s , texInfo->t , u  , v );\
	builder.setTexCoord( 0 , is * u , it * v );\
	LIGHT_MAP_TEXCOORD( 1 );\
	builder.addVertex();

			ADD_VERTEX( v0 );
			ADD_VERTEX( v1 );
			ADD_VERTEX( v2 );

#undef ADD_VERTEX
#undef LIGHT_MAP_TEXCOORD

		}
		else
		{
			float u , v;

#define ADD_VERTEX( POS )\
	builder.setPosition( POS );\
	calcTextureCoordinate( POS , texInfo->s , texInfo->t , u  , v );\
	builder.setTexCoord( 0 , is * u , it * v );\
	builder.addVertex();

			ADD_VERTEX( v0 );
			ADD_VERTEX( v1 );
			ADD_VERTEX( v2 );

#undef ADD_VERTEX
		}

		builder.addTriangle( idx, idx + 1 , idx + 2 );
		idx1 = idx2;

	}//for( int j = 1 ; j < face->numEdges ; ++j )

	builder.createIndexTrangle( obj , mat );
}

Object* HLBspFileDataV30::createModel( BspV30::model_t* model )
{
	Object* obj = mScene->createObject();
	obj->setWorldPosition( Vector3( model->origin ) );
	loadModelSurface( model->firstface , model->numfaces , obj );
	return obj;
}

Texture* BspLightMapMergeHelper::createTexture( World* world )
{
	return world->_getMaterialManager()->createTexture2D( 
		"lightMap" , CF_TEX_FMT_ARGB32 , mLightMapData , 
		mRoot->rect.w , mRoot->rect.h );
}

void BspLightMapMergeHelper::convertTexcoord( int imageID , float& u , float& v )
{
	Node* node = mImageNodeMap[ imageID ];
	if ( !node )
		return;

	u = ( node->rect.x + u * node->rect.w ) / mRoot->rect.w ;
	v = ( node->rect.y + v * node->rect.h ) / mRoot->rect.h ;
}

void BspLightMapMergeHelper::bitBltRGBToARGB( uint8* src , uint8* dest , Rect const& rect , int destWidth , uint8 baseC )
{
	uint8* cdOrg = dest + 4 * ( destWidth * rect.y + rect.x );
	uint8* cs = src;

	int pitch = 4 * destWidth;

	for( int j = 0 ; j <  rect.h ; ++j )
	{
		uint8* cd = cdOrg + j * pitch;
		for( int i = 0 ; i <  rect.w ; ++i )
		{
			cd[0] = uint8( std::min( unsigned(baseC) + cs[2]  , (unsigned)0xff ) );
			cd[1] = uint8( std::min( unsigned(baseC) + cs[1]  , (unsigned)0xff ) );
			cd[2] = uint8( std::min( unsigned(baseC) + cs[0]  , (unsigned)0xff ) );
			cd[3] = 255;
			cd += 4;
			cs += 3;
		}
	}
}

void BspLightMapMergeHelper::fillImageData( int imageId , uint8* data , uint8 baseC )
{
	Node* node = mImageNodeMap[ imageId ];
	if ( !node )
		return;

	bitBltRGBToARGB( data , mLightMapData , node->rect , mRoot->rect.w , baseC );
}



bool BspTree::create( HLBspFileDataV30& fileData )
{
	int len;
	len = fileData.getLumpDataLength( BspV30::LUMP_PLANES );
	BspV30::plane_t* lumpPlane = reinterpret_cast< BspV30::plane_t* >(  fileData.getLumpData( BspV30::LUMP_PLANES ) );
	mPlanes.insert( mPlanes.begin() , lumpPlane , lumpPlane + len / sizeof( BspV30::plane_t ) );

	len = fileData.getLumpDataLength( BspV30::LUMP_NODES );
	BspV30::node_t* lumpNode = reinterpret_cast< BspV30::node_t* >(  fileData.getLumpData( BspV30::LUMP_NODES ) );
	mNodes.insert( mNodes.begin() , lumpNode , lumpNode + len / sizeof( BspV30::node_t ) );

	mNumLeaves = fileData.getLumpDataLength( BspV30::LUMP_LEAVES ) / sizeof( BspV30::leaf_t );
	mLeaves = new BspLeaf[ mNumLeaves ];

	for( int i = 0 ; i < mNumLeaves ; ++i )
	{
		fileData.loadLeaf( i , mLeaves[i] , mNumLeaves );
	}

	int numModels = fileData.getLumpDataLength( BspV30::LUMP_MODELS ) / sizeof( BspV30::model_t );
	BspV30::model_t* models = reinterpret_cast< BspV30::model_t* >(  fileData.getLumpData( BspV30::LUMP_MODELS ) );
	for (int i = 0; i < numModels ; ++i )
	{
		Model model;
		BspV30::model_t* bspModel = models + i;
		model.obj     = fileData.createModel( bspModel );
		model.visLeaf = bspModel->visleaves;
		mModels.push_back( model );
	}
	return true;
}

int BspTree::updateVis( BspLeaf* leaf )
{
	int result = 0;
	if ( leaf )
	{
		leaf->object->show( true );
		for( int i = 1 ; i < getLeafNum() ; ++i )
		{
			BspLeaf* vLeaf = getLeaf( i );
			if ( leaf->isVisible( i ) )
			{
				vLeaf->object->show( true );
				++result;
			}
			else
			{
				vLeaf->object->show( false );
			}
		}
	}
	else
	{
		for( int i = 0 ; i < getLeafNum() ; ++i )
		{
			BspLeaf* vLeaf = getLeaf( i );
			vLeaf->object->show( true );
			++result;
		}
	}
	return result;
}
