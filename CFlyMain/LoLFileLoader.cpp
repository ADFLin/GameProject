#include "CFlyPCH.h"
#include "LoLFileLoader.h"

#include "CFMaterial.h"
#include "CFWorld.h"
#include "CFScene.h"
#include "CFActor.h"
#include "CFObject.h"
#include "CFMatrix3.h"
#include "CFPluginManager.h"

#include "FileSystem.h"

#include <cctype>

namespace CFly
{
	void toLower( char* str )
	{
		while( *str != '\0' )
		{
			*str = ::tolower( (int)*str );
			++str;
		}
	}
	namespace LoL
	{

		bool SKNFile::load( BinaryStream& stream )
		{
			char  magic[4];
			stream.read( magic );
			version    = stream.read< int16 >();
			numObjects = stream.read< int16 >();

			if ( version == 1 || version == 2 )
			{
				int numMat = stream.read< int32 >();
				mats.resize( numMat );
				stream.readArray( &mats[0] , numMat );

				numIndices = stream.read< int32 >();
				numVertices = stream.read< int32 >();

				indices.resize( numIndices );
				stream.readArray( &indices[0] , numIndices );

				vertices.resize( numVertices );
				stream.readArray( &vertices[0] , numVertices );

				if ( version == 2 )
				{
					stream.read( endTab );
				}
			}
			else
			{
				return false;
			}
			return true;
		}


		bool SKLFile::load( BinaryStream& stream )
		{
			stream.read( id );
			version = stream.read< uint32 >();

			if ( version == 1 || version == 2 )
			{
				designId = stream.read< uint32 >();
				uint32 numBones = stream.read< uint32 >();
				bones.resize( numBones );
				for( int i = 0 ; i < numBones ; ++i )
				{
					Bone& bone = bones[i];
					stream.readArray( bone.name );
					toLower( bone.name );
					bone.parent = stream.read< int32 >();
					bone.scale  = stream.read< float >();
					float m[12];
					stream.readArray( m );

					Matrix3 mat;
					mat.setValue(
						m[0] , m[4] , m[8]  ,
						m[1] , m[5] , m[9]  ,
						m[2] , m[6] , m[10] );
					bone.rotation.setMatrix( mat );
					bone.pos.setValue( m[3] , m[7] , m[11] );
				}
				// Version two contains bone IDs.
				if ( version == 2 )
				{
					uint32 numBoneIndexConv = stream.read< uint32 >();
					boneIndexMap.resize( numBoneIndexConv );
					stream.readArray( &boneIndexMap[0] , numBoneIndexConv );
				}

			}
			else if ( version == 0 )
			{
				// Header
				int16 zero = stream.read< int16 >(); // ?
				uint32 numBones = stream.read< int16 >();
				uint32 numBoneIndexConv = stream.read< uint32 >();

				int16 offsetToVertexData = stream.read< int16 >(); // Should be 64.

				int16 unknown = stream.read< int16 >(); // ?

				int32 offset1 = stream.read< int32 >();
				int32 offsetToAnimationIndices = stream.read< int32 >();
				int32 offset2 = stream.read< int32 >();
				int32 offset3 = stream.read< int32 >();
				int32 offsetToStrings = stream.read< int32 >();

				// Not sure what this data represents.
				// I think it's padding incase more header data is required later.
				//stream.BaseStream.Position += 20;

				stream.seek( offsetToVertexData , std::ios::beg );

				bones.resize( numBones );
				for (int i = 0; i < numBones; ++i )
				{
					zero = stream.read< int16 >(); // ?
					int16 boneId = stream.read< int16 >();

					Bone& bone = bones[boneId];

					bone.scale = 0.1f;
					bone.parent = stream.read< int16 >();
					
					unknown = stream.read< int16 >(); // ?

					int namehash = stream.read< int32 >();

					float twoPointOne = stream.read< float >();

					stream.read( bone.pos );

					float one = stream.read< float >(); // ? Maybe scales for X, Y, and Z
					one = stream.read< float >();
					one = stream.read< float >();

					stream.read( bone.rotation );

					float ctx = stream.read< float >(); // ctx
					float cty = stream.read< float >(); // cty
					float ctz = stream.read< float >(); // ctz

					// The rest of the bone data is unknown. Maybe padding?
					stream.seek( 32 , std::ios::cur );
				}

				stream.seek( offset1 , std::ios::beg );
				for (int i = 0; i < numBones; ++i) // Inds for version 4 animation.
				{
					// 8 bytes
					uint32 sklID = stream.read< uint32 >();
					uint32 anmID = stream.read< uint32 >();

					boneIdMap[anmID] = sklID;
				}

				stream.seek( offsetToAnimationIndices , std::ios::beg );

				boneIndexMap.resize( numBoneIndexConv );
				for (int i = 0; i < numBoneIndexConv ; ++i ) // Inds for animation
				{
					boneIndexMap[i] = stream.read< uint16 >();
				}

				stream.seek( offsetToStrings , std::ios::beg );
				for (int i = 0; i < numBones; ++i)
				{
					Bone& bone = bones[i];

					int const total = 32 / 4;
					for( int n = 0 ; n < total ; ++n )
					{
						char* ptr = bone.name + 4 * n;
						stream.readArray( ptr , 4 );
						if( ptr[0] == '\0' || ptr[1] == '\0' || ptr[2] == '\0' || ptr[3] == '\0' )
							break;
					}
					toLower( bone.name );
				}

				for( int i = 0 ; i < bones.size() ; ++i )
				{
					Bone& bone = bones[i];
					if ( bone.parent == -1 )
						continue;

					Bone& parent = bones[ bone.parent ];
					bone.pos      = parent.pos + parent.rotation.rotate( bone.pos );
					bone.rotation = parent.rotation * bone.rotation;
				}
			}
			else
			{
				return false;
			}
			return true;
		}


