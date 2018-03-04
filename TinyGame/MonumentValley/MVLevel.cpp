#include "MVLevel.h"

#include "DataStream.h"

#include <cmath>
#include <unordered_map>

namespace MV
{
	Level::~Level()
	{
		cleanup( true );
	}

	void Level::save(DataSerializer& sr)
	{
		DataSerializer::WriteOp op( sr );

		uint32 const version = 4;
		op & version;

		uint16 ptrId = 0;
		int num;
		num = mWorld.mGroups.size();
		op & num;

		std::unordered_map< void* , uint16 > groupPtrMap;
		groupPtrMap[ &mWorld.mRootGroup ] = ptrId++; 
		for( int i = 0 ; i < mWorld.mGroups.size() ; ++i )
		{
			groupPtrMap[ mWorld.mGroups[i] ] = ptrId++;
			ObjectGroup* group = mWorld.mGroups[i];
			ptrId = groupPtrMap[ group->group ];
			op & ptrId;
		}

		int numBlock = 0;
		for( int i = 1 ; i < mWorld.mBlocks.size() ; ++i )
		{
			if ( mWorld.mBlocks[i]->id > 0 )
				++numBlock;
		}
		op & numBlock;
		for( int i = 1 ; i < mWorld.mBlocks.size() ; ++i )
		{
			Block* block = mWorld.mBlocks[i];
			if ( block->id <= 0 )
				continue;

			op & block->pos;
			op & uint8( block->rotation[0] );
			op & uint8( block->rotation[2] );
			op & block->idMesh;
			ptrId = groupPtrMap[ block->group ];
			op & ptrId;
			for( int i = 0 ; i < 6 ; ++i )
			{
				BlockSurface& surf = block->surfaces[i];
				op & surf.fun;
				op & surf.meta;
			}
		}

		num = mModifiers.size();
		op & num;

		std::unordered_map< void* , uint16 > modifierPtrMap;
		for( int i = 0 ; i < num ; ++i )
		{
			ISpaceModifier* modifier = mModifiers[i];

			modifierPtrMap[ modifier ] = i;
			op & uint8( modifier->getType() );
			switch( modifier->getType() )
			{
			case SNT_ROTATOR:
				{
					IRotator* rotator = static_cast< IRotator* >( modifier );
					op & rotator->mPos;
					op & uint8( rotator->mDir );
				}
				break;
			}

			if ( modifier->isGroup() )
			{
				IGroupModifier* groupMF = static_cast< IGroupModifier* >( modifier );
				int groupSize = groupMF->mGourps.size();
				op & groupSize;
				for( ObjectGroupList::iterator iter = groupMF->mGourps.begin() , itEnd = groupMF->mGourps.end();
					iter != itEnd ; ++iter )
				{
					ptrId = groupPtrMap[ *iter ];
					op & ptrId;
				}
			}
		}


		if ( version >= 4 )
		{
			num = mMeshVec.size();
			op & num;
			for( int i = 0 ; i < mMeshVec.size() ; ++i )
			{
				MeshObject* mesh = mMeshVec[i];
				ptrId = groupPtrMap[ mesh->group ];
				op & ptrId;
				op & mesh->idMesh;
				op & mesh->pos;
				op & mesh->rotation;
			}

			num = mSpaceCtrlors.size();
			op & num;
			for( int i = 0 ; i < mSpaceCtrlors.size() ; ++i )
			{
				SpaceControllor* ctrlor = mSpaceCtrlors[i];

				num = ctrlor->mInfo.size();
				op & num;
				for( int n = 0 ; n < ctrlor->mInfo.size() ; ++n )
				{
					SpaceControllor::Info& info = ctrlor->mInfo[n];
					ptrId = modifierPtrMap[ info.modifier ];
					op & ptrId ;
					op & info.factorScale;
				}
			}
		}
	}

	NavFunType NavFunMapV1[] =
	{
		NFT_NULL ,
		NFT_LADDER  ,
		NFT_ROTATOR_C ,
		NFT_ROTATOR_NC ,
		NFT_ROTATOR_C_B1 ,
		NFT_ROTATOR_C_B2 ,
		NFT_ROTATOR_NC_B1 ,
		NFT_ROTATOR_NC_B2 ,
		NFT_ONE_WAY ,
		NFT_STAIR ,
		NFT_PLANE_LADDER ,
		NFT_PLANE ,
		NFT_BLOCK ,
		NFT_PASS_VIEW ,
	};

