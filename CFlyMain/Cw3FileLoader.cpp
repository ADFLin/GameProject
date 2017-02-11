#include "CFlyPCH.h"
#include "Cw3FileLoader.h"

#include "CFObject.h"
#include "CFMaterial.h"
#include "CFScene.h"
#include "CFWorld.h"
#include "CFActor.h"

#include "CFVertexUtility.h"
#include <fstream>
#include <limits>

#include "FileSystem.h"
#include "StringParse.h"

namespace CFly
{
	void truncateStringSpace( char* s )
	{
		char* cur = s;

		while ( *cur != '\0' && (*cur != ' ') ){ ++cur; }

		while( *cur != '\0' )
		{
			if ( *cur != ' ' )
				*(s++) = *(cur++);
			else
			{
				*(s++) = ' ';
				while ( *cur != '\0' && (*cur != ' ') ){ ++cur; }
			}
		}
		*s = '\0';
	};

	std::istream& operator >> ( std::istream& is , Vector3& vec )
	{
		is >> vec[0] >> vec[1] >> vec[2];
		return is;
	}

	std::istream& operator >> ( std::istream& is , Quaternion& q )
	{
		is >> q[3] >> q[0] >> q[1] >> q[2];
		return is;
	}

	bool loadMotion( Skeleton& skeleton , char const* path , int numFrame , std::vector< Vector3 >& pivotVec )
	{
		std::ifstream fs( path , std::ios::binary );

		String token;
		BoneNode* bone = NULL;


		while ( fs.good() )
		{
			fs >> token;

			if ( token == "#" )
			{
				fs.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
			}
			else if ( token == "Model" )
			{
				fs.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
			}
			else if ( token == "Name" )
			{
				fs >> token;
				bone = skeleton.getBone( token.c_str() );
			}
			else if ( token == "Right" )
			{
				fs.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
			}
			else if ( token == "Motion" )
			{
				if ( !bone )
					return false;

				int num;
				fs >> num;
				Vector3 pos;
				fs >> pos;

				fs >> token;
				assert( token == "Q" );

				std::ifstream::pos_type fp = fs.tellg();
				fs >> token;
				if ( token == "P" )
				{
					for( int i = 0 ; i < numFrame ; ++i )
					{
						MotionKeyFrame& motion = bone->keyFrames[i];

						fs >> motion.rotation;
						fs >> motion.pos;

						motion.pos += pivotVec[ bone->id ];
					}
				}
				else
				{
					fs.seekg( fp );
					for( int i = 0 ; i < numFrame ; ++i )
					{
						MotionKeyFrame& motion = bone->keyFrames[i];

						fs >> motion.rotation;
						motion.pos = pivotVec[ bone->id ];
					}
				}
			}
		}
		return true;
	}



