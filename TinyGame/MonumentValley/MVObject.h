#ifndef MVObject_h__
#define MVObject_h__

#include "MVCommon.h"

#include "IntrList.h"

namespace MV
{

	enum NavFunType
	{
		NFT_NULL   = 0 ,
		NFT_LADDER = 1 ,
		NFT_PLANE  = 2 ,
		NFT_BLOCK  = 3 ,
		NFT_STAIR  = 4,
		NFT_PLANE_LADDER = 5,
		NFT_PASS_VIEW = 6,
		NFT_ROTATOR_C = 7,
		NFT_ROTATOR_NC = 8,


		NFT_SUPER_PLANE ,
		NFT_ROTATOR_C_B1 ,
		NFT_ROTATOR_C_B2 ,
		NFT_ROTATOR_NC_B1 ,
		NFT_ROTATOR_NC_B2 ,
		NFT_ONE_WAY ,
	};

	enum SurfaceType
	{
		eSurfDirX ,
		eSurfDirInvX ,
		eSurfDirY ,
		eSurfDirInvY ,
		eSurfDirZ ,
		eSurfDirInvZ ,

		eSurfDirXY ,
		eSurfDirInvXY ,
		eSurfDirYZ ,
		eSurfDirInvYZ ,
		eSurfDirZX ,
		eSurfDirInvZX ,
	};

	struct SurfaceDef
	{
		NavFunType fun;
		int        meta;
	};

	enum NodeType
	{
		NODE_DIRECT ,
		NODE_PARALLAX ,

		NUM_NODE_TYPE ,
	};

	int const NUM_BLOCK_FACE = 6;
	int const NUM_FACE_NAV_LINK = 4;

	struct NavSurfInfo
	{
		int idxDir;
		BlockSurface* surf;
	};

	struct NavNode
	{
		NavNode()
		{
			flag = 0;
			link = NULL;
		}
		enum FlagMask
		{
			eStatic = 0 << 1 ,
			eFixPos = 1 << 1 ,
		};

		int           getDirIndex(){ return surfInfo->idxDir; }
		BlockSurface* getSurface(){ return surfInfo->surf; }

		uint8         flag;
		NavSurfInfo*  surfInfo;
		NavNode*      link;

		void connect( NavNode& other );

		void disconnect()
		{
			assert( link );
			link->link = NULL;
			link = NULL;
			flag = 0;
		}

	};

	class BlockSurface : public SurfaceDef
	{
	public:
		BlockSurface()
		{
			for( int i = 0 ; i < NUM_FACE_NAV_LINK ; ++i )
			{
				NavSurfInfo* info = surfInfo + i;
				info->idxDir = i;
				info->surf   = this;
				nodes[ NODE_DIRECT ][i].surfInfo = info;
				nodes[ NODE_PARALLAX ][i].surfInfo = info;
			}
		}
		Block*      block;
		NavNode     nodes[ NUM_NODE_TYPE ][ NUM_FACE_NAV_LINK ];
	private:
		NavSurfInfo surfInfo[ NUM_FACE_NAV_LINK ];
	};

	struct Object
	{
		Object()
		{
			group = NULL;
		}
		ObjectGroup* group;
		HookNode     groupHook;
	};

	class Block : public Object
	{
	public:
		Block()
		{
			for( int i = 0 ; i < NUM_BLOCK_FACE; ++i )
				surfaces[i].block = this;
		}
		int          id;
		int          idMesh;
		Vec3i        pos;
		Roataion     rotation;
		ObjectGroup* group;
		BlockSurface surfaces[ NUM_BLOCK_FACE ];
		HookNode     mapHook;
		uint32       updateCount;

		BlockSurface& getLocalFace( Dir dir )
		{
			return surfaces[ dir ];
		}
		BlockSurface& getFace( Dir dir )
		{
			return surfaces[ rotation.toLocal( dir ) ];
		}


		static Dir getWorldDir( BlockSurface& surface )
		{
			return surface.block->rotation.toWorld( getLocalDir( surface ) );
		}
		static Dir getLocalDir( BlockSurface& surface )
		{
			return Dir( &surface - surface.block->surfaces );
		}

	};

	struct MeshObject : public Object
	{
		int      idMesh;
		Vec3f    pos;
		Vec3f    rotation;
	};

	struct Actor : public Object
	{
		Actor()
		{
			actBlockId = 0;
			rotation.set( eDirX , eDirZ );
			actState = eActMove;
			moveOffset = 0;
		}

		Vec3i    pos;
		Roataion rotation;

		//
		Vec3f   renderPos;
		float   moveOffset;
		//float rotateOffset;

		enum ActState
		{
			eActClimb ,
			eActMove ,
			eActSlope ,
			eActRotate ,
		};
		Dir      actFaceDir;
		ActState actState;
		int      actBlockId;


		void rotate( int factor )
		{
			rotation.rotate( rotation[2] , factor );
		}

	};

	typedef IntrList< Block , MemberHook< Object , &Object::groupHook > , PointerType > BlockList;
	typedef IntrList< Actor , MemberHook< Object , &Object::groupHook > , PointerType > ActorList;
	typedef IntrList< MeshObject , MemberHook< Object , &Object::groupHook > , PointerType > MeshList;
	typedef IntrList< ObjectGroup , MemberHook< Object , &Object::groupHook > , PointerType > GroupList;

	class ObjectGroup : public Object
	{
	public:
		ObjectGroup()
		{
			node = NULL;
		}
#define ADD_FUN( TYPE , LIST )\
	void add( TYPE& data )\
		{\
		LIST.push_back( &data );\
		data.group = this;\
		}

		ADD_FUN( Block , blocks )
		ADD_FUN( Actor , actors )
		ADD_FUN( MeshObject , meshs )
		ADD_FUN( ObjectGroup , children )

#undef ADD_FUN

		void remove( Object& obj )
		{
			obj.groupHook.unlink();
			obj.group = NULL;
		}

		template< class Fun >
		void removeAll( Fun& fun )
		{
			while ( !blocks.empty() )
			{
				Block* object = blocks.front();
				remove( *object );
				fun( object );
			}

			while ( !actors.empty() )
			{
				Actor* object = actors.front();
				remove( *object );
				fun( object );
			}

			while ( !meshs.empty() )
			{
				MeshObject* object = meshs.front();
				remove( *object );
				fun( object );
			}

			while ( !children.empty() )
			{
				ObjectGroup* object = children.front();
				remove( *object );
				fun( object );
			}
		}

		int            idx;
		BlockList      blocks;
		ActorList      actors;
		MeshList       meshs;
		GroupList      children;

		IGroupModifier*    node;
		HookNode       spaceHook;
	};


}//namespace MV

#endif // MVObject_h__