		static void lookUpVector( Vector3& out , std::vector< float >& data , uint16 id )
		{
			uint32 idx = id * 3;
			out[0] = data[ idx++ ];
			out[1] = data[ idx++ ];
			out[2] = data[ idx++ ];
		}
		static void lookUpQuaternion( Quaternion& out , std::vector< float >& data , uint16 id )
		{
			uint32 idx = id * 4;
			out[0] = data[ idx++ ];
			out[1] = data[ idx++ ];
			out[2] = data[ idx++ ];
			out[4] = data[ idx++ ];
		}

		bool InibinFile::load( BinaryStream& stream )
		{
			KeyType keys[ MAX_KEYS_NUM ];

			int fileLen = (int)stream.getLength();
			version = stream.read< uint8 >();

			int oldLen  = (int)stream.read<int16>();

			int oldStyleOffset = fileLen - oldLen;
			uint16 format = stream.read<uint16>();

			if ( format & 0x0001 )
			{
				//uint32
				int num = readSegmentKeys( stream , keys );
				for( int i = 0 ; i < num ; ++i )
				{
					int32 value = stream.read< int32 >();
					addProperty( keys[i] , value );
				}
			}

			if ( format & 0x0002 )
			{
				int num = readSegmentKeys( stream , keys );
				for( int i = 0 ; i < num ; ++i )
				{
					float value = stream.read< float >();
					addProperty( keys[i] , value );
				}
			}

			// U8 values
			if (format & 0x0004 )
			{
				int num = readSegmentKeys( stream , keys );
				for( int i = 0 ; i < num ; ++i )
				{
					float value = stream.read< uint8 >() * 0.1f;
					addProperty( keys[i] , value );
				}
			}

			// U16 values
			if (format & 0x0008)
			{

				int num = readSegmentKeys( stream , keys );
				for( int i = 0 ; i < num ; ++i )
				{
					int32 value = stream.read< uint16 >();
					addProperty( keys[i] , value );
				}
			}

			// U8 values
			if (format & 0x0010)
			{
				int num = readSegmentKeys( stream , keys );
				for( int i = 0 ; i < num ; ++i )
				{
					int32 value = 0xff& stream.read< uint8 >();
					addProperty( keys[i] , value );
				}
			}

			// Boolean flags - single bit, ignoring
			if (format & 0x0020)
			{
				int num = readSegmentKeys( stream , keys );
				if ( num )
				{
					int index = 0;
					int size = 1 + ( num - 1 ) / 8;
					for( int i = 0 ; i < size ; ++i )
					{
						uint8 bits = stream.read< uint8 >();
						for( int n = 0 ; n < 8 ; ++ n )
						{
							int32 value = bits & 0x1;
							addProperty( keys[index] , value );
							bits >>= 1;
							++index;
							if ( index == num )
								break;
						}
					}
				}
			}

			//3-byte ??
			if ( format & 0x0040 )
			{
				int num = readSegmentKeys( stream , keys );
				for( int i = 0 ; i < num ; ++i )
				{
					uint8 c[3];
					stream.readArray( c );
					addProperty( keys[i] , int32( ( c[2] << 8 ) | ( c[1] << 4 ) | c[0] ) );
				}
			}


			if ( format & 0x0080 )
			{
				int i = 1;

			}

			// 4-byte color values or something?
			if (format & 0x0400)
			{
				int num = readSegmentKeys( stream , keys );
				for( int i = 0 ; i < num ; ++i )
				{
					int32 value = (int32)stream.read< uint32 >();
					addProperty( keys[i] , value );
				}
			}

			// Newer section.
			// I don't know what exactly these values represent.
			// I think it's related to champions with the new rage mechanic.
			// I'm just using it to increment the stream.
			// So, when I get to the part to read in strings, the pointer is at the
			// correct location.
			if (format & 0x0080)
			{
				int num = readSegmentKeys( stream , keys );
				for( int i = 0 ; i < num ; ++i )
				{
					float color[3];
					color[0] = stream.read< float >();
					color[1] = stream.read< float >();
					color[2] = stream.read< float >();

					addColorProp( keys[i] , color );
				}
			}

			// Old-style offsets to strings
			if ( format & 0x1000 )
			{
				int lastOffset = -1;
				int num = readSegmentKeys( stream , keys );

				//
				// New method to read the newer .inibins.
				// Why determine the offset by reading in data from the file header
				// when we can just compute it here?  This seems to fix the problem
				// with newer .inibins.  I'm not sure what the value in the header
				// is used for though.
				//
				if ( num )
				{
					oldStyleOffset = (int)stream.getPosition() + num * 2;

					for ( int i = 0 ; i < num ; ++i )
					{
						uint32 offset = stream.read< uint16 >() + oldStyleOffset;

						int pos = stream.getPosition();
						stream.seek( offset , std::ios::beg );

						char str[512];
						int size = stream.readCString( str , 512 );
						stream.seek( pos );

						addProperty( keys[i] , str );
						lastOffset = offset;
					}
				}
			}

			return true;
		}