	bool loadSkeleton( Skeleton& skeleton , char const* path , BlendAnimation* animatable )
	{
		std::ifstream fs( path );

		std::vector< String >  motionFileList;
		std::vector< Vector3 > pivotVec;

		String token;
		String boneName;
		String MDType;

		String  parentName;
		String  children;

		int numBones = 0;
		int nb = 0;

		int totalPoseFrame = 0;

		while ( fs.good() )
		{
			fs >> token;

			if ( token == "#" )
			{
				fs.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
			}
			else if ( token == "Skeleton" )
			{
				fs.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
			}
			else if ( token == "Bones")
			{
				fs >> numBones;
				pivotVec.resize( numBones + 1 );
			}
			else if ( token == "Segment" )
			{
				fs >> boneName;
				fs >> token;

				KeyFrameFormat keyFormat;
				assert( token == "QP" || token == "Q" );
				if ( token == "QP" )
				{
					keyFormat = KFF_QP;
				}
				else if ( token == "Q" )
				{
					keyFormat = KFF_Q;
				}

				fs >> token;
				assert ( token[0] == '{' );

				int jointNum;

				fs >> token;
				assert( token == "Parent" );
				fs >> parentName;

				if ( parentName == "no_name" )
					parentName = CF_BASE_BONE_NAME;

				BoneNode* curBone = skeleton.getBone( boneName.c_str() );
				if ( !curBone )
					curBone = skeleton.createBone( boneName.c_str() , parentName.c_str() );

				curBone->mKeyFormat = keyFormat;

				fs >> token;
				assert( token == "Pivot" );

				fs >> pivotVec[ curBone->id ];

				fs >> token;
				if ( token == "Joint" )
				{
					fs >> jointNum;

					for ( int i = 0 ; i < jointNum ; ++ i )
					{
						fs >> token;
						Vector3 pos;
						fs >> pos;
					}
				}
				else if ( token == "MotionFile" )
				{
					fs >> token;
					motionFileList.push_back( token );
				}

				fs >> token;
				assert( token == "}" );

				++nb;
			}
			else if ( token == "Poses" )
			{
				int numPose;

				fs >> numPose;
				for ( int i = 0 ; i < numPose ; ++i )
				{
					char posName[ 64 ];
					int start;
					int end;
					int idx;

					fs >> posName >> start >> end >> idx;
					animatable->createAnimationState( posName , start , end );
					fs.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
				}
			}
			else if ( token == "Frames" )
			{
				fs >> totalPoseFrame;
				skeleton.createMotionData( totalPoseFrame );
			}
			else if ( token == "Motion" )
			{
				int numData;
				fs >> numData;
				assert( numData == numBones * 7 );
				assert( totalPoseFrame != 0 );

				for( int frame = 0; frame < totalPoseFrame ; ++frame )
				{
					for( int i = 1 ; i <= numBones ; ++i )
					{
						BoneNode* bone = skeleton.getBone( i );
						MotionKeyFrame& motion = bone->keyFrames[frame];

						fs >> motion.rotation;
						fs >> motion.pos;

						motion.pos += pivotVec[ i ];
					}
				}
			}
		}

		if ( !motionFileList.empty() )
		{
			for( int i = 0 ; i < motionFileList.size() ; ++i )
			{
				String motionPath = String(path);

				size_t pos = motionPath.find_last_of( '\\' );
				if ( pos == String::npos )
				{
					pos = motionPath.find_last_of('/');
					if ( pos == String::npos )
						continue;
				}

				motionPath.resize( pos );
				motionPath += "/" + motionFileList[i] + ".cw3";

				if ( !loadMotion( skeleton , motionPath.c_str() , totalPoseFrame , pivotVec ) )
					return false;
			}
		}
		return true;
	}

	Cw3FileImport::Cw3FileImport()
		:mObject( nullptr )
		,mScene( nullptr )
	{
		mVertex = nullptr;

		for( int i = 0 ; i < MAX_GROUP_NUM ; ++i )
			tri[i] = nullptr;

	}

	Cw3FileImport::~Cw3FileImport()
	{
		delete [] mVertex;

		for( int i = 0 ; i < MAX_GROUP_NUM ; ++i )
			delete [] tri[i];
	}

	bool Cw3FileImport::load( char const* path , Object* object , ILoadListener* listener )
	{
		mScene = object->getScene();
		setObject( object );
		return loadModel( path , BIT(1) , 0 );
	}

	bool Cw3FileImport::load( char const* path , Actor* actor , ILoadListener* listener )
	{
		TextStream file;
		if ( !file.open( path ) )
			return false;
		try
		{
			if ( !loadActor( file, actor ) )
				return false;
		}
		catch ( TokenException&  )
		{
			return false;
		}
		return true;
	}

	bool Cw3FileImport::load( char const* path , Scene* scene , ILoadListener* listener )
	{
		TextStream  file;
		if ( !file.open( path ) )
			return false;

		try
		{
			if ( !loadScene( file , scene , listener ) )
				return false;
		}
		catch ( TokenException& )
		{
			return false;
		}
		return true;
	}


