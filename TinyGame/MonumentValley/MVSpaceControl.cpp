#include "MVSpaceControl.h"

#include "MVWorld.h"

#include <algorithm>

namespace MV
{
	void SpaceControllor::addNode( ISpaceModifier& modifier , float scale )
	{
		Info info;
		info.modifier = &modifier;
		info.factorScale = scale;
		mInfo.push_back( info );
	}

	void SpaceControllor::prevModify( World& world )
	{
		for( InfoVec::iterator iter = mInfo.begin() , itEnd = mInfo.end();
			iter != itEnd ; ++iter )
		{
			Info& info = *iter;
			info.modifier->isUse = true;
			info.modifier->prevModify( world );
		}
	}

	bool SpaceControllor::modify( World& world , float factor )
	{
		for( InfoVec::iterator iter = mInfo.begin() , itEnd = mInfo.end();
			iter != itEnd ; ++iter )
		{
			Info& info = *iter;
			if ( !info.modifier->canModify( world , info.factorScale * factor ) )
				return false;
		}

		for( InfoVec::iterator iter = mInfo.begin() , itEnd = mInfo.end();
			iter != itEnd ; ++iter )
		{
			Info& info = *iter;
			info.modifier->modify( world , info.factorScale * factor );
		}

		for( InfoVec::iterator iter = mInfo.begin() , itEnd = mInfo.end();
			iter != itEnd ; ++iter )
		{
			Info& info = *iter;
			info.modifier->isUse = false;
			info.modifier->postModify( world );
		}

		return true;
	}

	void SpaceControllor::updateValue( float ctrlFactor )
	{
		for( InfoVec::iterator iter = mInfo.begin() , itEnd = mInfo.end();
			iter != itEnd ; ++iter )
		{
			Info& info = *iter;
			assert( info.modifier->isUse );
			info.modifier->updateValue( info.factorScale * ctrlFactor );
		}
	}


	void IGroupModifier::prevModify( World& world )
	{
		for( ObjectGroupList::iterator iter = mGourps.begin() , itEnd = mGourps.end() ;
			iter != itEnd ; ++iter )
		{
			ObjectGroup* group = *iter;
			world.removeMap( *group );
			world.removeDynamicNavNode( *group );
		}

		world.increaseUpdateCount();
		for( ObjectGroupList::iterator iter = mGourps.begin() , itEnd = mGourps.end() ;
			iter != itEnd ; ++iter )
		{
			ObjectGroup* group = *iter;

			for( BlockList::iterator iter2 = group->blocks.begin() , it2End = group->blocks.end();
				iter2 != it2End ; ++iter2 )
			{
				Block* block = *iter2;
				world.updateNeighborNavNode( block->pos , block->group );
			}
		}
	}

	void IGroupModifier::postModify( World& world )
	{
		for( ObjectGroupList::iterator iter = mGourps.begin() , itEnd = mGourps.end() ;
			iter != itEnd ; ++iter )
		{
			ObjectGroup* group = *iter;
			world.updateMap( *group );
		}

		for( ObjectGroupList::iterator iter = mGourps.begin() , itEnd = mGourps.end() ;
			iter != itEnd ; ++iter )
		{
			ObjectGroup* group = *iter;
			world.updateNavNode( *group );
		}
	}

	bool IRotator::canModify( World& world , float factor )
	{
		return true;
	}

	void IRotator::modify( World& world , float factor )
	{
		int iFactor = std::ceil( factor - 0.5f );
		iFactor = ( iFactor % 4 );
		if ( iFactor < 0 )
			iFactor += 4;
		for( ObjectGroupList::iterator iter = mGourps.begin() , itEnd = mGourps.end() ;
			iter != itEnd ; ++iter )
		{
			ObjectGroup* group = *iter;
			rotate_R( *group , iFactor , bModifyChildren );
		}
	}

	void IRotator::rotate_R( ObjectGroup& group , int factor , bool modifyChildren )
	{
		assert( 0 <= factor && factor < 4 );
		if ( modifyChildren )
		{
			for( GroupList::iterator iter = group.children.begin() , itEnd = group.children.end();
				iter != itEnd ; ++iter )
			{
				ObjectGroup* child = *iter;
				rotate_R( *child , factor , true );
			}
		}

		for( BlockList::iterator iter = group.blocks.begin() , itEnd = group.blocks.end();
			iter != itEnd ; ++iter )
		{
			Block* block = *iter;

			block->pos = roatePos( mPos , mDir , block->pos , factor );
			block->rotation.rotate( mDir , factor );

		}

		for( ActorList::iterator iter = group.actors.begin() , itEnd = group.actors.end();
			iter != itEnd ; ++iter )
		{
			Actor* actor = *iter;

			actor->pos = roatePos( mPos , mDir , actor->pos , factor );
			actor->renderPos = roatePos( mPos , mDir , actor->renderPos , factor );
			actor->rotation.rotate( mDir , factor );
		}

		for( MeshList::iterator iter = group.meshs.begin() , itEnd = group.meshs.end();
			iter != itEnd ; ++iter )
		{
			MeshObject* mesh = *iter;
			mesh->pos = roatePos( mPos , mDir , mesh->pos , factor );
			Quat q; q.setEulerZYX( mesh->rotation.z , mesh->rotation.y , mesh->rotation.x );
			Quat rq = Quat::Rotate( FDir::OffsetF( mDir ) , Math::Deg2Rad( 90 * factor )  ) * q;
			mesh->rotation = rq.getEulerZYX();
		}
	}

	bool ITranslator::canModify(World& world , float factor)
	{
		return true;
	}

	void ITranslator::modify(World& world , float factor)
	{
		int temp = std::min( maxFactor , (int)std::ceil( factor - 0.5f ) );

		Vec3i destPos = org + temp * diff;
		Vec3i offset = destPos - prevPos;

		for( ObjectGroupList::iterator iter = mGourps.begin() , itEnd = mGourps.end() ;
			iter != itEnd ; ++iter )
		{
			ObjectGroup* group = *iter;
			translate_R( *group , offset , bModifyChildren );
		}
	}

	void ITranslator::translate_R(ObjectGroup& group , Vec3i const& offset , bool modifyChildren)
	{
		if ( modifyChildren )
		{
			for( GroupList::iterator iter = group.children.begin() , itEnd = group.children.end();
				iter != itEnd ; ++iter )
			{
				ObjectGroup* child = *iter;
				translate_R( *child , offset , true );
			}
		}

		for( BlockList::iterator iter = group.blocks.begin() , itEnd = group.blocks.end();
			iter != itEnd ; ++iter )
		{
			Block* block = *iter;
			block->pos += offset;
		}

		for( ActorList::iterator iter = group.actors.begin() , itEnd = group.actors.end();
			iter != itEnd ; ++iter )
		{
			Actor* actor = *iter;
			actor->pos += offset;
		}

		for( MeshList::iterator iter = group.meshs.begin() , itEnd = group.meshs.end();
			iter != itEnd ; ++iter )
		{
			MeshObject* mesh = *iter;
			mesh->pos += Vec3f( offset );
		}
	}

	void IParallaxRotator::prevModify(World& world)
	{
		world.removeAllParallaxNavNode();
	}

	void IParallaxRotator::postModify(World& world)
	{
		world.updateAllNavNode();
	}

	void IParallaxRotator::modify(World& world , float factor )
	{

	}



}//namespace MV