		int InibinFile::readSegmentKeys( BinaryStream& stream , KeyType keys[] )
		{
			int16 count = stream.read< int16 >();
			assert( MAX_KEYS_NUM > count );
			// Sometimes this happens.
			if (count < 0)
				return 0;
			stream.readArray( keys , count );
			return count;
		}


		template< class T , class C >
		void InibinFile::addPropImpl( KeyType key , T value , PropType type , C& c )
		{
			Prop prop;
			prop.type  = type;
			prop.index = c.size();
			typedef std::pair< PropMap::iterator , bool > InsertResult;
			InsertResult result = mMap.insert( std::make_pair( key , prop ) );
			if ( result.second )
			{
				c.push_back( value );
			}
			else
			{
				c[ result.first->second.index ] = value;
			}
		}
		void InibinFile::addColorProp( KeyType key , float* color )
		{

			Prop prop;
			prop.type  = PT_COLOR;
			prop.index = mFloatProps.size();
			typedef std::pair< PropMap::iterator , bool > InsertResult;
			InsertResult result = mMap.insert( std::make_pair( key , prop ) );
			if ( result.second )
			{
				mFloatProps.push_back( color[0] );
				mFloatProps.push_back( color[1] );
				mFloatProps.push_back( color[2] );
			}
			else
			{
				int idx = result.first->second.index;
				mFloatProps[ idx + 0 ] = color[0];
				mFloatProps[ idx + 1 ] = color[1];
				mFloatProps[ idx + 2 ] = color[2];
			}
		}

		void InibinFile::addProperty( KeyType key , int32 value )
		{
			addPropImpl( key , value , PT_INT , mIntProps );
		}

		void InibinFile::addProperty( KeyType key , float value )
		{
			addPropImpl( key , value , PT_FLOAT , mFloatProps );
		}

		void InibinFile::addProperty( KeyType key , char* str )
		{
			addPropImpl( key , str , PT_STRING , mStringProps );
		}

