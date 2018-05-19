#include "CFlyPCH.h"
#include "MdlFileLoader.h"

#include <fstream>

#include "CFWorld.h"
#include "CFScene.h"
#include "CFActor.h"
#include "CFObject.h"
#include "CFMaterial.h"
#include "CFMeshBuilder.h"

namespace CFly
{

	MdlFileImport::MdlFileImport( Scene* scene ) 
		:mScene( scene )
		,mData( nullptr )
		,mStudioHeader( nullptr )
		,mTextureHeader( nullptr )
		,mNumTexture( 0 )
	{
		std::fill_n( mAnimSeqHeader , ARRAYSIZE( mAnimSeqHeader ) , (studioseqhdr_t*) nullptr );
	}

	MdlFileImport::~MdlFileImport()
	{
		freeFileData( mData );
		for( int i = 0 ; i < ARRAYSIZE( mAnimSeqHeader ) ; ++i )
		{
			if ( mAnimSeqHeader[i] )
				freeFileData( ( uint8*)mAnimSeqHeader[i] );
		}
	}

	void MdlFileImport::loadTexture( studiohdr_t* studioHeader )
	{
		mstudiotexture_t* texDescVec = (mstudiotexture_t*)( (uint8*)studioHeader  + studioHeader->textureindex );
		for( int i = 0 ; i < studioHeader->numtextures ; ++i )
		{
			Texture* tex = loadTexture(  texDescVec[i] , (uint8*)studioHeader + texDescVec[i].index );

			texDescVec[i].index = mNumTexture;
			mTexture[ mNumTexture ++ ] = tex;

			Material* mat = mScene->getWorld()->createMaterial();
			Object* obj = mScene->createObject( nullptr );
			mat->addTexture( 0 , 0 , tex );
			obj->createPlane( mat , 100 , 100 , NULL , 1 , 1 , Vector3(0,1,0) , Vector3(1,0,0) );
			obj->setLocalPosition( Vector3( i * 100 , -100 , 0 ) );
		}

	}

	uint8* MdlFileImport::loadFileData( char const* path )
	{
		using std::ifstream;
		using std::ios;

		ifstream stream( path , ios::binary );
		if ( !stream )
			return nullptr;

		stream.seekg( 0 , ios::end );
		unsigned size = stream.tellg();

		uint8* out = new uint8[ size ];

		stream.seekg( 0 , ios::beg );
		stream.read( (char*) out , size );

		return out;
	}

	void MdlFileImport::freeFileData( uint8* ptr )
	{
		delete [] ptr;
	}

	Object* MdlFileImport::loadObject( char const* path )
	{
		Object* obj = mScene->createObject();

		if ( obj == nullptr )
			return nullptr;

		if ( ! load( path ) )
			return nullptr;

		mstudiobodyparts_t* bodyHeader = (mstudiobodyparts_t*)( mData + mStudioHeader->bodypartindex );
		for( int i = 0 ; i < mStudioHeader->numbodyparts ; ++i )
		{
			mstudiomodel_t* modelHeader = (mstudiomodel_t*)( mData + bodyHeader[i].modelindex );
			for( int n = 0 ; n < bodyHeader[n].nummodels ; ++n )
			{
				loadModel( obj , modelHeader[n] , false , 0 );
			}
			//loadTexture(  texHeader[i] , mData + bodyHeader[i].);
		}

		for( int i = 0 ; i < getStudio()->numbones; ++i )
		{
			mstudiobone_t& bone = getStudio()->getBones()[i];

			int k = 1;
		}

		return obj;
	}

	Actor* MdlFileImport::loadActor( char const* path )
	{
		Actor* actor = mScene->createActor();

		if ( actor == nullptr )
			return nullptr;

		if ( ! load( path ) )
			return nullptr;

		loadBone( actor );

		World* world = mScene->getWorld();
		mstudiobodyparts_t* bodyHeader = (mstudiobodyparts_t*)( mData + mStudioHeader->bodypartindex );
		for( int i = 0 ; i < mStudioHeader->numbodyparts ; ++i )
		{
			mstudiomodel_t* modelHeader = (mstudiomodel_t*)( mData + bodyHeader[i].modelindex );
			for( int n = 0 ; n < bodyHeader[n].nummodels ; ++n )
			{
				Object* skin = actor->createSkin();
				loadModel( skin , modelHeader[n] , true , CFVD_SOFTWARE_PROCESSING );
			}
			//loadTexture(  texHeader[i] , mData + bodyHeader[i].);
		}

		queryAttachment();
		actor->getSkeleton()->useInvLocalTransform( false );
		actor->updateSkeleton( false );
		return actor;
	}