	bool Cw3FileImport::loadModelData( TextStream& file , char const* strModelEnd , unsigned reserveTexBit , unsigned flag )
	{
		resetBuffer();

		char str[256];
		char* token;

		int  len;
		int  posSize = 0;
		int  BWSize  = 0;

		while( ( len = file.getLine( str , sizeof( str ) / sizeof( str[0] ) ) ) >= 0 )
		{
			if ( len == 0 )
				continue;

			StringTokenizer st;
			st.begin( str );
			token = st.nextToken( " \n" );

			if ( !token )
				continue;

			if ( token[0] == '#' )
				continue;


			if ( strcmp( token , "Name" ) == 0 )
			{
				char* name = st.nextToken();
				mObject->registerName( name );
			}
			else if ( strcmp( token , "VertexType" ) == 0 )
			{
				token = st.nextToken();
				if ( !token )
					continue;

				if ( strcmp( token , "Position" ) == 0 )
				{
					mVertexType |= CFV_XYZ;
					posSize = 3;
					mVertexSize += 3;
					token = st.nextToken();
					if ( !token )
						continue;
				}
				if ( strcmp( token , "Normal" ) == 0 )
				{
					mVertexType |= CFV_NORMAL;
					mVertexSize += 3;
					token = st.nextToken();
					if ( !token )
						continue;
				}
				if ( strcmp( token , "Color" ) == 0 )
				{
					mVertexType |= CFV_COLOR;
					mVertexSize += 3;
					token = st.nextToken();
					if ( !token )
						continue;
				}
				if ( strcmp( token ,"Texture" ) == 0 )
				{
					numTex = st.nextInt();
					mVertexType |= CFVF_TEXCOUNT( numTex );

					for( int i = 0 ; i < numTex ; ++i )
					{
						texLen[i] = st.nextInt();
						mVertexType |= CFVF_TEXSIZE( i , texLen[i] );
						mVertexSize += texLen[i];

					}
					token = st.nextToken();
					if ( !token )
						continue;
				}
				if ( strcmp( token , "SkinWeight" ) == 0 )
				{
					BWSize = 0;
					mVertexType |= CFV_BLENDINDICES;
					BWSize += 1;
					numSkinWeight = st.nextInt();
					if ( numSkinWeight > 1 )
					{
						mVertexType |= CFVF_BLENDWEIGHTSIZE( numSkinWeight - 1 );
						BWSize += numSkinWeight - 1;
					}
					mVertexSize += BWSize;
					token = st.nextToken();
					if ( !token )
						continue;
				}
			}
			else if ( strcmp( token , "RenderOption" ) == 0 )
			{
				token = st.nextToken();
				if ( strcmp( token , "SOURCE_BLEND_MODE" ) == 0 )
				{
					token = st.nextToken();
					mObject->setRenderOption( CFRO_SRC_BLEND , getBlendMode( token ) );
					mObject->setRenderOption( CFRO_Z_BUFFER_WRITE , FALSE );
					mObject->setRenderOption( CFRO_ALPHA_BLENGING , TRUE );
				}
				else if ( strcmp( token , "DESTINATION_BLEND_MODE" ) == 0 )
				{
					token = st.nextToken();
					mObject->setRenderOption( CFRO_DEST_BLEND , getBlendMode( token ) );
					mObject->setRenderOption( CFRO_Z_BUFFER_WRITE , FALSE );
					mObject->setRenderOption( CFRO_ALPHA_BLENGING , TRUE );
				}
				else
				{
					assert(0);
				}
			}
			else if ( strcmp( token , "Position" ) == 0 )
			{
				Quaternion quat;
				Vector3    pos;
				quat[3] = (float) st.nextFloat();
				quat[0] = (float) st.nextFloat();
				quat[1] = (float) st.nextFloat();
				quat[2] = (float) st.nextFloat();

				pos[0] = (float) st.nextFloat();
				pos[1] = (float) st.nextFloat();
				pos[2] = (float) st.nextFloat();

				mObject->transform( pos , quat.conjugation() , CFTO_GLOBAL );

			}
			else if ( strcmp( token , "Material" ) == 0 )
			{
				numMaterial = st.nextInt();

				int numM = 0;

				while( ( len = file.getLine( str , sizeof( str ) / sizeof( str[0] ) ) ) >= 0 )
				{
					if ( len == 0 )
						continue;

					st.begin( str );
					char* token = st.nextToken( " " );

					if ( !token )
						continue;

					if ( token[0] == '#' )
						continue;

					Material* material = mScene->getWorld()->createMaterial();

					bool beKey = false;
					BYTE r,g,b;
					char const* texName;

					if ( strcmp( token ,"Name" ) == 0 )
					{
						char* matName = st.nextToken();
						token = st.nextToken();
					}
					
					if ( strcmp( token ,"Texture" ) == 0 )
					{
						texName = st.nextToken();
						Texture* tex = material->addTexture( 0 ,0 , texName );
						assert( tex );
					}
					else if ( strcmp( token ,"TextureWithKey" ) == 0 )
					{
						r = (BYTE) st.nextInt();
						g = (BYTE) st.nextInt();
						b = (BYTE) st.nextInt();

						texName = st.nextToken();

						Texture* tex = material->addTexture( 0 , 0 , texName , CFT_TEXTURE_2D , &Color3ub( r, g , b ) );
						assert( tex );
					}
					else if ( strcmp( token , "TextureWithAlpha" ) == 0 )
					{
						texName = st.nextToken();
						char const* name = st.nextToken();

						Texture* tex = material->addTexture( 0 , 0 , texName , CFT_TEXTURE_2D , &Color3ub( r, g , b ) );
					}
					else if ( strcmp( token ,"MultiTexture" ) == 0 )
					{
						int numTex = st.nextInt();

						while ( 1 )
						{
							texName = st.nextToken( " {}" );

							int layer = st.nextInt();
							token = st.nextToken();
							assert( *token = 'N' );

							token = st.nextToken( " [" );
							if ( strcmp( token ,"Colorkey" ) == 0 )
							{
								r = (uint8) st.nextInt();
								g = (uint8) st.nextInt();
								b = (uint8) st.nextInt();
								Texture* tex = material->addTexture( 0 , layer , texName ,CFT_TEXTURE_2D ,&Color3ub(r,g,b) );
								assert( tex );
								token = st.nextToken( " [" );
							}
							else
							{
								Texture* tex = material->addTexture( 0 , layer , texName );
								assert( tex );
							}

							assert( strcmp( token , "Operator" ) == 0 );
							token = st.nextToken( " ]");

							if ( strcmp( token ,"ColorMap" ) == 0 )
							{
								material->setTexOperator( layer , CFT_COLOR_MAP );
							}
							else if ( strcmp( token ,"Lightmap" ) == 0 )
							{
								material->setTexOperator( layer , CFT_LIGHT_MAP );
							}

							--numTex;
							if ( numTex == 0 )
								break;
						}
					}
				

					while ( 1 )
					{
						float color[4];
						token = st.nextToken();

						if ( !token )
							break;

						if ( *token == '\0' || *token == '\n' )
							break;

						if ( strcmp( token , "Ambient" ) == 0 )
						{		
							color[0] = st.nextFloat();
							color[1] = st.nextFloat();
							color[2] = st.nextFloat();
							color[3] = 1.0f;
							material->setAmbient( color );
						}
						else if ( strcmp( token , "Diffuse" ) == 0 )
						{
							color[0] = st.nextFloat();
							color[1] = st.nextFloat();
							color[2] = st.nextFloat();
							color[3] = 1.0f;
							material->setDiffuse( color );
						}
						else if ( strcmp( token , "Specular" ) == 0 )
						{
							color[0] = st.nextFloat();
							color[1] = st.nextFloat();
							color[2] = st.nextFloat();
							color[3] = 1.0f;
							material->setSpeclar( color );
						}
					}

					mat[ numM ] = material;
					++numM;
					if ( numM == numMaterial )
						break;
				}

				if ( numM != numMaterial )
					return false;
			}
			else if ( strcmp( token , "UVAnim" ) == 0 )
			{
				int idxLayer = st.nextInt();
				float du = st.nextFloat();
				float dv = st.nextFloat();
				mat[0]->createUVAnimation( idxLayer , du , dv );
			}
			else if ( strcmp( token , "Vertex" ) == 0 )
			{
				int unknown = st.nextInt();

				assert( unknown == 1 );

				numVertex = st.nextInt();

				mVertex = new float[ numVertex * mVertexSize ];

				int nV = 0;
				float* v = mVertex;
				
				if ( numSkinWeight )
				{
					
					int otherSize  = mVertexSize - posSize - BWSize;

					while( ( len = file.getLine( str , sizeof( str ) / sizeof( str[0] ) ) ) >= 0 )
					{
						if ( len == 0 )
							continue;
						if ( str[0] == '#' )
							continue;

						st.begin( str );

						for( int i = 0 ; i < posSize ; ++i )
						{
							*(v++) = st.nextFloat();
						}

						float* bw = v;
						v += BWSize;
						for( int i = 0 ; i < otherSize ; ++i )
						{
							*(v++) = st.nextFloat();
						}

						uint32 blendIndices = 0;
						
						int count = 0;
						for( ; count < numSkinWeight - 1 ; ++count )
						{
							blendIndices |= BYTE( st.nextInt() + 1 ) << ( 8 * count );
							*(bw++) = st.nextFloat();
						}
						blendIndices |= BYTE( st.nextInt() + 1 ) << ( 8 * count );
						float weight = st.nextFloat();

						uint32* fillIndices = reinterpret_cast< uint32* >( bw );
						*fillIndices = blendIndices;

						++nV;
						if ( nV == numVertex )
							break;
					}
				}
				else
				{
					while( ( len = file.getLine( str , sizeof( str ) / sizeof( str[0] ) ) ) >= 0 )
					{
						if ( len == 0 )
							continue;
						if ( str[0] == '#' )
							continue;

						st.begin( str );
						for( int count = 0 ; count < mVertexSize ; ++count )
						{
							*(v++) = st.nextFloat();
						}
						++nV;
						if ( nV == numVertex )
							break;
					}
				}

				if ( nV != numVertex )
					return false;
			}
			else if ( strcmp( token , "Polygon" ) == 0 )
			{
				numTri = st.nextInt();

				for( int i = 0 ; i < numMaterial ; ++i )
				{
					nextTri[i] = tri[i] = new int[ numTri * 3 ];
				}

				int nT = 0;
				while( ( len = file.getLine( str , sizeof( str ) / sizeof( str[0] ) ) ) >= 0 )
				{
					if ( len == 0 )
						continue;

					st.begin( str );

					int nS    = st.nextInt();
					int group = st.nextInt();

					*(nextTri[group]++) = st.nextInt();
					*(nextTri[group]++) = st.nextInt();
					*(nextTri[group]++) = st.nextInt();

					//if ( sscanf( str , "%d %d %d %d %d" , &nS , &group , t , t + 1 , t + 2  ) != 5 )
					//{
					//	return false;
					//}
					++nT;
					if( nT == numTri )
						break;
				}
			}
			else if ( strcmp( token , "Motion" ) == 0 )
			{
				int totalFrame = st.nextInt();
				
				Vector3 pos;
				pos[0] = atof( st.nextToken() );
				pos[1] = atof( st.nextToken() );
				pos[2] = atof( st.nextToken() );

				mObject->translate( pos , CFTO_REPLACE );

				char* qStr = st.nextToken();
				assert( strcmp( qStr ,"Q" ) == 0 );
				char* pStr = st.nextToken();
				assert( strcmp( pStr ,"P" ) == 0 );

				int nFrame = 0;
				while( ( len = file.getLine( str , sizeof( str ) / sizeof( str[0] ) ) ) >= 0 )
				{
					if ( len == 0 )
						continue;

					Quaternion q;
					Vector3    pos;

					if ( sscanf_s( str , "%f %f %f %f %f %f %f" , &q[3], &q[0], &q[1], &q[2] , &pos[0] , &pos[1], &pos[2] ) != 7 )
					{
						return false;
					}
					if ( nFrame == 1 )
					{
						mObject->rotate( q , CFTO_LOCAL );
					}
					++nFrame;
					if( nFrame == totalFrame )
						break;
				}
			}
			else if ( strModelEnd && strcmp( token , strModelEnd ) == 0 )
			{
				break;
			}
		}

		if ( reserveTexBit )
			VertexUtility::reserveTexCoord( reserveTexBit , mVertex , numVertex , mVertexType , mVertexSize );

		if ( ! createMesh( flag ) )
			return false;

		return true;
	}