		char const* InibinFile::getStringProp( KeyType key )
		{
			PropMap::iterator iter = mMap.find( key );
			if ( iter == mMap.end() )
				return "";
			if ( iter->second.type != PT_STRING )
				return "";
			return mStringProps[ iter->second.index ].c_str();
		}

		bool ANMFile::load( BinaryStream& stream )
		{
			stream.readArray( id );
			version = stream.read< uint32 >();
			if ( version == 0 || version == 1 || version == 2 || version == 3 )
			{
				magic = stream.read< uint32 >();
				numBones = stream.read< uint32 >();
				numFrames = stream.read< uint32 >();
				playbackFPS = stream.read< uint32 >();

				bones.resize( numBones );
				frames.resize( numBones * numFrames );
				// Read in all the bones
				for (uint32 i = 0; i < numBones; ++i )
				{
					Bone& bone = bones[ i ];
					stream.readArray( bone.name );
					toLower( bone.name );
					bone.id = -1; 
					uint32 known = stream.read< uint32 >();
					bone.idxFrame = i * numFrames;
					stream.readArray( &frames[ bone.idxFrame ] , numFrames );
				}
			}
			else if ( version == 4 )
			{
				return false;
				//
				// Based on the reverse engineering work of Hossein Ahmadi.
				//
				// In this version, position vectors and orientation quaternions are
				// stored separately in sorted, keyed blocks.  The assumption is Riot
				// is removing duplicate vectors and quaternions by using an indexing scheme
				// to look up values.
				//
				// So, after the header, there are three data sections: a vector section, a quaternion
				// section, and a look up section.  The number of vectors and quaternions
				// may not match the expected value based on the number of frames and bones.  However, 
				// the number of look ups should match this value and can be used to create the animation.

				//
				// Header information specific to version 4.
				//

				magic = stream.read< uint32 >();

				// Not sure what any of these mean.
				float unknown = stream.read< float >();
				unknown = stream.read< float >();
				unknown = stream.read< float >();


				numBones = stream.read< uint32 >();
				numFrames = stream.read< uint32 >();

				// Time per frame is stored in this file type.  Need to invert it into FPS.
				playbackFPS = (uint32) Math::Round( 1.0f / stream.read< float >() );

				// These are offsets to specific data sections in the file.
				uint32 unknownOffset = stream.read< uint32 >();
				unknownOffset = stream.read< uint32 >();
				unknownOffset = stream.read< uint32 >();

				uint32 positionOffset = stream.read< uint32 >();
				uint32 orientationOffset = stream.read< uint32 >();
				uint32 indexOffset = stream.read< uint32 >();

				// These last three values are confusing.
				// They aren't a vector and they throw off the offset values
				// by 12 bytes. Just ignore them and keep reading.
				unknownOffset = stream.read< uint32 >();
				unknownOffset = stream.read< uint32 >();
				unknownOffset = stream.read< uint32 >();

				bones.resize( numBones );
				frames.resize( numBones * numFrames );

				std::vector< float > poitions;
				uint32 numPosition = (orientationOffset - positionOffset) / sizeof(float);
				poitions.resize( numPosition );
				stream.readArray( &poitions[0] , numPosition );

				std::vector< float > quaternions;
				uint32 numQuaternion = ( indexOffset - orientationOffset) / sizeof(float);
				quaternions.resize( numQuaternion );
				stream.readArray( &quaternions[0] , numPosition );

				//
				// Offset section.
				//
				// Note: Unlike versions 0-3, data in this version is
				// Frame 1:
				//      Bone 1:
				//      Bone 2:
				// ...
				// Frame 2:
				//      Bone 1:
				// ...
				//

				std::vector< Bone* > boneMap;
				boneMap.resize( numBones , nullptr );

				for (uint32 i = 0; i < numBones; ++i )
				{
					Bone& bone = bones[ i ];
					// The first frame is a special case since we are allocating bones
					// as we read them in.
					//
					// Read in the offset data.
					uint32 boneID = stream.read< uint32 >();
					uint16 positionID = stream.read< uint16 >();
					uint16 unknownIndex = stream.read< uint16 >(); // Unknown.
					uint16 orientationID = stream.read< uint16 >();

					bone.id = boneID;
					bone.idxFrame = i * numFrames;

					Frame& frame = frames[ bone.idxFrame ];
					lookUpVector( frame.pos , poitions , positionID );
					lookUpQuaternion( frame.rotation , quaternions , orientationID );

					assert( boneID < numBones );
					boneMap[ boneID ] = &bone;
				}

				int32 curFrame = 1;
				int32 curBone = 0;
				uint32 numLookUp = (numFrames - 1) * numBones;

				for( uint32 i = 0 ; i < numLookUp ; ++i )
				{
					//
					// Normal case for all frames after the first.
					//

					// Read in the offset data.

					uint32 boneID = stream.read< uint32 >();
					uint16 positionID = stream.read< uint16 >();
					uint16 unknownIndex = stream.read< uint16 >(); // Unknown.
					uint16 orientationID = stream.read< uint16 >();

	
					unknownIndex = stream.read< uint16 >(); // Unknown. Seems to always be zero.

					// Retrieve the bone from the dictionary.
					// Note: The bones appear to be in the same order in every frame.  So, a dictionary
					// isn't exactly needed and you could probably get away with a list.  However, this way
					// feels safer just in case something ends up being out of order.
					Bone* pBone  = boneMap[boneID];         
					Frame& frame = frames[ pBone->idxFrame + curFrame ];

					lookUpVector( frame.pos , poitions , positionID );
					lookUpQuaternion( frame.rotation , quaternions , orientationID );

					// This loop is slightly ambiguous.  
					//
					// The problem is previous .anm versions contain data like:
					// foreach bone
					//      foreach frame
					//
					// However, this version contains data like:
					// foreach frame
					//      foreach bone
					//
					// So, reading one version is going to be a little goofy.
					curBone++;
					if (curBone >= numBones )
					{
						curBone = 0;
						curFrame++;
					}
				}
				return false;
			}
			return true;
		}


	}//namespace LoL