	bool MdlFileImport::load( char const* path )
	{
		mData = loadFileData( path );
		if ( !mData )
			return false;

		mStudioHeader = (studiohdr_t*)mData;
		loadTexture( mStudioHeader );

		mTextureHeader = mStudioHeader;

		if ( mStudioHeader->numseqgroups > 1 )
		{
			for (int i = 1; i < mStudioHeader->numseqgroups; i++)
			{
				char seqGroupName[256];

				strcpy( seqGroupName, path );
				sprintf( &seqGroupName[strlen(seqGroupName) - 4], "%02d.mdl", i );

				mAnimSeqHeader[i] = ( studioseqhdr_t* ) loadFileData( seqGroupName );
			}
		}
		return true;

	}


	static int fillModelMesh( MeshBuilder& builder , mstudiotexture_t& texDesc , short* tri  ,  Vector3 vtxPos[] , uint8* vtxBone )
	{
		int numTri = 0;

		float fs = 1.0f / texDesc.width;
		float ft = 1.0f / texDesc.height;
		int base = 0;

		if ( vtxBone )
		{
			while (1)
			{
				short nEle = *(tri++);
				if ( nEle == 0)
					break;

				if ( nEle < 0 )
				{
					//triangle fan
					nEle = -nEle;

					builder.setPosition(  vtxPos[ tri[0] ] );
					builder.setBlendIndices( vtxBone[ tri[0] ] + 1 );
					builder.setTexCoord( 0 , fs * tri[2] , ft * tri[3] );
					builder.addVertex();
					int start = base++;
					tri += 4;

					builder.setPosition(  vtxPos[ tri[0] ] );
					builder.setBlendIndices( vtxBone[ tri[0] ] + 1 );
					builder.setTexCoord( 0 , fs * tri[2] , ft * tri[3] );
					builder.addVertex();
					int prev = base++;
					tri += 4;

					for( int n = 2 ; n < nEle ; ++n )
					{
						builder.setPosition(  vtxPos[ tri[0] ] );
						builder.setBlendIndices( vtxBone[ tri[0] ] + 1 );
						builder.setTexCoord( 0 , fs * tri[2] , ft * tri[3] );
						builder.addVertex();

						int cur = base++;
						builder.addTriangle( start , cur , prev  );

						++numTri;

						prev = cur;
						tri += 4;
					}
				}
				else 
				{
					//triangle strip
					builder.setPosition(  vtxPos[ tri[0] ] );
					builder.setBlendIndices( vtxBone[ tri[0] ] + 1 );
					builder.setTexCoord( 0 , fs * tri[2] , ft * tri[3] );
					builder.addVertex();
					int prev2 = base++;
					tri += 4;

					builder.setPosition(  vtxPos[ tri[0] ] );
					builder.setBlendIndices( vtxBone[ tri[0] ] + 1 );
					builder.setTexCoord( 0 , fs * tri[2] , ft * tri[3] );
					builder.addVertex();
					int prev = base++;
					tri += 4;

					for( int n = 2 ; n < nEle ; ++n )
					{
						builder.setPosition(  vtxPos[ tri[0] ] );
						builder.setBlendIndices( vtxBone[ tri[0] ] + 1 );
						builder.setTexCoord( 0 , fs * tri[2] , ft * tri[3] );
						builder.addVertex();

						int cur = base++;

						if ( n % 2 == 0 )
							builder.addTriangle( prev2 , cur , prev );
						else
							builder.addTriangle( prev2  , prev , cur );

						++numTri;

						prev2 = prev;
						prev = cur;
						tri += 4;
					}
				}
			}
		}
		else //vtxBone
		{
			while (1)
			{
				short nEle = *(tri++);
				if ( nEle == 0)
					break;

				if ( nEle < 0 )
				{
					//triangle fan
					nEle = -nEle;

					builder.setPosition(  vtxPos[ tri[0] ] );
					builder.setTexCoord( 0 , fs * tri[2] , ft * tri[3] );
					builder.addVertex();
					int start = base++;
					tri += 4;

					builder.setPosition(  vtxPos[ tri[0] ] );
					builder.setTexCoord( 0 , fs * tri[2] , ft * tri[3] );
					builder.addVertex();
					int prev = base++;
					tri += 4;

					for( int n = 2 ; n < nEle ; ++n )
					{
						builder.setPosition(  vtxPos[ tri[0] ] );
						builder.setTexCoord( 0 , fs * tri[2] , ft * tri[3] );
						builder.addVertex();

						int cur = base++;
						builder.addTriangle( start , cur , prev  );

						++numTri;

						prev = cur;
						tri += 4;
					}
				}
				else 
				{
					//triangle strip
					builder.setPosition(  vtxPos[ tri[0] ] );
					builder.setTexCoord( 0 , fs * tri[2] , ft * tri[3] );
					builder.addVertex();
					int prev2 = base++;
					tri += 4;

					builder.setPosition(  vtxPos[ tri[0] ] );
					builder.setTexCoord( 0 , fs * tri[2] , ft * tri[3] );
					builder.addVertex();
					int prev = base++;
					tri += 4;

					for( int n = 2 ; n < nEle ; ++n )
					{
						builder.setPosition(  vtxPos[ tri[0] ] );
						builder.setTexCoord( 0 , fs * tri[2] , ft * tri[3] );
						builder.addVertex();

						int cur = base++;

						if ( n % 2 == 0 )
							builder.addTriangle( prev2 , cur , prev );
						else
							builder.addTriangle( prev2 , prev , cur );

						++numTri;

						prev2 = prev;
						prev = cur;
						tri += 4;
					}
				}
			}
		}
		return numTri;
	}

