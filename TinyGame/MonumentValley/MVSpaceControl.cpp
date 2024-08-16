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
		for (Info& info : mInfo)
		{
			info.modifier->bUsing = true;
			info.modifier->prevModify( world );
		}
	}

	bool SpaceControllor::modify( World& world , float factor )
	{
		for (Info& info : mInfo)
		{
			if ( !info.modifier->canModify( world , info.factorScale * factor ) )
				return false;
		}

		for (Info& info : mInfo)
		{
			info.modifier->modify( world , info.factorScale * factor );
		}

		for(Info& info : mInfo)
		{
			info.modifier->bUsing = false;
			info.modifier->postModify( world );
		}

		return true;
	}

	void SpaceControllor::updateValue( float ctrlFactor )
	{
		for (Info& info : mInfo)
		{
			assert( info.modifier->bUsing );
			info.modifier->updateValue( info.factorScale * ctrlFactor );
		}
	}


	void IGroupModifier::prevModify( World& world )
	{
		for (ObjectGroup* group : mGourps)
		{
			world.removeMap( *group );
			world.removeDynamicNavNode( *group );
		}

		world.increaseUpdateCount();
		
		for(ObjectGroup* group : mGourps)
		{
			for(Block* block : group->blocks)
			{
				world.updateNeighborNavNode( block->pos , block->group );
			}
		}
	}

	void IGroupModifier::postModify( World& world )
	{
		for (ObjectGroup* group : mGourps)
		{
			world.updateMap( *group );
		}

		for (ObjectGroup* group : mGourps)
		{
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

		for (ObjectGroup* group : mGourps)
		{
			rotate_R( *group , iFactor , bModifyChildren );
		}
	}

	void IRotator::rotate_R( ObjectGroup& group , int factor , bool modifyChildren )
	{
		assert( 0 <= factor && factor < 4 );
		if ( modifyChildren )
		{
			for (ObjectGroup* child : group.children)
			{
				rotate_R( *child , factor , true );
			}
		}

		for (Block* block : group.blocks)
		{
			block->pos = RotatePos( mPos , mDir , block->pos , factor );
			block->rotation.rotate( mDir , factor );
		}

		for (Actor* actor : group.actors)
		{
			actor->pos = RotatePos( mPos , mDir , actor->pos , factor );
			actor->renderPos = RotatePos( mPos , mDir , actor->renderPos , factor );
			actor->rotation.rotate( mDir , factor );
		}

		for (MeshObject* mesh : group.meshs)
		{
			mesh->pos = RotatePos( mPos , mDir , mesh->pos , factor );
			Quat q; q.setEulerZYX( mesh->rotation.z , mesh->rotation.y , mesh->rotation.x );
			Quat rq = Quat::Rotate( FDir::OffsetF( mDir ) , Math::DegToRad( 90 * factor )  ) * q;
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

		for (ObjectGroup* group : mGourps)
		{
			translate_R( *group , offset , bModifyChildren );
		}
	}

	void ITranslator::translate_R(ObjectGroup& group , Vec3i const& offset , bool modifyChildren)
	{
		if ( modifyChildren )
		{
			for(ObjectGroup* child: group.children)
			{
				translate_R( *child , offset , true );
			}
		}

		for(Block* block : group.blocks)
		{
			block->pos += offset;
		}

		for(Actor* actor : group.actors)
		{
			actor->pos += offset;
		}

		for(MeshObject* mesh : group.meshs)
		{
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