	struct LoLModel
	{
		String name;
		String texName;
		String sknName;
		String sklName;
	};

	enum InibinHashID
	{
		SKIN_NAME_1    = 2142495409L,
		SKIN_SKN_1     = 769344815L,
		SKIN_TEXTURE_1 = -1654783749L,
		SKIN_SKL_1     = 1895303501L,

		SKIN_NAME_2    = 1742464788L,
		SKIN_SKN_2     = 306040338L,
		SKIN_TEXTURE_2 = 664710616L,
		SKIN_SKL_2     = 599757744L,

		SKIN_NAME_3    = 1652845395L,
		SKIN_SKN_3     = 525951185L,
		SKIN_TEXTURE_3 = -88577063L,
		SKIN_SKL_3     = -719956497L,

		SKIN_NAME_4    = 1563226002L,
		SKIN_SKN_4     = 745862032L,
		SKIN_TEXTURE_4 = -841864742L,
		SKIN_SKL_4     = -2039670738L,

		SKIN_NAME_5    = 1473606609L,
		SKIN_SKN_5     = 965772879L,
		SKIN_TEXTURE_5 = -1595152421L,
		SKIN_SKL_5     = 935582317L,

		SKIN_NAME_6    = 1383987216L,
		SKIN_SKN_6     = 1185683726L,
		SKIN_TEXTURE_6 = 1946527196L,
		SKIN_SKL_6     = -384131924L,

		SKIN_NAME_7    = 1294367823L,
		SKIN_SKN_7     = 1405594573L,
		SKIN_TEXTURE_7 = 1193239517L,
		SKIN_SKL_7     = -1703846165L,

		SKIN_NAME_8    = 1204748430L,
		SKIN_SKN_8     = 1625505420L,
		SKIN_TEXTURE_8 = 439951838L,
		SKIN_SKL_8     = 1271406890L,

		// Keys from original code.
		// LOLModel Viewer does not use them.  So, I have never confirmed their correctness.
		// However, the originals are here if anyone needs to extend this class.
		PROP_ID = 2921476548L,
		PROP_HP = 742042233L,
		PROP_HPL = 3306821199L,
		PROP_MANA = 742370228L,
		PROP_MANAL = 1003217290L,
		PROP_MOVE = 1081768566L,
		PROP_ARMOR = 2599053023L,
		PROP_ARMORL = 1608827366L,
		PROP_MR = 1395891205L,
		PROP_MRL = 4032178956L,
		PROP_HP5 = 4128291318L,
		PROP_HP5L = 3062102972L,
		PROP_MP5 = 619143803L,
		PROP_MP5L = 1248483905L,
		PROP_BASE_ASPD_MINUS = 2191293239L,
		PROP_ASPDL = 770205030L,
		PROP_AD = 1880118880L,
		PROP_ADL = 1139868982L,
		PROP_RANGE = 1387461685L,
	};