	void MdlFileImport::loadModel( Object* obj , mstudiomodel_t const& modelHeader , bool haveBW , unsigned flag )
	{

		mstudiomesh_t* meshVec = ( mstudiomesh_t*)( mData + modelHeader.meshindex );

		int useSkinIdx = 0;
		mstudiotexture_t* texDescVec = (mstudiotexture_t *)((uint8 *)mTextureHeader + mTextureHeader->textureindex );

		short* skinRefVec = (short *)((uint8 *)mTextureHeader + mTextureHeader->skinindex);
		if ( useSkinIdx != 0 && useSkinIdx < mTextureHeader->numskinfamilies )
			skinRefVec += (useSkinIdx * mTextureHeader->numskinref );

		vec3_t* pos = reinterpret_cast< vec3_t* >( mData + modelHeader.vertindex );
		vec3_t* normal = reinterpret_cast< vec3_t* >( mData + modelHeader.normindex );

		VertexType vertexType = CFVT_XYZ | CFVF_TEX1( 2 );
		if ( haveBW )
			vertexType |= CFV_BLENDINDICES;

		MeshBuilder builder( vertexType );
		builder.setFlag( flag );

		uint8* vtxBone = haveBW ? ( mData + modelHeader.vertinfoindex ) : 0;

		for( int i = 0 ; i < modelHeader.nummesh ; ++i )
		{
			mstudiomesh_t& curMesh = meshVec[i];
			mstudiotexture_t& texDesc = texDescVec[ skinRefVec[curMesh.skinref] ];

			if ( texDesc.flags & STUDIO_NF_CHROME )
				continue;

			short* tri = (short*)(mData + curMesh.triindex );

			builder.reserveIndexBuffer( 3 * curMesh.numtris );
			builder.resetBuffer();

			int numTri = fillModelMesh( builder , texDesc , tri , pos , vtxBone );

			if ( numTri != curMesh.numtris )
			{
				LogDevMsg( 1 , "Warmming Load Model Mesh Error" );
				continue;
			}

			Material* mat = mScene->getWorld()->createMaterial();
			mat->addTexture( 0 , 0 , mTexture[ texDesc.index ] );

			builder.createIndexTrangle( obj , mat );

		}
	}



	Texture* MdlFileImport::loadTexture( mstudiotexture_t& texDesc , uint8* data )
	{
		int texArea = texDesc.height * texDesc.width;
		uint8* pal = data + texArea;

		std::vector< uint8 > texData( texArea * 4 );

		uint32* pColor = (uint32*) &texData[0];
		for( int i = 0 ; i < texArea ; ++i )
		{
			uint8* palC = pal + 3 * data[i];
			pColor[i] = FColor::ToARGB( palC[0] , palC[1], palC[2] , 255 );
		}

		MaterialManager* manager = mScene->getWorld()->_getMaterialManager();
		Texture* texture =  manager->createTexture2D( texDesc.name , CF_TEX_FMT_ARGB32 , &texData[0] , texDesc.width , texDesc.height );

		return texture;
	}

