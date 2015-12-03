#ifndef MdlFileLoader_h__
#define MdlFileLoader_h__

#include "CFBase.h"

#include "CFMaterial.h"

namespace CFly
{
	typedef Vector3 vec3_t;
	class  Texture;


#define MAXSTUDIOTRIANGLES	20000	// TODO: tune this
#define MAXSTUDIOVERTS		2048	// TODO: tune this
#define MAXSTUDIOSEQUENCES	256		// total animation sequences
#define MAXSTUDIOSKINS		100		// total textures
#define MAXSTUDIOSRCBONES	512		// bones allowed at source movement
#define MAXSTUDIOBONES		128		// total bones actually used
#define MAXSTUDIOMODELS		32		// sub-models per model
#define MAXSTUDIOBODYPARTS	32
#define MAXSTUDIOGROUPS		16
#define MAXSTUDIOANIMATIONS	512		// per sequence
#define MAXSTUDIOMESHES		256
#define MAXSTUDIOEVENTS		1024
#define MAXSTUDIOPIVOTS		256
#define MAXSTUDIOCONTROLLERS 8




// header for demand loaded sequence group data
struct studioseqhdr_t
{
	int					id;
	int					version;

	char				name[64];
	int					length;
} ;

// bones
struct  mstudiobone_t
{
	char				name[32];	// bone name for symbolic links
	int		 			parent;		// parent bone
	int					flags;		// ??
	int					bonecontroller[6];	// bone controller index, -1 == none
	float				value[6];	// default DoF values
	float				scale[6];   // scale for delta DoF values
};


// bone controllers
struct  mstudiobonecontroller_t
{
	int					bone;	// -1 == 0
	int					type;	// X, Y, Z, XR, YR, ZR, M
	float				start;
	float				end;
	int					rest;	// byte index value at rest
	int					index;	// 0-3 user set controller, 4 mouth
};

// intersection boxes
struct mstudiobbox_t
{
	int					bone;
	int					group;			// intersection group
	vec3_t				bbmin;		// bounding box
	vec3_t				bbmax;		
};

#if !defined( CACHE_USER ) && !defined( QUAKEDEF_H )
#define CACHE_USER
typedef struct cache_user_s
{
	void *data;
} cache_user_t;
#endif

// demand loaded sequence groups
typedef struct
{
	char				label[32];	// textual name
	char				name[64];	// file name
	cache_user_t		cache;		// cache index pointer
	int					data;		// hack for group 0
} mstudioseqgroup_t;

// sequence descriptions
typedef struct
{
	char				label[32];	// sequence label

	float				fps;		// frames per second	
	int					flags;		// looping/non-looping flags

	int					activity;
	int					actweight;

	int					numevents;
	int					eventindex;

	int					numframes;	// number of frames per sequence

	int					numpivots;	// number of foot pivots
	int					pivotindex;

	int					motiontype;	
	int					motionbone;
	vec3_t				linearmovement;
	int					automoveposindex;
	int					automoveangleindex;

	vec3_t				bbmin;		// per sequence bounding box
	vec3_t				bbmax;		

	int					numblends;
	int					animindex;		// mstudioanim_t pointer relative to start of sequence group data
										// [blend][bone][X, Y, Z, XR, YR, ZR]

	int					blendtype[2];	// X, Y, Z, XR, YR, ZR
	float				blendstart[2];	// starting value
	float				blendend[2];	// ending value
	int					blendparent;

	int					seqgroup;		// sequence group for demand loading

	int					entrynode;		// transition node at entry
	int					exitnode;		// transition node at exit
	int					nodeflags;		// transition rules
	
	int					nextseq;		// auto advancing sequences
} mstudioseqdesc_t;




// events
//#include "studio_event.h"

typedef struct 
{
	int 				frame;
	int					event;
	int					type;
	char				options[64];
} mstudioevent_t;


// pivots
typedef struct 
{
	vec3_t				org;	// pivot point
	int					start;
	int					end;
} mstudiopivot_t;

// attachment
typedef struct 
{
	char				name[32];
	int					type;
	int					bone;
	vec3_t				org;	// attachment point
	vec3_t				vectors[3];
} mstudioattachment_t;

typedef struct
{
	unsigned short	offset[6];
} mstudioanim_t;

// animation frames
typedef union 
{
	struct {
		uint8	valid;
		uint8	total;
	} num;
	short		value;
} mstudioanimvalue_t;



// body part index
typedef struct
{
	char				name[64];
	int					nummodels;
	int					base;
	int					modelindex; // index into models array
} mstudiobodyparts_t;



// skin info
typedef struct
{
	char					name[64];
	int						flags;
	int						width;
	int						height;
	int						index;
} mstudiotexture_t;


// skin families
// short	index[skinfamilies][skinref]

// studio models
typedef struct
{
	char				name[64];

	int					type;

	float				boundingradius;

	int					nummesh;
	int					meshindex;

	int					numverts;		// number of unique vertices
	int					vertinfoindex;	// vertex bone info
	int					vertindex;		// vertex vec3_t
	int					numnorms;		// number of unique surface normals
	int					norminfoindex;	// normal bone info
	int					normindex;		// normal vec3_t

	int					numgroups;		// deformation groups
	int					groupindex;
} mstudiomodel_t;


// vec3_t	boundingbox[model][bone][2];	// complex intersection info


// meshes
typedef struct 
{
	int					numtris;
	int					triindex;
	int					skinref;
	int					numnorms;		// per mesh normals
	int					normindex;		// normal vec3_t
} mstudiomesh_t;


struct  studiohdr_t
{
	int					id;
	int					version;