	struct ModelSkinHash
	{
		int32 id[4];
	};

	ModelSkinHash const gModelSkinHash[8] =
	{
		{ SKIN_NAME_1 , SKIN_SKN_1 , SKIN_SKL_1 , SKIN_TEXTURE_1 } ,
		{ SKIN_NAME_2 , SKIN_SKN_2 , SKIN_SKL_2 , SKIN_TEXTURE_2 } ,
		{ SKIN_NAME_3 , SKIN_SKN_3 , SKIN_SKL_3 , SKIN_TEXTURE_3 } ,
		{ SKIN_NAME_4 , SKIN_SKN_4 , SKIN_SKL_4 , SKIN_TEXTURE_4 } ,
		{ SKIN_NAME_5 , SKIN_SKN_5 , SKIN_SKL_5 , SKIN_TEXTURE_5 } ,
		{ SKIN_NAME_6 , SKIN_SKN_6 , SKIN_SKL_6 , SKIN_TEXTURE_6 } ,
		{ SKIN_NAME_7 , SKIN_SKN_7 , SKIN_SKL_7 , SKIN_TEXTURE_7 } ,
		{ SKIN_NAME_8 , SKIN_SKN_8 , SKIN_SKL_8 , SKIN_TEXTURE_8 } ,
	};

	char const* const gSkinDirs[] = 
	{ 
		"Base" , "Skin01" , "Skin02", "Skin03", "Skin04", "Skin05", "Skin06", "Skin07" 
	};
	bool LoLFileImport::loadActor( Actor* actor , char const* dir , char const* name , int indexModelSkin )
	{
		mScene = actor->getScene();

		String baseDir = String( dir ) + "/" + name + "/";
		getScene()->getWorld()->_setUseDirBit( BIT( DIR_ACTOR ) );

		bool isNewSkinPath = false;

		LoL::InibinFile inibinFile;
		
		{
			String path = baseDir + name + ".inibin";
			std::ifstream fs( path.c_str() , std::ios::binary );
			if ( !fs.is_open() )
				return false;
			if ( !inibinFile.load( BinaryStream( fs ) ) )
				return false;
		}

		if ( FileSystem::isExist( ( baseDir + "Skins").c_str() ) )
		{
			isNewSkinPath = true;
			baseDir += String( "Skins/" ) + gSkinDirs[ indexModelSkin ] + "/";
			String path = baseDir + gSkinDirs[ indexModelSkin ] + ".inibin";
			std::ifstream fs( path.c_str() , std::ios::binary );
			if ( !fs.is_open() )
				return false;
			if ( !inibinFile.load( BinaryStream( fs ) ) )
				return false;
		}

		int idxSkin = ( isNewSkinPath ) ? 0 : indexModelSkin;
		LoLModel model;
		model.name    = inibinFile.getStringProp( gModelSkinHash[idxSkin].id[0] );
		model.sknName = inibinFile.getStringProp( gModelSkinHash[idxSkin].id[1] );
		model.sklName = inibinFile.getStringProp( gModelSkinHash[idxSkin].id[2] );
		model.texName = inibinFile.getStringProp( gModelSkinHash[idxSkin].id[3] );


		LoL::SKLFile sklFile;
		{
			String path = baseDir + model.sklName;
			std::ifstream fs( path.c_str() , std::ios::binary );
			if ( !fs.is_open() )
				return false;
			if ( !sklFile.load( BinaryStream( fs ) ) )
				return false;
		}
		LoL::SKNFile sknFile;
		{
			String path = baseDir + model.sknName;
			std::ifstream fs( path.c_str() , std::ios::binary );
			if ( !fs.is_open() )
				return false;
			if ( !sknFile.load( BinaryStream( fs ) ) )
				return false;
		}

		Object* skin = actor->createSkin();
		getScene()->getWorld()->setDir( DIR_ACTOR , baseDir.c_str() );
		if ( !createMesh( skin , sknFile , sklFile , model.texName.c_str() ) )
		{
			skin->release();
			return false;
		}

		if ( !setupSkeleton( actor->getSkeleton() , sklFile ) )
			return false;

		{
			ANMMap anms;
			String dir  = baseDir + "Animations/";
			FileIterator fileIter;
			if ( FileSystem::findFile( dir.c_str() , ".anm" , fileIter ) )
			{
				for( ;fileIter.haveMore();fileIter.goNext() )
				{
					loadANMFile( anms , dir , fileIter.getFileName() );
				}
			}
			setupAnimation( actor , sklFile , anms );
			releaseANMFile( anms );
		}

		actor->updateSkeleton( false );
		return true;
	}


