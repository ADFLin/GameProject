#ifndef LoLFileLoader_h__
#define LoLFileLoader_h__

#include "CFBase.h"
#include "CFFileLoader.h"

#include "CFVertexDecl.h"

#include <vector>
#include <map>

namespace CFly
{
	class Skeleton;

	namespace LoL
	{
		class SKNFile
		{
		public:
#pragma pack(1)
			struct Material
			{
				char name[64];    //Name of material
				int  startVertex; //First vertex belonging to this material
				int  numVertices; //Number of vertices in this material
				int  startIndex;  //First index belonging to this material
				int  numIndices;  //Number of indicies belonging to this material
			};
			struct Vertex
			{
				Vector3 pos;
				uint8   bone[4];
				float   weight[4];
				Vector3 noraml;
				float   texCoord[2];
			};
#pragma pack()

			int16 version;
			int   numVertices;
			int   numIndices;
			int   numObjects;
			int   endTab[3];

			std::vector< Material > mats;
			std::vector< Vertex >   vertices;
			std::vector< int16 >    indices;

			bool load( BinaryStream& stream );
		};

		class SKLFile
		{
		public:

			struct Bone
			{
				int32 id;
				char  name[32];
				int32 parent;
				float scale;

				Vector3    pos;
				Quaternion rotation;
			};
			SKLFile()
			{
				designId = 0;
			}

			char     id[8];
			uint32   version;
			uint32   designId;
			std::vector< Bone >         bones;
			//
			std::vector< uint32 >       boneIndexMap;
			//AMN BoneId to SKLBoneId in AMN Version 4

			typedef std::map< uint32 , uint32 > BoneIdMap;
			BoneIdMap boneIdMap;

			bool load( BinaryStream& stream );

		};

		class InibinFile
		{
			typedef int32 KeyType;
		public:

			uint8 version;
			static int const MAX_KEYS_NUM = 1024;
			bool load( BinaryStream& stream );
			char const* getStringProp( KeyType key );

		private:
			int  readSegmentKeys( BinaryStream& stream , KeyType keys[] );

			enum PropType
			{
				PT_INT    ,
				PT_FLOAT  ,
				PT_STRING ,
				PT_COLOR  ,
			};

			template< class T , class C >
			void addPropImpl( KeyType key , T value , PropType type , C& c );
			void addProperty( KeyType key , int32 value );
			void addProperty( KeyType key , float value );
			void addProperty( KeyType key , char* str );
			void addColorProp( KeyType key , float* color );

			struct Prop
			{
				PropType type;
				int      index;
			};

			std::vector< float  >  mFloatProps;
			std::vector< int32 >   mIntProps;
			std::vector< String >  mStringProps;

			typedef std::map< int32 , Prop > PropMap;
			PropMap mMap;

		};

		class ANMFile
		{
		public:
			struct Frame
			{
				Quaternion rotation;
				Vector3    pos;
			};

			struct Bone
			{
				// version 0 1 2 3
				char   name[32];
				// version 4
				uint32 id;

				int32  idxFrame;
			};

			char   id[8];
			uint32 version;
			uint32 magic;
			uint32 numFrames;
			uint32 numBones;
			uint32 playbackFPS;

			std::vector< Bone >  bones;
			std::vector< Frame > frames;

			bool load( BinaryStream& stream );



		};

	}//namespace LoL

	enum LoLDataId
	{
		DATA_LOL_INDEX_MODEL_SKIN ,
	};

	class LoLFileImport : public IFileImport
	{
	public:
		LoLFileImport(){ mScene = nullptr; }

		bool  load( char const* path , Actor* actor , ILoadListener* listener );
		void  deleteThis(){ delete this; }
		bool  loadActor( Actor* actor , char const* dir , char const* name , int indexModelSkin );
	private:
		typedef std::map< String , LoL::ANMFile* > ANMMap;
		
		bool   loadANMFile( ANMMap& anms , String const& dir , String const& fileName );
		void   releaseANMFile( ANMMap& anms );
		bool   createMesh( Object* skin , LoL::SKNFile& skn , LoL::SKLFile& skl , char const* texName );
		bool   setupSkeleton( Skeleton* skeleton , LoL::SKLFile& skl );
		bool   setupAnimation( Actor* actor , LoL::SKLFile& skl , ANMMap& anms );
		bool   loadSkin( Object* skin , char const* path );
		Scene* getScene(){ return mScene; }
		Scene* mScene;
	};

	class LoLFileLinker : public IFileLinker
	{
	public:
		virtual void deleteThis(){ delete this; }
		virtual bool canLoad( EntityType type , char const* subFileName );
		virtual IFileImport* createLoader(){ return new LoLFileImport; }
	};


}//namespace CFly

#endif // LoLFileLoader_h__