	bool Cw3FileImport::loadModel( char const* path , unsigned reserveTexBit , unsigned flag )
	{
		TextStream  file;
		if ( !file.open( path ) )
			return false;
		return loadModel( file , reserveTexBit , flag );
	}

	bool Cw3FileImport::loadModel( TextStream& file , unsigned reserveTexBit , unsigned flag )
	{
		char str[256];
		char* token;
		int  len;

		while( ( len = file.getLine( str , sizeof( str ) / sizeof( str[0] ) ) ) >= 0 )
		{
			if ( len == 0 )
				continue;

			StringTokenizer st;
			st.begin( str );
			token = st.nextToken( " \n" );

			if ( !token )
				continue;

			if ( token[0] == '#' )
				continue;

			if ( strcmp( token , "BeginModel" ) == 0 || strcmp( token , "Model") )
			{
				if ( !loadModelData( file , "EndModel" , reserveTexBit , flag ) )
					return false;
			}
		}
		return true;
	}

	bool Cw3FileImport::loadScene( TextStream& file , Scene* scene , ILoadListener* listener )
	{
		mScene = scene;

		char str[256];
		char* token;
		int len;

		float version;
		int   numObj = 0;
		int   nb = 0;

		StringTokenizer st;

		while( ( len = file.getLine( str , sizeof( str ) / sizeof( str[0] ) ) ) >= 0 )
		{
			if ( len == 0 )
				continue;

			st.begin( str );
			token = st.nextToken( " \n" );

			if ( !token )
				continue;

			if ( token[0] == '#' )
				continue;

			if ( strcmp( token , "Scene" ) == 0 )
			{
				token = st.nextToken();
				assert( *token == 'v' );
				version = atof( st.nextToken() );
			}
			else if ( strcmp( token , "Object" ) == 0 )
			{
				numObj = st.nextInt();
			}
			else if ( strcmp( token , "BeginObject" ) == 0 )
			{
				int idx = st.nextInt();
				assert( idx == nb + 1 );

				Object* obj = mScene->createObject( nullptr );

				setObject( obj );

				token = st.nextToken();
				if ( strcmp( token , "Model" ) == 0  )
				{
					if  ( !loadModelData( file , "EndObject" , BIT(1) , 0 ) )
						return false;
				}

				if ( listener )
					listener->onLoadObject( obj );

				//if ( nb < 8 && nb != 1 )
				//	m_scene->destroyObject( obj );
				++nb;
			}
			else if ( strcmp( token , "BeginRelationship" ) == 0 )
			{
				return true;
			}
		}

		return true;
	}