	bool LoLFileImport::loadANMFile( ANMMap& anms , String const& dir , String const& fileName )
	{
		String path = dir + fileName;
		std::ifstream fs( path.c_str() , std::ios::binary );
		if ( !fs.is_open() )
			return false;
		LoL::ANMFile* anmFile = new LoL::ANMFile;
		if ( !anmFile->load( BinaryStream( fs ) ) )
		{
			delete anmFile;
			return false;
		}
		String name( fileName.begin() , fileName.begin() + fileName.find_last_of( '.' ) );
		if ( !anms.insert( std::make_pair( name , anmFile ) ).second )
		{
			delete anmFile;
			return false;
		}
		return true;
	}

	void LoLFileImport::releaseANMFile( ANMMap& anms )
	{
		for( ANMMap::iterator iter( anms.begin() ) , end( anms.end() );
			iter != end ; ++iter )
		{
			delete iter->second;
		}
		anms.clear();
	}

#pragma pack(1)
	struct Vertex
	{
		static int const NumBlendWeight = 3;
		static VertexType const Type = CFVT_XYZ_N | CFVF_BLENDWEIGHTSIZE( NumBlendWeight ) | CFV_BLENDINDICES | CFVF_TEX1( 2 );
		Vector3 pos;
		float   weight[ NumBlendWeight ];
		uint8   boneIndices[ 4 ];
		Vector3 noraml;
		float   texCoord[2];
	};
#pragma pack()

	bool LoLFileImport::createMesh( Object* skin , LoL::SKNFile& skn , LoL::SKLFile& skl , char const* texName)
	{
		assert( sizeof( Vertex ) == 12 * 4 );
		std::vector< Vertex > bufVertex;
		bufVertex.resize( skn.numVertices );

		for( int i = 0 ; i < skn.numVertices ; ++i )
		{
			Vertex& v = bufVertex[i];
			LoL::SKNFile::Vertex& src = skn.vertices[i];

			v.pos = src.pos;
			uint32 offset = 0;
			for ( int n = 0 ; n < 4 ; ++n )
				v.boneIndices[n] = 0;
			int n = 0;
			for(  ; n < Vertex::NumBlendWeight ; ++n )
			{
				v.weight[n] = src.weight[n];
				v.boneIndices[n] = src.bone[ n ] + 1;
			}
			v.boneIndices[n] = src.bone[ n ] + 1;

			v.noraml = src.noraml;
			std::copy( src.texCoord , src.texCoord + 2 , v.texCoord );
		}

		if (skl.version == 2 || skl.version == 0)
		{
			for( int i = 0 ; i < skn.numVertices ; ++i )
			{
				Vertex& v = bufVertex[i];
	
				for(  int n = 0 ; n < 4 ; ++n )
				{
					if ( v.boneIndices[n] )
					{
						uint32 index = (uint32)v.boneIndices[n] - 1;
						if ( index < skl.boneIndexMap.size() )
							v.boneIndices[n] = skl.boneIndexMap[ index ] + 1;
					}
				}
			}
		}

		MeshInfo info;
		info.primitiveType = CFPT_TRIANGLELIST;
		info.vertexType    = Vertex::Type;
		info.flag          = CFVD_SOFTWARE_PROCESSING;
		for( int i = 0 ; i < skn.mats.size() ; ++i )
		{
			LoL::SKNFile::Material& infoMat = skn.mats[i];
			Material* mat = getScene()->getWorld()->createMaterial();
			mat->addTexture( 0 , 0 , texName );
			info.pVertex     = ( &bufVertex[0] + infoMat.startVertex );
			info.numVertices = infoMat.numVertices;
			info.pIndex      = ( &skn.indices[0] + infoMat.startIndex );
			info.numIndices  = infoMat.numIndices;

			skin->createMesh( mat , info );
		}
		return true;
	}

