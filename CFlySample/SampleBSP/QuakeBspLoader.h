#ifndef QuakeBspLoader_h__
#define QuakeBspLoader_h__

#include "BspV30.h"
#include "Core/IntegerType.h"
#include "Misc/ImageMergeHelper.h"

#include "CFBase.h"
#include "CFMaterial.h"
#include "CFVertexDecl.h"
#include "Holder.h"


namespace CFly
{
	class MeshBuilder;
}

using namespace CFly;

struct RFace
{
	int       offsetIndex;
	int       numIndex;
	Material* mat;
};

struct BspVertex
{
	static VertexType const Type = CFVT_XYZ | CFVF_TEX2( 2 , 2 );
	Vector3 pos;
	float   tex[2];
	float   texLight[2];
};


struct BspLeaf
{
	Object* object;

	std::vector< RFace > faces;
	std::vector< uint8 >  pvs;

	int*    indexOffset;
	int     numIndex;

	bool isVisible( int idxLeaf )
	{
		--idxLeaf;									// Fix off by one error
		int nIndex = idxLeaf >> 3;					// nLeafTo / 8
		int nBit = idxLeaf & 0x07;					// nLeafTo % 8
		return ( pvs[nIndex] & uint8( 1 << nBit ) ) != 0;
	}
};



class BspLightMapMergeHelper : public ImageMergeHelper
{
public:
	BspLightMapMergeHelper( int w , int h )
		:ImageMergeHelper( w , h )
		,mLightMapData( new uint8[ w * h * 4 ] )
	{
		std::fill_n( reinterpret_cast< uint32* >( mLightMapData.get() ) , w * h , 0xffffffff );
		//memset( mLightMapData , 0 , size );
	}

	~BspLightMapMergeHelper()
	{

	}

	static void bitBltRGBToARGB( uint8* src , uint8* dest , Rect const& rect , int destWidth , uint8 baseC );

	void fillImageData( int imageId , uint8* data , uint8 baseC );

	Texture* createTexture( World* world );

	void convertTexcoord( int imageID , float& u , float& v );

	TArrayHolder< uint8 > mLightMapData;

};

namespace WAD3
{

	enum
	{
		TYP_NONE		= 0,
		TYP_LABEL		= 1,
		TYP_LUMPY		= 64,
		TYP_PALETTE		= 64,
		TYP_COLORMAP	= 65,
		TYP_QPIC		= 66,
		TYP_MIPTEX		= 67,
		TYP_RAW			= 68,
		TYP_COLORMAP2	= 69,
		TYP_FONT		= 70,
		TYP_SOUND		= 71,
		TYP_QTEX		= 72,
	};


	struct Header
	{
		uint32 	identification;		// should be WAD2 or 2DAW
		int	    numLumps;
		int		infoTableOffset;
	};

#define TEXTURE_NAME_LENGTH	16

	struct LumpInfo
	{
		uint32		fileOffset;
		uint32		disksize;
		uint32		size;			// uncompressed
		char		type;
		char		compression;
		char		pad1, pad2;
		char		name[TEXTURE_NAME_LENGTH];	
	};

	struct MipTexInfo
	{
		char		name[TEXTURE_NAME_LENGTH];
		uint32      width;
		uint32      height;
		uint32      mipMapOffset[4];
	};

	struct ColorKey
	{
		unsigned char r , g , b;
	};

}//namespace WAD3

class Wad3FileData
{
public:
	Wad3FileData();
	~Wad3FileData();

	bool load( char const* path );

	Texture* createTexture( World* world , char const* name );

	static bool fillImageDataARGB( unsigned char* data  , unsigned length  , unsigned char* idxData, WAD3::ColorKey* pal );

	static Texture* createMipTexture( World* world , char* data , WAD3::MipTexInfo* texInfo , int const minMapLevel = 0 );


	Texture* createTexture( World* world , WAD3::LumpInfo* lumpInfo );

	WAD3::Header* getHeader(){ return reinterpret_cast< WAD3::Header*>( &mData[0] ); }

	WAD3::LumpInfo* mLumps;
	std::vector< char > mData;
};


class HLBspFileDataV30
{
public:
	HLBspFileDataV30()
	{
		mHelper = nullptr;

		mScene = nullptr;
		mWorld = nullptr;
	}

	~HLBspFileDataV30()
	{
		cleanup();
	}

	void cleanup();


	typedef std::vector< Wad3FileData* > Wad3FileList;

	Wad3FileList    mWadLoader;
	std::vector< Texture::RefCountPtr > mTextureMap;
	Texture::RefCountPtr mTexEmpty;
	Texture::RefCountPtr mTexLightMap;
	BspLightMapMergeHelper* mHelper;
	Scene*  mScene;
	World*  mWorld;

	void  setScene( Scene* scene );

	void loadEntity()
	{
		//std::stringstream ss( std::string( mEntity , getHeader()->directory[ BspV30::LUMP_ENTITIES ].length) );

		//std::string token;
		//ss >> token;
		//ss >> token;
		//if ( token == "wad" )
		//{
		//	ss >> token;

		//}
		//ss >> token;
	}