	void Cw3FileImport::resetBuffer()
	{
		mVertexType = 0;
		mVertexSize = 0;

		numGroup = 0;
		numMaterial = 0;
		numVertex = 0;
		numTri = 0;
		numTex = 0;
		numSkinWeight = 0;

		if ( mVertex )
		{
			delete [] mVertex;
			mVertex = nullptr;
		}
		for( int i = 0 ; i < MAX_GROUP_NUM ; ++i )
		{
			delete [] tri[i];
			tri[i] = nullptr;
		}
	
		for( int i= 0 ; i < 8 ; ++i )
		{
			mat[i] = nullptr;
		}
	}

	DWORD Cw3FileImport::getBlendMode( char const* token )
	{
		if ( strcmp( token , "BLEND_ONE" ) == 0 )
			return CF_BLEND_ONE;
		if ( strcmp( token , "BLEND_ZERO" ) == 0 )
			return CF_BLEND_ZERO;
		if ( strcmp( token , "BLEND_SRC_COLOR" ) == 0 )
			return CF_BLEND_SRC_COLOR;

		assert( 0 );
		return 0;
	}

	bool Cw3FileImport::loadActor( TextStream& file , Actor* actor )
	{
		mScene = actor->getScene();

		char str[256];
		char* token;
		int len;

		float version;
		StringTokenizer st;
		World* world = mScene->getWorld();

		while( ( len = file.getLine( str , sizeof( str ) / sizeof( str[0] ) ) ) >= 0 )
		{
			if ( len == 0 )
				continue;

			st.begin( str );
			token = st.nextToken( " \n" );

			if ( !token )
				continue;

			if ( token[0] == '#' )
				continue;

			if ( strcmp( token , "Character" ) == 0 )
			{
				version = (float)st.nextFloat();
			}
			else if ( strcmp( token , "Skeleton" ) == 0 )
			{
				char skPath[256];
				if ( !world->getPath( skPath , st.nextToken() , ".cwk") )
					return false;

				if ( !loadSkeleton( *actor->getSkeleton() , skPath , actor ) )
					return false;
			}
			else if ( strcmp( token , "Skin" ) == 0 )
			{
				char skinPath[256];
				if ( !world->getPath( skinPath , st.nextToken() , ".cw3" ) )
					return false;

				Object* skin = actor->createSkin();
				setObject( skin );

				bool result = loadModel( skinPath , BIT(1) , CFVD_SOFTWARE_PROCESSING );

				if ( !result )
					return false;
			}
			else if ( strcmp( token , "Attach" ) == 0 )
			{
				char attchPath[256];
				if ( !world->getPath( attchPath , st.nextToken() , ".cw3" ) )
					return false;

				Object* obj = mScene->createObject( nullptr );
				setObject( obj );

				token = st.nextToken();
				TransOp op;
				if ( strcmp( token , "LOCAL" ) == 0 )
				{
					op = CFTO_LOCAL;
				}
				else
				{
					assert(0);
				}

				char* boneName = st.nextToken();

				try
				{
					if ( !loadModel( attchPath , BIT(1) , 0 ) )
						return false;
				}
				catch ( TokenException&  )
				{
					return false;
				}

				if ( actor->applyAttachment( obj , boneName , op ) == CF_FAIL_ID )
					return false;
			}
		}

		actor->updateSkeleton( true );
		return true;

	}