	void Level::load( DataSerializer& sr )
	{
		cleanup( false );

		DataSerializer::ReadOp op( sr );

		uint32 version = 0;
		op & version;

		uint16 ptrId = 0;
		int num;
		op & num;
		for( int i = 0 ; i < num ; ++i )
		{
			op & ptrId ;
			mWorld.createGroup( ptrId == 0 ? NULL : mWorld.mGroups[ ptrId - 1 ] );
		}
		op & num ;
		for( int i = 0 ; i < num ; ++i )
		{
			Vec3i pos;
			uint8 dir[2];
			int idMesh;
			SurfaceDef surfaces[6];

			op & pos;
			op & dir[0];
			op & dir[1];
			op & idMesh;
			op & ptrId;
			for( int i = 0 ; i < 6 ; ++i )
			{
				SurfaceDef& surf = surfaces[i];
				op & surf.fun;
				op & surf.meta;
				if ( version <= 1 )
				{
					surf.fun = NavFunMapV1[ surf.fun ];
				}
			}
			ObjectGroup* group = ( ptrId == 0 ) ? ( &mWorld.mRootGroup ) : mWorld.mGroups[ ptrId - 1 ]; 
			mWorld.createBlock( pos , idMesh , surfaces , false , group  , Dir(dir[1]) , Dir(dir[0]) );
		}

		if ( version >= 1 )
		{
			op & num;
			for( int i = 0 ; i < num ; ++i )
			{
				IGroupModifier* node = NULL;
				uint8 type;
				op & type;
				switch( type )
				{
				case SNT_ROTATOR:
					{
						Vec3i pos;
						uint8 dir;
						op & pos;
						op & dir;
						node = createRotator( pos , Dir(dir) );
					}
					break;
				}

				int groupSize;
				op & groupSize;
				for( int i = 0 ; i < groupSize ; ++i )
				{
					op & ptrId;
					ObjectGroup* group = ( ptrId == 0 ) ? ( &mWorld.mRootGroup ) : mWorld.mGroups[ ptrId - 1 ];
					node->addGroup( *group );
				}
			}
		}

		if ( version >= 4 )
		{
			op & num;
			for( int i = 0 ; i < num ; ++i )
			{
				op & ptrId;
				ObjectGroup* group = ( ptrId == 0 ) ? ( &mWorld.mRootGroup ) : mWorld.mGroups[ ptrId - 1 ]; 
				MeshObject* mesh = createEmptyMesh( group );
				op & mesh->idMesh;
				op & mesh->pos;
				op & mesh->rotation;
			}

			op & num;
			for( int i = 0 ; i < num ; ++i )
			{
				SpaceControllor* ctrlor = createControllor();
				int num2;
				op & num2;
				for( int n = 0 ; n < num2 ; ++n )
				{
					SpaceControllor::Info& info = ctrlor->mInfo[n];
					op & ptrId ;
					float scale;
					op & scale;
					ctrlor->addNode( *mModifiers[ptrId] , scale );
				}
			}
		}

		mWorld.updateAllNavNode();
	}

	void Level::cleanup(bool beDestroy)
	{
		for( SpaceCtrlorVec::iterator iter = mSpaceCtrlors.begin(),itEnd = mSpaceCtrlors.end();
			iter != itEnd ; ++iter )
		{
			delete *iter;
		}
		mSpaceCtrlors.clear();

		for( ModifierVec::iterator iter = mModifiers.begin(),itEnd = mModifiers.end();
			iter != itEnd ; ++iter )
		{
			delete *iter;
		}
		mModifiers.clear();
		mWorld.cleanup( beDestroy );
	}

	SpaceControllor* Level::createControllor()
	{
		SpaceControllor* ctrl = new SpaceControllor;
		mSpaceCtrlors.push_back( ctrl );
		return ctrl;
	}

	MeshObject* Level::createMesh(Vec3f const& pos , int idMesh , Vec3f const& rotation , ObjectGroup* group /*= NULL */)
	{
		MeshObject* mesh = createEmptyMesh( group );
		mesh->idMesh = idMesh;
		mesh->pos    = pos;
		mesh->rotation = rotation;

		return mesh;
	}

	MeshObject* Level::createEmptyMesh( ObjectGroup* group )
	{
		MeshObject* mesh = new MeshObject;
		mMeshVec.push_back( mesh );
		if ( group == NULL )
			group = &mWorld.mRootGroup;
		group->add( *mesh );
		return mesh;
	}

	void Level::destroyMeshByIndex(int idx)
	{
		MeshObject* mesh = mMeshVec[ idx ];
		mMeshVec.erase( mMeshVec.begin() + idx );
		mesh->group->remove( *mesh );
	}

	IRotator* Level::createRotator(Vec3i const& pos , Dir dir)
	{
		IRotator* rotator = mCreator->createRotator();
		rotator->mPos = pos;
		rotator->mDir = dir;
		mModifiers.push_back( rotator );
		return rotator;
	}

}//namespace MV

