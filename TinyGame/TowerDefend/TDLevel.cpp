#include "TDPCH.h"
#include "TDLevel.h"

#include <cmath>

namespace TowerDefend
{
	class ITarget
	{
	public:
		virtual Actor* getActor() const = 0;
		virtual Vector2    getPos() const = 0;
	};

	struct SelectVisitor 
	{
		SelectVisitor( Viewport& viewport , TDRect const& rect )
			:rectSelector( viewport , rect )
			,selectType( eNULL )
			,actorID( AID_NONE ){}

		bool visit( Entity* entity )
		{
			Actor* actor = entity->cast< Actor >();
			if ( !actor )
				return true;

			if ( actorID != AID_NONE && actor->getActorID() != actorID )
				return true;

			bool canControl = level->canControl( player , actor );

			if ( !canControl )
			{
				if ( selectType != eNULL )
					return true;
			}

			if ( Unit* unit = actor->cast< Unit >() )
			{
				float radius = unit->getColRadius();
				Vector2 offset( radius , radius );
				Vector2 const& pos = unit->getPos();
				if ( !rectSelector.test( pos - offset , pos + offset ) )
					return true;

				if ( canControl )
				{
					if ( selectType != eUnit )
					{
						level->removeAllSelect();
					}
					selectType = eUnit;
				}
				else
				{
					selectType = eOtherUnit;
				}
				level->selectActor( actor );

			}
			else if ( Building* building = actor->cast< Building >() )
			{
				if ( selectType == eUnit )
					return true;

				Vec2i blgSize = building->getBuildingInfo().blgSize;
				Vector2 min = building->getPos() - Vector2( blgSize / 2 ) * gMapCellLength;
				Vector2 max = min + Vector2( blgSize ) * gMapCellLength;

				if ( !rectSelector.test( min , max ) )
					return true;

				if ( canControl )
				{
					if ( selectType != eBuilding )
					{
						level->removeAllSelect();
					}
					selectType = eBuilding;
				}
				else
				{
					selectType = eOtherBuilding;
				}
				level->selectActor( actor );
			}
			//else
			//{
			//	Vector2 const& pos = actor->getPos();
			//	if ( !rectSelector.test( pos , pos ) )
			//		return true;

			//	level->selectActor( actor );
			//}
			return true;
		}


		bool checkUnitSelect( Unit* unit )
		{

			bool canControl = level->canControl( player , unit );

			if ( !canControl )
			{
				if ( selectType != eNULL )
					return true;
			}

			//float radius = unit->getColRadius();
			//Vector2 const& pos = unit->getPos();
			//if ( !rectSelector.test( pos - offset , pos + offset ) )
			//	return true;
			if ( canControl )
			{
				if ( selectType == eOtherUnit )
				{
					level->removeAllSelect();
				}
				selectType = eUnit;
			}
			else
			{
				selectType = eOtherUnit;
			}
			level->selectActor( unit );
			return true;
		}

		enum SelectType
		{
			eNULL ,
			eUnit ,
			eBuilding ,
			eOtherUnit ,
			eOtherBuilding ,
		};

		ActorId      actorID;
		PlayerInfo*  player;
		Viewport::RectSelector rectSelector;
		SelectType  selectType;
		Level*    level;
	};