	bool LoLFileImport::setupSkeleton( Skeleton* skeleton , LoL::SKLFile& skl )
	{
		for( int i = 0 ; i < skl.bones.size() ; ++i )
		{
			LoL::SKLFile::Bone& sklBone = skl.bones[i];
			BoneNode* bone = skeleton->createBone( sklBone.name , sklBone.parent + 1 );

			Matrix4 mat;
			mat.setTransform( sklBone.pos , sklBone.rotation );
			float det;
			mat.inverse( bone->invLocalTrans , det );
		}
		return true;
	}

	bool LoLFileImport::setupAnimation(  Actor* actor , LoL::SKLFile& skl , ANMMap& anms )
	{
		Skeleton* skeleton = actor->getSkeleton();
		int totalFrames = 1;
		for( ANMMap::iterator iter( anms.begin() ) , end( anms.end() ) ;
			 iter != end ; ++iter )
		{
			LoL::ANMFile* anm = iter->second;
			totalFrames += anm->numFrames;
		}
		skeleton->createMotionData( totalFrames );

		for( int i = 1 ; i < skeleton->getBoneNum() ; ++i )
		{
			BoneNode* bone = skeleton->getBone( i );
			MotionKeyFrame& frame = bone->keyFrames[0];

			LoL::SKLFile::Bone& sklBone   = skl.bones[ i - 1 ];
			if ( sklBone.parent != -1 )
			{
				LoL::SKLFile::Bone& sklParent = skl.bones[ sklBone.parent ];

				//bone.pos      = parent.pos + parent.rotation.rotate( bone.pos );
				//bone.rotation = parent.rotation * bone.rotation;

				Quaternion qs = sklParent.rotation.inverse();
				frame.pos      = qs.rotate( sklBone.pos - sklParent.pos );
				frame.rotation = qs * sklBone.rotation;
			}
			else
			{
				frame.pos = sklBone.pos;
				frame.rotation = sklBone.rotation;
			}
		}

		int curFrame = 1;
		for( ANMMap::iterator iter( anms.begin() ) , end( anms.end() ) ;
			iter != end ; ++iter )
		{
			LoL::ANMFile* anm = iter->second;

			for( int i = 0 ; i < anm->numBones ; ++i )
			{
				LoL::ANMFile::Bone& anmBone = anm->bones[i];

				BoneNode* bone = 0;
				if ( anm->version == 4 )
				{
					LoL::SKLFile::BoneIdMap::iterator iter = skl.boneIdMap.find( anmBone.id );
					if ( iter != skl.boneIdMap.end() )
						bone = skeleton->getBone( iter->second );
					else
						bone = skeleton->getBone( anmBone.id );
				}
				else
				{
					bone = skeleton->getBone( anmBone.name );
				}
				if ( bone )
				{
					MotionKeyFrame* pFrame = bone->keyFrames + curFrame;
					for( int i = 0 ; i < anm->numFrames ; ++i )
					{
						LoL::ANMFile::Frame& frame = anm->frames[ anmBone.idxFrame + i ];
						pFrame->pos      = frame.pos;
						pFrame->rotation = frame.rotation;
						++pFrame;
					}
				}
			}

			String name( iter->first.begin() + ( iter->first.find_first_of( '_' ) + 1 ) , iter->first.end() );
			toLower( &name[0] );
			actor->createAnimationState( name.c_str() , curFrame , curFrame + anm->numFrames - 1 );
			curFrame += anm->numFrames;
		}

		return true;
	}

	bool LoLFileImport::load( char const* path , Actor* actor , ILoadListener* listener )
	{
		char const* pStr = strrchr( path , '/' );
		if ( pStr == nullptr )
			pStr = strrchr( path , '\\' );
		if ( pStr == nullptr )
			return false;

		String dir( path , pStr );
		String name( pStr + 1 );

		int indexModelSkin = (int)PluginManager::getInstance().getLoaderMeta( DATA_LOL_INDEX_MODEL_SKIN );

		return loadActor( actor , dir.c_str() , name.c_str() , indexModelSkin );
	}


	bool LoLFileLinker::canLoad( EntityType type , char const* subFileName )
	{
		switch( type )
		{
		case ET_ACTOR:
			if ( *subFileName == '\0' )
				return true;
			break;
		}
		return false;
	}

}//namespace LoL