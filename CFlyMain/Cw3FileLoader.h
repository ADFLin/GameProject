#ifndef CWFileLoader_h__
#define CWFileLoader_h__

#include "CFBase.h"

#include "CFFileLoader.h"
#include "CFMaterial.h"
#include "CFMeshBuilder.h"

namespace CFly
{

	class  ILoadListener;

	class TextStream
	{
	public:
		int getLine( char* buf , int maxNum )
		{
			if ( !fgets( buf , maxNum , mFp )  )
				return -1;
			if ( *buf == '\0' || *buf == '\n' )
				return 0;
			return 1;
		}

		bool open( char const* path )
		{
			mFp = fopen( path , "r" );
			return mFp != NULL;
		}
	private:
		FILE* mFp;
	};


	class Cw3FileImport : public IFileImport
	{
	public:

		Cw3FileImport();
		~Cw3FileImport();

		void deleteThis(){ delete this; }

		bool load( char const* path , Object* object , ILoadListener* listener );
		bool load( char const* path , Scene* scene , ILoadListener* listener );
		bool load( char const* path , Actor* actor , ILoadListener* listener );

	private:
		static DWORD getBlendMode( char const* token );
		void setObject( Object* obj ){ mObject = obj; }
		void resetBuffer();
		bool loadModel( char const* path , unsigned reserveTexBit , unsigned flag );
		bool loadModel( TextStream& file , unsigned reserveTexBit , unsigned flag );
		bool loadActor( TextStream& file , Actor* actor );
		bool loadScene( TextStream& file , Scene* scene , ILoadListener* listener );
		bool loadModelData( TextStream& file , char const* strModelEnd , unsigned reserveTexBit , unsigned flag );
		bool createMesh( unsigned flag );

		Object* mObject;
		Scene*  mScene;

		float modelVersion;
		float sceneVersion;

		int   numGroup;
		int   groupIndex[ 16 ];
		int   numMaterial;
		int   numVertex;

		int    mVertexSize;
		float* mVertex;
		uint32 mVertexType;
		int    numSkinWeight;

		static int const MAX_GROUP_NUM = 8;
		Material::RefCountPtr mat[ MAX_GROUP_NUM ];
		int   numTriGroup[ MAX_GROUP_NUM ];
		int*  tri[ MAX_GROUP_NUM ];
		int*  nextTri[ MAX_GROUP_NUM ];
		int   numTri;

		int   numTex;
		int   texLen[ 8 ];
	};

	class Cw3FileLinker : public IFileLinker
	{
	public:
		virtual void deleteThis(){ delete this; }
		virtual bool canLoad( EntityType type , char const* subFileName );
		virtual Cw3FileImport* createLoader(){ return new Cw3FileImport; }
	};


}//namespace CFly


#endif // CWFileLoader_h__