	void Level::fireAction( ActionTrigger& trigger )
	{
		if ( !trigger.haveUpdateFrame() )
			return;

		ActionPort port = 0;
		CIViewportMove vpMoveInfo;
		if ( trigger.detect( port, ACT_TD_VIEWPORT_MOVE , & vpMoveInfo ) )
		{
			if ( vpMoveInfo.mode == CIViewportMove::eREL_MOVE )
				mVPCtrl.setCenterViewPos( mVPCtrl.getCenterViewPos() + vpMoveInfo.offset );
			else
				mVPCtrl.setCenterViewPos( vpMoveInfo.offset );
		}

		removeInSelectActor();

		CIMouse mouseInfo;
		CISelectRange srInfo;

		ActorId groupActorID = getCurGroupID();

		CIComID comInfo;
		comInfo.comMapID = mCurComMapID;

		if ( trigger.detect(port, ACT_TD_CHOICE_COM_ID , &comInfo ) )
		{
			mControlUI->onFireCom( comInfo.idxComMap , comInfo.chioceID );

			if ( comInfo.chioceID == CID_CANCEL )
			{
				changeBaseComMap( groupActorID );
			}
			else
			{
				switch( mComID )
				{
				case CID_NULL:
				case CID_MOVE:
				case CID_ATTACK:
				case CID_PATROL:
					if ( comInfo.chioceID > 0 )
					{
						mComActorID = ActorId( comInfo.chioceID );

						Actor* actor = evalGroupActorCom( groupActorID , Vector2(0,0) , 0 );
						if ( actor )
						{

						}
					}
					else
					{
						mComID = ComID( comInfo.chioceID );
						switch( mComID )
						{
						case CID_CHOICE_BLG:
						case CID_CHOICE_BLG_ADV:
							{
								assert( isUnit( groupActorID ) );
								int idx = ( mComID == CID_CHOICE_BLG ) ? 0 : 1;
								changeComMap( Unit::getUnitInfo( groupActorID ).extendComMapID[ idx ] );
							}
							break;
						case CID_MOVE:
						case CID_ATTACK:
						case CID_PATROL:
						case CID_RALLY_POINT:
							setSelectMode( SM_LOCK_ACTOR_POS );
							break;

						default:
							;
						}
					}
					break;
				case CID_CHOICE_BLG:
				case CID_CHOICE_BLG_ADV:
					if ( comInfo.chioceID > 0 )
					{
						mComActorID = ActorId( comInfo.chioceID );
						mComID      = CID_BUILD;
						setSelectMode( SM_LOCK_POS );
						changeComMap( COM_MAP_CANCEL );
					}
					break;
				default:
					break;
				}
			}
		}

		switch( mSelectMode )
		{
		case SM_NORMAL:
			if ( trigger.detect(port, ACT_TD_SELECT_UNIT_GROUP , &mouseInfo ) )
			{
				Vector2 wPos = getCtrlViewport().convertToWorldPos( mouseInfo.pos );

				Actor* actor = getTargetActor( wPos );

				if ( !( mouseInfo.opFlag & CF_OP_APPEND ) )
				{
					removeAllSelect();
				}

				if ( actor )
				{
					TDRect rect;
					rect.start.setValue(0,0);
					rect.end = getCtrlViewport().getScreenSize();
					selectActorInRange( rect , actor->getActorID() );
				}
			}
			else if ( trigger.detect(port, ACT_TD_SELECT_UNIT_RANGE , &srInfo )  )
			{
				if ( !( srInfo.opFlag & CF_OP_APPEND ) )
				{
					removeAllSelect();
				}
				srInfo.rect.sort();
				selectActorInRange( srInfo.rect , AID_NONE );
			}

			{

				if ( trigger.detect(port, ACT_TD_MOUSE_COM , &mouseInfo )  )
				{
					Vector2 wPos = getCtrlViewport().convertToWorldPos( mouseInfo.pos );
					Actor* target = getTargetActor( wPos );

					for( ActorVec::iterator iter = mSelectActors.begin();
						iter != mSelectActors.end() ; ++iter )
					{
						Actor* actor = (*iter);
						if ( actor )
						{
							actor->evalDefaultCommand( target , wPos , mouseInfo.opFlag );
						}
					}
				}
			}
			break;
		case SM_LOCK_POS:
			if ( trigger.detect(port, ACT_TD_TARGET_CHOICE , &mouseInfo )  )
			{
				Vector2 wPos = getCtrlViewport().convertToWorldPos( mouseInfo.pos );
				switch( mComID )
				{
				case CID_BUILD:
					if ( evalGroupActorCom( groupActorID , wPos , mouseInfo.opFlag ) )
					{


					}
					break;
				default:
					break;
				}

				if ( !( mouseInfo.opFlag & CF_OP_APPEND ) )
				{
					changeBaseComMap( groupActorID );
				}
			}
			break;
		case SM_LOCK_ACTOR:

			break;
		case SM_LOCK_ACTOR_POS:
			if ( trigger.detect(port, ACT_TD_TARGET_CHOICE , &mouseInfo )  )
			{
				assert( mComID != CID_NULL );

				Vector2 wPos = getCtrlViewport().convertToWorldPos( mouseInfo.pos );
				Actor* target = getTargetActor( wPos );

				switch( mComID )
				{
				case CID_MOVE:
				case CID_ATTACK:
				case CID_PATROL:
				case CID_RALLY_POINT:
					for( ActorVec::iterator iter = mSelectActors.begin();
						iter != mSelectActors.end() ; ++iter )
					{
						Actor* actor = (*iter);
						if ( actor )
						{
							actor->evalCommand( mComID , target , wPos , mouseInfo.opFlag );
						}
					}
					break;
				default:
					;
				}

				if ( !( mouseInfo.opFlag & CF_OP_APPEND ) )
				{
					changeBaseComMap( groupActorID );
				}
			}
			break;
		}

		if ( mSelectMode != SM_NORMAL )
		{
			if ( trigger.detect(port, ACT_TD_CANCEL_COM_MODE ) )
			{
				changeBaseComMap( groupActorID );
			}
		}
	}