	mstudioanim_t* MdlFileImport::getAnimations( mstudioseqdesc_t& seqDesc )
	{
		if ( seqDesc.seqgroup == 0 )
		{
			mstudioseqgroup_t* seqGroup = (mstudioseqgroup_t *)( mData + mStudioHeader->seqgroupindex ) + seqDesc.seqgroup;
			return (mstudioanim_t *)( mData + seqGroup->data + seqDesc.animindex );
		}
		return (mstudioanim_t *)((uint8*)mAnimSeqHeader[seqDesc.seqgroup] + seqDesc.animindex);
	}

	void MdlFileImport::getBoneQuaternion( int frame, mstudiobone_t& boneDesc , mstudioanim_t& animDesc , Quaternion& q )
	{
		Vector3 angle;

		for ( int i = 0; i < 3; ++i )
		{
			if ( animDesc.offset[i+3] == 0 )
			{
				angle[i] = boneDesc.value[i+3]; // default;
			}
			else
			{
				mstudioanimvalue_t* animValue = ( mstudioanimvalue_t *)((uint8 *)&animDesc + animDesc.offset[i+3] );
				int k = frame;
				while ( animValue->num.total <= k)
				{
					k -= animValue->num.total;
					animValue += animValue->num.valid + 1;
				}
				// Bah, missing blend!
				if ( animValue->num.valid > k )
				{
					angle[i] = animValue[k+1].value;
				}
				else
				{
					angle[i] = animValue[ animValue->num.valid ].value;
				}
				angle[i] = boneDesc.value[i+3] + angle[i] * boneDesc.scale[i+3];
			}

			//if ( boneDesc.bonecontroller[j+3] != -1)
			//{
			//	angle1[j] += m_adj[boneDesc.bonecontroller[i+3]];
			//	angle2[j] += m_adj[boneDesc.bonecontroller[i+3]];
			//}
		}

		q.setEulerZYX( angle[2] ,  angle[1] ,  angle[0] );
	}


	void MdlFileImport::getBonePosition( int frame , mstudiobone_t& boneDesc , mstudioanim_t& animDesc , Vector3& pos )
	{
		for ( int j = 0; j < 3; ++j )
		{
			pos[j] = boneDesc.value[j];

			if ( animDesc.offset[j] )
			{
				mstudioanimvalue_t* animValue = ( mstudioanimvalue_t *)((uint8 *)&animDesc + animDesc.offset[j] );
				int k = frame;
				while ( animValue->num.total <= k)
				{
					k -= animValue->num.total;
					animValue += animValue->num.valid + 1;
				}
				// if we're inside the span
				if ( animValue->num.valid > k)
				{
					// and there's more data in the span
					pos[j] += animValue[k+1].value * boneDesc.scale[j];
				}
				else
				{
					// are we at the end of the repeating values section and there's another section with data?
					pos[j] += animValue[animValue->num.valid].value * boneDesc.scale[j];
				}
			}
			//if (pbone->bonecontroller[j] != -1)
			//{
			//	pos[j] += m_adj[pbone->bonecontroller[j]];
			//}
		}
	}

	void MdlFileImport::getSeqenceTransform( mstudioseqdesc_t& seqDesc , mstudioanim_t* animDescs , int frame , Vector3 pos[] , Quaternion q[] )
	{
		// add in programatic controllers
		//CalcBoneAdj( );

		mstudiobone_t* boneDescs = (mstudiobone_t *)( mData + mStudioHeader->boneindex);
		for ( int i = 0; i < mStudioHeader->numbones; i++ ) 
		{
			getBoneQuaternion( frame, boneDescs[i] , animDescs[i] , q[i] );
			getBonePosition( frame, boneDescs[i], animDescs[i] , pos[i] );
		}

		if ( seqDesc.motiontype & STUDIO_X)
			pos[seqDesc.motionbone][0] = 0.0;
		if ( seqDesc.motiontype & STUDIO_Y)
			pos[seqDesc.motionbone][1] = 0.0;
		if ( seqDesc.motiontype & STUDIO_Z)
			pos[seqDesc.motionbone][2] = 0.0;
	}

	void MdlFileImport::slerpBones( Quaternion q1[], vec3_t pos1[], Quaternion q2[], vec3_t pos2[], float s )
	{
		s = Math::Clamp<float>( s , 0 , 1.0f );

		for ( int i = 0; i < mStudioHeader->numbones; i++)
		{
			q1[i]   = Math::Slerp( q1[1] , q2[i] , s );
			pos1[i] = Math::Lerp( pos1[i] , pos2[i] , s );
		}
	}