	bool    load( char const* path );
	Object* createModel( BspV30::model_t* model );

	static void converRGBToARGB( uint8* src , uint8* dest , int num , unsigned baseC )
	{
		for( int i = 0 ; i< num ; ++i )
		{
			dest[0] = BYTE( std::min( unsigned( baseC + src[2] ) , ( unsigned)0xff ) );
			dest[1] = BYTE( std::min( unsigned( baseC + src[1] ) , ( unsigned)0xff ) );
			dest[2] = BYTE( std::min( unsigned( baseC + src[0] ) , ( unsigned)0xff ) );
			dest[3] = 0;

			dest += 4;
			src  += 3;
		}
	}

	void buildLightMap();

	BspV30::header* getHeader()
	{
		return reinterpret_cast<BspV30::header*>(&mData[0]);
	}

	char* getLumpData( int lumpID )
	{
		return &mData[0] + getHeader()->directory[ lumpID ].offset;
	}
	unsigned getLumpDataLength( int lumpID )
	{
		return getHeader()->directory[ lumpID ].length;
	}

	static float VectorDot( Vector3 const& pos , float* v4  )
	{
		return pos.x * v4[0] + pos.y * v4[1] + pos.z * v4[2] + v4[3];
	}

	static void calcTextureCoordinate( Vector3 const& pos , float* s , float* t , float& u , float& v )
	{
		u = VectorDot( pos , s );
		v = VectorDot( pos , t );
	}

	Texture* createTextureFromWAD( char const* name );

	void loadPVS( BspV30::leaf_t* leaf , BspLeaf& bspLeaf , int numLeaf );

	void calcTextureBoundRect( BspV30::face_t* face , BspV30::texinfo_t* texInfo , float min[2] , float max[2] );

	Texture* getTexture( BspV30::texinfo_t* texInfo );
	void     loadLeaf( unsigned idxLeaf , BspLeaf& bspLeaf , int numLeaf );
	void     loadSurface( uint16 fristSurface , uint16 numSurfaces , Object* obj );
	void     loadModelSurface( uint16 fristSurface , uint16 numSurfaces , Object* obj );

	void     loadFace( int idxFace, MeshBuilder& builder , Object* obj );

	std::map< uint32 , Material* > mMatMap;
	std::vector< char > mData;

	BspV30::texture_lump_header* mTexLumpHeader;
	BspV30::header*    mHeader;
	BspV30::face_t*    mFaces;
	BspV30::plane_t*   mPlanes;
	BspV30::edge_t*    mEdges;
	BspV30::vertex_t*  mVertexs;
	BspV30::texinfo_t* mTexInfo;
	BspV30::node_t*    mNodes;
	BspV30::leaf_t*    mLeaves;
	BspV30::miptex_t*  mMipTex;
	BspV30::model_t*   mModels;
	char*              mLightMaps;

	char const*        mEntity;
};


class BspTree
{
public:
	BspTree()
	{
		mLeaves = nullptr;
		mNumNode  = 0;
	}

	~BspTree()
	{
		delete [] mLeaves;
	}

	bool create( HLBspFileDataV30& fileData );



	static float VectorDot( Vector3 const& a , float b[] )
	{
		return a.x * b[0] + a.y * b[1] + a.z * b[2];
	}

	static bool isInBoundBox( BspV30::node_t& node , Vector3 const& pos )
	{
		return node.min[0] <= pos.x && pos.x <= node.max[0] &&
			   node.min[1] <= pos.y && pos.y <= node.max[1] &&
			   node.min[2] <= pos.z && pos.z <= node.max[2] ;
	}

	int      getLeafNum() const { return mNumLeaves; }
	BspLeaf* getLeaf( int idx ) const { return &mLeaves[idx]; }
	BspLeaf* findLeaf( Vector3 const& pos )
	{
		int idxNode = 0;
		while( idxNode >= 0 )
		{
			BspV30::node_t& node = mNodes[ idxNode ];

			if ( !isInBoundBox( node , pos ) )
				return nullptr;

			BspV30::plane_t& plane = mPlanes[ node.plane_idx ];

			if ( VectorDot( pos , plane.normal ) > plane.dist )
				idxNode = node.children[0];
			else
				idxNode = node.children[1];
		}

		idxNode = -( idxNode + 1 );
		return mLeaves + idxNode;
	}

	std::vector< BspV30::plane_t > mPlanes;
	std::vector< BspV30::node_t  > mNodes;

	int updateVis( BspLeaf* leaf );

	struct Model
	{
		Object* obj;
		uint32  visLeaf;
	};


	std::vector< int >    mMeshIndexs;
	std::vector< Model >  mModels;
	//VertexBuffer* mVertex;
	int      mNumNode;
	BspLeaf* mLeaves;
	int      mNumLeaves;
};

#endif // QuakeBspLoader_h__