	Actor* Level::evalGroupActorCom( ActorId groupActorID , Vector2 wPos , unsigned flag )
	{
		ActorVecIter iter = getGroupIteraotr( groupActorID );

		Actor* bestActor = NULL;
		size_t   numCom    = 2048;

		while( Actor* curActor = getNextActor( groupActorID , iter ) )
		{
			if ( curActor->getCurActCommand() == NULL )
			{
				if ( curActor->evalActorCommand( mComID , mComActorID , wPos , flag ) )
				{
					return curActor;
				}
			}

			if ( curActor->getActQueueNum() < numCom )
			{
				bestActor = curActor;
				numCom = curActor->getActQueueNum();
			}
		}

		if ( mSelectActors.size() != 1 )
		{
			flag |= CF_OP_APPEND;
		}

		if ( bestActor && bestActor->evalActorCommand( mComID , mComActorID , wPos , flag ) )
		{
			return bestActor;
		}

		return NULL;
	}

	void Level::changeBaseComMap( ActorId aID )
	{
		mComID      = CID_NULL;
		setSelectMode( SM_NORMAL );
		unsigned cmID = getActorBaseComMapID( aID );
		changeComMap( cmID );
	}

	void Level::removeInSelectActor()
	{
		for( ActorVec::iterator iter = mSelectActors.begin();
			iter != mSelectActors.end() ; )
		{
			if ( *iter == NULL )
				iter = mSelectActors.erase( iter );
			else
				++iter;
		}
	}

	void Level::selectActorInRange( TDRect const& rect , ActorId actorID )
	{
		SelectVisitor visitor( getCtrlViewport() , rect );
		visitor.player  = &mPlayerInfo;
		visitor.level   = this;
		visitor.actorID = actorID;

		//Vector2 min = getCtrlViewport().convertToWorldPos( rect.start );
		//Vector2 max = getCtrlViewport().convertToWorldPos( rect.end );

		//getColManager().testCollision( min , max , CollisionCallback( &visitor , &SelectVisitor::checkUnitSelect ) );
		//if ( visitor.selectType == SelectVisitor::eUnit )
		//	return;

		getEntityMgr().visitEntity( visitor );

		changeComMap( getActorBaseComMapID( getCurGroupID() ) );
	}

	void Level::selectActorInRange( TDRect const& rect , EntityFilter const& filter )
	{
		SelectVisitor visitor( getCtrlViewport() , rect );
		visitor.level = this;
		mEntityMgr.visitEntity( visitor , filter );
	}

	Actor* Level::getTargetActor( Vector2 const& wPos )
	{
		ColObject* obj = getCollisionMgr().getObject( wPos );

		Actor* result = ( obj ) ? obj->getOwner().cast< Actor >() : NULL;

		if ( !result )
			result = getMap().getBuilding( wPos );

		//EntityFilter filter;
		//filter.bitType = ET_ACTOR;
		//TDEntity* entity = mEntityMgr.findEntity( wPos , 10 , false , filter );
		return result;
	}

	unsigned Level::getActorBaseComMapID( ActorId aID )
	{
		if ( aID == AID_NONE )
			return COM_MAP_NULL;
		if ( isBuilding( aID ) )
		{
			return Building::getBuildingInfo( aID  ).baseComMapID;
		}
		else if ( isUnit( aID ) )
		{
			return Unit::getUnitInfo( aID ).baseComMapID;
		}
		return COM_MAP_NULL;
	}

	void Level::onPlayerEvent( EntityEvent event , Entity* entity )
	{
		switch( event )
		{
		case EVT_EN_CHANGE_COM:
			{
				Actor* actor = entity->castFast< Actor >();
				actor->getCurActCommand()->describe( mActComMsg );
			}
			break;
		case EVT_EN_PUSH_COM:
			{
				Actor* actor = entity->castFast< Actor >();
				actor->getLastQueueAct()->describe( mActComMsg );
			}
			break;
		}
	}

	void Level::onMonsterEvent( EntityEvent event , Entity* entity )
	{
		switch( event )
		{
		case EVT_EN_KILLED:
			mPlayerInfo.gold += 10;
			break;
		}
	}

	void Level::setSelectMode( TDSelectMode mode )
	{
		if ( mode == mSelectMode )
			return;
		mSelectMode = mode;

		bool beE = !( mSelectMode == SM_LOCK_ACTOR ||
			mSelectMode == SM_LOCK_POS   ||
			mSelectMode == SM_LOCK_ACTOR_POS );

		mController->enableRectSelect( beE );
	}


	void ActComMessage::haveGoalPos( Vector2 const& pos )
	{
		Msg msg;
		msg.pos   = pos;
		msg.frame = 0;
		msgList.push_back( msg );
	}

	void ActComMessage::haveTarget( Entity* entity )
	{
		Msg msg;
		msg.pos   = entity->getPos();
		msg.frame = 0;
		msgList.push_back( msg );
	}

	void ActComMessage::updateFrame( int frame )
	{
		for ( MsgList::iterator iter = msgList.begin();
			iter != msgList.end() ; )
		{
			iter->frame += frame;
			if ( iter->frame > NumFrameShow )
				iter = msgList.erase( iter );
			else
				++iter;
		}
	}

}//namespace TowerDefend