	void MdlFileImport::loadMotionData( Skeleton* skeleton , mstudioseqdesc_t& curSeq  , int frame , int offset )
	{
		Quaternion  q[ 128 ];
		Vector3     pos[ 128 ];

		mstudioanim_t*    animDescVec = getAnimations( curSeq  );

		getSeqenceTransform( curSeq , animDescVec , frame , pos , q );

		//if ( curSeq.numblends > 1 )
		//{
		//	Quaternion  q2[ 128 ];
		//	Vector3     pos2[ 128 ];
		//	getSeqenceTransform( curSeq , animDescVec + mStudioHeader->numbones , frame , pos2 , q2 );
		//	slerpBones( q , pos , q2 , pos2 , 0.5f );
		//}

		mstudiobone_t* boneVec = mStudioHeader->getBones();
		for ( int i = 0; i < mStudioHeader->numbones; i++ ) 
		{
			BoneNode* bone = skeleton->getBone( i + 1 );

			MotionKeyFrame& motion = bone->keyFrames[ offset + frame ];

			motion.rotation = q[i];
			motion.pos = pos[i];
		}

	}


	float MdlFileImport::calcMouth( float flValue )
	{
		mstudiobonecontroller_t* boneController = getBoneControllers();

		// find first controller that matches the mouth
		for (int i = 0; i < mStudioHeader->numbonecontrollers; i++ )
		{
			if ( boneController->index == 4 )
				break;

			++boneController;
		}

		// wrap 0..360 if it's a rotational controller
		if ( boneController->type & (STUDIO_XR | STUDIO_YR | STUDIO_ZR))
		{
			// ugly hack, invert value if end < start
			if (boneController->end < boneController->start)
				flValue = -flValue;

			// does the controller not wrap?
			if (boneController->start + 359.0 >= boneController->end)
			{
				if (flValue > ((boneController->start + boneController->end) / 2.0) + 180)
					flValue = flValue - 360;
				if (flValue < ((boneController->start + boneController->end) / 2.0) - 180)
					flValue = flValue + 360;
			}
			else
			{
				if (flValue > 360)
					flValue = flValue - (int)(flValue / 360.0) * 360.0;
				else if (flValue < 0)
					flValue = flValue + (int)((flValue / -360.0) + 1) * 360.0;
			}
		}

		float setting = (flValue - boneController->start) / (boneController->end - boneController->start);

		if (setting < 0) setting = 0;
		if (setting > 1.0f ) setting = 1.0f;

		return setting;
	}

	void MdlFileImport::loadBone( Actor* actor )
	{
		Skeleton* skeleton = actor->getSkeleton();

		mstudiobonecontroller_t* boneCtrl = mStudioHeader->getBoneControllors();
		for( int i = 0 ; i < mStudioHeader->numbonecontrollers ; ++i )
		{




		}

		mstudiobone_t* boneVec = mStudioHeader->getBones();
		assert( skeleton->getBoneNum() == 1 );
		for( int i = 0 ; i < mStudioHeader->numbones; ++i )
		{
			mstudiobone_t& boneDesc = boneVec[i];
			BoneNode* bone;
			skeleton->createBone( boneDesc.name , boneDesc.parent + 1 );

			for( int n = 0 ; n < 6 ;++n )
			{
				if ( boneDesc.bonecontroller[n] != -1 )
				{
					int kk = 1;
				}
			}
		}

		int totalFrame = 0;
		mstudioseqdesc_t* seqVec = mStudioHeader->getSequences();
		for ( int i = 0 ; i < mStudioHeader->numseq ; ++i )
		{
			mstudioseqdesc_t& curSeq = seqVec[i];
			AnimationState* state = actor->createAnimationState( curSeq.label , totalFrame ,  totalFrame + seqVec[i].numframes - 1 );

			state->setTimeScale( curSeq.fps / 30.0f );
			totalFrame += curSeq.numframes;
		}

		skeleton->createMotionData( totalFrame );

		totalFrame = 0;
		for ( int i = 0 ; i < mStudioHeader->numseq ; ++i )
		{
			mstudioseqdesc_t& curSeq = seqVec[i];

			for( int frame = 0 ; frame < curSeq.numframes ; ++frame )
			{
				loadMotionData( skeleton , curSeq , frame , totalFrame );
			}
			totalFrame += curSeq.numframes;
		}
	}

	void MdlFileImport::queryAttachment()
	{
		mstudioattachment_t* attchVec = mStudioHeader->getAttachments();

		for( int i = 0 ; i < mStudioHeader->numattachments ; ++i )
		{
			mstudioattachment_t& attch = attchVec[i];

			int j = 1;
		}
	}

}//namespace CFly