	char				name[64];
	int					length;

	vec3_t				eyeposition;	// ideal eye position
	vec3_t				min;			// ideal movement hull size
	vec3_t				max;			

	vec3_t				bbmin;			// clipping bounding box
	vec3_t				bbmax;		

	int					flags;

	int					numbones;			// bones
	int					boneindex;

	int					numbonecontrollers;		// bone controllers
	int					bonecontrollerindex;

	int					numhitboxes;			// complex bounding boxes
	int					hitboxindex;			

	int					numseq;				// animation sequences
	int					seqindex;

	int					numseqgroups;		// demand loaded sequences
	int					seqgroupindex;

	int					numtextures;		// raw textures
	int					textureindex;
	int					texturedataindex;

	int					numskinref;			// replaceable textures
	int					numskinfamilies;
	int					skinindex;

	int					numbodyparts;		
	int					bodypartindex;

	int					numattachments;		// queryable attachable points
	int					attachmentindex;

	int					soundtable;
	int					soundindex;
	int					soundgroups;
	int					soundgroupindex;

	int					numtransitions;		// animation node to animation node transition graph
	int					transitionindex;



	mstudioattachment_t*     getAttachments(){ return getObject< mstudioattachment_t >( attachmentindex ); }
	mstudiobone_t*           getBones()     { return getObject<mstudiobone_t >( boneindex ); }
	mstudioseqdesc_t*        getSequences() { return getObject< mstudioseqdesc_t >( seqindex ); }
	mstudiobonecontroller_t* getBoneControllors(){ return getObject< mstudiobonecontroller_t >( bonecontrollerindex ); }

	template< class T >
	T* getObject( int index ){ return reinterpret_cast< T* >( reinterpret_cast< uint8* >( this ) + index ); }
};

// triangles
#if 0
typedef struct 
{
	short				vertindex;		// index into vertex array
	short				normindex;		// index into normal array
	short				s,t;			// s,t position on skin
} mstudiotrivert_t;
#endif

// lighting options
#define STUDIO_NF_FLATSHADE		0x0001
#define STUDIO_NF_CHROME		0x0002
#define STUDIO_NF_FULLBRIGHT	0x0004

// motion flags
#define STUDIO_X		0x0001
#define STUDIO_Y		0x0002	
#define STUDIO_Z		0x0004
#define STUDIO_XR		0x0008
#define STUDIO_YR		0x0010
#define STUDIO_ZR		0x0020
#define STUDIO_LX		0x0040
#define STUDIO_LY		0x0080
#define STUDIO_LZ		0x0100
#define STUDIO_AX		0x0200
#define STUDIO_AY		0x0400
#define STUDIO_AZ		0x0800
#define STUDIO_AXR		0x1000
#define STUDIO_AYR		0x2000
#define STUDIO_AZR		0x4000
#define STUDIO_TYPES	0x7FFF
#define STUDIO_RLOOP	0x8000	// controller that wraps shortest distance

// sequence flags
#define STUDIO_LOOPING	0x0001

// bone flags
#define STUDIO_HAS_NORMALS	0x0001
#define STUDIO_HAS_VERTICES 0x0002
#define STUDIO_HAS_BBOX		0x0004
#define STUDIO_HAS_CHROME	0x0008	// if any of the textures have chrome on them

#define RAD_TO_STUDIO		(32768.0/M_PI)
#define STUDIO_TO_RAD		(M_PI/32768.0)


	class Skeleton;


#if 0
	class MDLController : public IBoneController
	{
	public:

		struct CtrlBoneInfo 
		{
		};