	bool Cw3FileImport::createMesh( unsigned flag )
	{
		MeshInfo info;
		info.primitiveType = CFPT_TRIANGLELIST;
		info.vertexType    = mVertexType;
		info.pVertex       = mVertex;
		info.numVertices   = numVertex;
		info.pIndex        = tri[0];
		info.numIndices    = nextTri[0] - tri[0];
		info.isIntIndexType = true;
		info.flag          = flag;
		
		int id = mObject->createMesh( mat[0] , info );
		if ( id == CF_FAIL_ID )
			return false;

		if ( numMaterial > 1 )
		{
			MeshBase* mesh = mObject->getElement( id )->getMesh();
			for( int i = 1 ; i < numMaterial ; ++i )
			{
				id = mObject->createMesh( mat[i] , 
					CFPT_TRIANGLELIST , mesh , tri[i] , nextTri[i] - tri[i] );
				if ( id == CF_FAIL_ID )
					return false;
			}
		}
		return true;
	}

	bool Cw3FileLinker::canLoad( EntityType type , char const* subFileName )
	{
		switch( type )
		{
		case ET_OBJECT:
			if ( strcmp( subFileName , "cw3" ) == 0 )
				return true;
		case ET_ACTOR:
			if ( strcmp( subFileName , "cwc" ) == 0 )
				return true;
		case ET_SCENE:
			if ( strcmp( subFileName , "cwn" ) == 0 )
				return true;
		}
		return false;
	}


}//namespace CFly