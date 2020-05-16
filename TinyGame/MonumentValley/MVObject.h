#ifndef MVObject_h__
#define MVObject_h__

#include "MVCommon.h"

#include "DataStructure/IntrList.h"

namespace MV
{

	enum NavFuncType
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
		NavFuncType func;
		int         meta;
	};

	enum NodeType
	{
		NODE_DIRECT ,
		NODE_PARALLAX ,

		NUM_NODE_TYPE ,
	};

	int const BLOCK_FACE_NUM = 6;
	int const FACE_NAV_LINK_NUM = 4;

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
			link = nullptr;
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
			link->link = nullptr;
			link = nullptr;
			flag = 0;
		}

	};

	class BlockSurface : public SurfaceDef
	{
	public:
		BlockSurface()
		{
			for( int i = 0 ; i < FACE_NAV_LINK_NUM ; ++i )
			{
				NavSurfInfo* info = surfInfo + i;
				info->idxDir = i;
				info->surf   = this;
				nodes[ NODE_DIRECT ][i].surfInfo = info;
				nodes[ NODE_PARALLAX ][i].surfInfo = info;
			}
		}
		Block*      block;
		NavNode     nodes[ NUM_NODE_TYPE ][ FACE_NAV_LINK_NUM ];
	private:
		NavSurfInfo surfInfo[ FACE_NAV_LINK_NUM ];
	};

	struct GameObject
	{
		GameObject()
		{
			group = nullptr;
		}
		ObjectGroup* group;
		LinkHook     groupHook;
	};



	class Block : public GameObject
	{
	public:
		Block()
		{
			for(auto& surface : surfaces)
				surface.block = this;
		}
		int          id;
		int          idMesh;
		Vec3i        pos;
		AxisRoataion rotation;
		ObjectGroup* group;
		BlockSurface surfaces[ BLOCK_FACE_NUM ];
		LinkHook     mapHook;
		uint32       updateCount;
		Block*       cellNext;

		BlockSurface& getLocalFace( Dir dir )
		{
			return surfaces[ dir ];
		}
		BlockSurface& getFace( Dir dir )
		{
			return surfaces[ rotation.toLocal( dir ) ];
		}


		static Dir WorldDir( BlockSurface& surface )
		{
			return surface.block->rotation.toWorld( LocalDir( surface ) );
		}
		static Dir LocalDir( BlockSurface& surface )
		{
			assert( 0 <= (&surface - surface.block->surfaces) && 
				    ( &surface - surface.block->surfaces) < BLOCK_FACE_NUM );
			return Dir( &surface - surface.block->surfaces );
		}

	};

	class WorldCell
	{
	public:
	};


	struct MeshObject : public GameObject
	{
		int      idMesh;
		Vec3f    pos;
		Vec3f    rotation;
	};

	struct Actor : public GameObject
	{
		Actor()
		{
			actBlockId = 0;
			rotation.set( Dir::X , Dir::Z );
			actState = eActMove;
			moveOffset = 0;
		}

		Vec3i    pos;
		AxisRoataion rotation;

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

	using BlockList = TIntrList< Block , MemberHook< GameObject , &GameObject::groupHook > , PointerType >;
	using ActorList = TIntrList< Actor , MemberHook< GameObject , &GameObject::groupHook > , PointerType >;
	using MeshList  = TIntrList< MeshObject , MemberHook< GameObject , &GameObject::groupHook > , PointerType >;
	using GroupList = TIntrList< ObjectGroup , MemberHook< GameObject , &GameObject::groupHook > , PointerType >;

	class ObjectGroup : public GameObject
	{
	public:
		ObjectGroup()
		{
			node = nullptr;
			bBlockListModify = true;
		}
#define DECL_LIST( TYPE , LIST )\
	void add( TYPE& data )\
	{\
		LIST.push_back( &data );\
		data.group = this;\
	}

#define DECL_LIST_WITH_NUM( TYPE , LIST )\
	mutable bool b##TYPE##ListModify;\
	mutable int  cached##TYPE##Num;\
	void add( TYPE& data )\
	{\
		LIST.push_back( &data );\
		data.group = this;\
		b##TYPE##ListModify = true;\
	}\
	void remove(TYPE& data)\
	{\
		data.groupHook.unlink();\
		data.group = nullptr;\
		b##TYPE##ListModify = true;\
	}\
	int get##TYPE##Num() const\
	{\
		if (b##TYPE##ListModify)\
		{\
			b##TYPE##ListModify = false;\
			cached##TYPE##Num = LIST.size();\
		}\
		return cached##TYPE##Num;\
	}

		DECL_LIST_WITH_NUM( Block , blocks )
		DECL_LIST( Actor , actors )
		DECL_LIST( MeshObject , meshs )
		DECL_LIST( ObjectGroup , children )

#undef DECL_LIST

		void remove( GameObject& obj )
		{
			obj.groupHook.unlink();
			obj.group = nullptr;
		}

		template< class T , class TFunc >
		void removeList(T& objectList, TFunc& func)
		{
			while (!objectList.empty())
			{
				auto object = objectList.front();
				remove(*object);
				func(object);
			}
		}
		template< class TFunc >
		void removeAll( TFunc& func )
		{
			removeList(blocks, func);
			removeList(actors, func);
			removeList(meshs, func);
			removeList(children, func);
		}



		int            idx;
		BlockList      blocks;
		ActorList      actors;
		MeshList       meshs;
		GroupList      children;

		IGroupModifier* node;
		LinkHook        spaceHook;
	};


}//namespace MV

#endif // MVObject_h__