		void setParamValue( float flValue )
		{
			mstudiobonecontroller_t	*pbonecontroller = (mstudiobonecontroller_t *)((byte *)m_pstudiohdr + m_pstudiohdr->bonecontrollerindex);

			// find first controller that matches the index
			//int i;
			//for ( i = 0; i < m_pstudiohdr->numbonecontrollers; i++, pbonecontroller++)
			//{
			//	if (pbonecontroller->index == iController)
			//		break;
			//}

			// wrap 0..360 if it's a rotational controller
			if (pbonecontroller->type & (STUDIO_XR | STUDIO_YR | STUDIO_ZR))
			{
				// ugly hack, invert value if end < start
				if (pbonecontroller->end < pbonecontroller->start)
					flValue = -flValue;

				// does the controller not wrap?
				if (pbonecontroller->start + 359.0 >= pbonecontroller->end)
				{
					if (flValue > ((pbonecontroller->start + pbonecontroller->end) / 2.0) + 180)
						flValue = flValue - 360;
					if (flValue < ((pbonecontroller->start + pbonecontroller->end) / 2.0) - 180)
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

			int setting = 255 * (flValue - pbonecontroller->start) / (pbonecontroller->end - pbonecontroller->start);

			if (setting < 0) setting = 0;
			if (setting > 255) setting = 255;

			mValue = setting;
			return setting * (1.0 / 255.0) * (pbonecontroller->end - pbonecontroller->start) + pbonecontroller->start;
		}

		void calcBoneAdj()
		{
			for ( int i = 0; i < mNumBoneControllor; i++ )
			{
				mstudiobonecontroller_t& boneControllor = mBoneControllor[i];

				int     index = boneControllor.index;
				float	value;
				if (index <= 3)
				{
					// check for 360% wrapping
					if (boneControllor.type & STUDIO_RLOOP)
					{
						value = mControllValue[index] * (360.0/256.0) + boneControllor.start;
					}
					else 
					{
						value = mControllValue[index] / 255.0;
						if (value < 0) value = 0;
						if (value > 1.0) value = 1.0;
						value = (1.0 - value) * boneControllor.start + value * boneControllor.end;
					}
					// Con_DPrintf( "%d %d %f : %f\n", m_controller[j], m_prevcontroller[j], value, dadt );
				}
				else
				{
					value = mMouth / 64.0;
					if (value > 1.0) value = 1.0;
					value = (1.0 - value) * boneControllor.start + value * boneControllor.end;
					// Con_DPrintf("%d %f\n", mouthopen, value );
				}
				switch( boneControllor.type & STUDIO_TYPES )
				{
				case STUDIO_XR:
				case STUDIO_YR:
				case STUDIO_ZR:
					mAdj[i] = value * ( Math::PI / 180.0);
					break;
				case STUDIO_X:
				case STUDIO_Y:
				case STUDIO_Z:
					mAdj[i] = value;
					break;
				}
			}
		}

	private:
		float mMouth;
		float mControllValue[];
		int   mNumBoneControllor;
		mstudiobonecontroller_t* mBoneControllor;
		float mAdj[ 4 ];
		float mValue;
	};
#endif

	class MdlFileImport
	{
	public:
		MdlFileImport( Scene* scene );
		~MdlFileImport();


		Actor*  loadActor( char const* path );
		Object* loadObject( char const* path );


		studiohdr_t* getStudio(){ return mStudioHeader; }

	private:
		void         queryAttachment();
		bool         load( char const* path );
		void         loadBone( Actor* actor );
		void         loadMotionData( Skeleton* skeleton , mstudioseqdesc_t& curSeq , int frame , int offset );
		Texture*     loadTexture( mstudiotexture_t& texDesc , uint8* data );
		void         loadTexture( studiohdr_t* studioHeader );
		void         loadModel( Object* obj , mstudiomodel_t const& modelHeader , bool haveBW , unsigned flag );
		mstudioanim_t* getAnimations( mstudioseqdesc_t& seqDesc );

		void getBoneQuaternion( int frame, mstudiobone_t& boneDesc , mstudioanim_t& animDesc , Quaternion& q );
		void getBonePosition( int frame , mstudiobone_t& boneDesc , mstudioanim_t& animDesc , Vector3& pos );
		void getSeqenceTransform( mstudioseqdesc_t& seqDesc , mstudioanim_t* animDescs , int frame , Vector3 pos[] , Quaternion q[] );

		mstudiobonecontroller_t* getBoneControllers(){ return (mstudiobonecontroller_t *)( mData + mStudioHeader->bonecontrollerindex ); }
		float  calcMouth( float flValue );


		studioseqhdr_t* loadDemandSequences( char const* path);
		static uint8*    loadFileData( char const* path );
		static void     freeFileData( uint8* ptr );
		void            slerpBones( Quaternion q1[], vec3_t pos1[], Quaternion q2[], vec3_t pos2[], float s );

		Scene*          mScene;
		uint8*           mData;
		Texture::RefPtr mTexture[32];
		int             mNumTexture;
		studioseqhdr_t* mAnimSeqHeader[ 32 ];
		studiohdr_t*    mTextureHeader;
		studiohdr_t*    mStudioHeader;
	};

}//namespace CFly

#endif // MdlFileLoader_h__