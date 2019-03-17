#include "RichPCH.h"
#include "RichWorldEditor.h"

#include "RichWorld.h"
#include "RichLevel.h"
#include "RichScene.h"

#include "GameGlobal.h"
#include "GameGUISystem.h"

namespace Rich
{


	WorldEditor::WorldEditor()
	{
		mEditType = ET_TILE;
		mMode     = MODE_SELECT;
		mAreaEdit = nullptr;
	}

	bool WorldEditor::onMouse( MouseMsg const& msg )
	{
		if ( msg.onLeftDown() )
		{
			switch( mEditType )
			{
			case ET_TILE:
				{
					MapCoord coord;
					if ( mSelect->calcCoord( msg.getPos() , coord ) )
					{
						if ( mMode == MODE_ADD )
							mWorld->addTile( coord , EMPTY_AREA_ID );
						else if ( mMode == MODE_REMOVE )
							mWorld->removeTile( coord );
						return false;
					}
				}
				break;
			case ET_LAND:
				{
					MapCoord coord;
					if ( mSelect->calcCoord( msg.getPos() , coord ) )
					{
						LandArea* land = new LandArea();
						land->setPos( coord );
					}

				}
			}
		}
		return true;
	}

	void WorldEditor::setup( Level& level , Scene& scene )
	{
		mWorld = &level.getWorld();
		mSelect = &scene;
		Vec2i posFrame = Vec2i( 20 ,::Global::GetDrawEngine().getScreenHeight() - 60 );
		mFrame = new WorldEditFrame( UI_ANY , posFrame , nullptr );
		::Global::GUI().addWidget( mFrame );
	}

	void WorldEditor::stopEdit()
	{
		mFrame->destroy();
	}


	WorldEditFrame::WorldEditFrame( int id , Vec2i const& pos , GWidget* parent ) 
		:GFrame( id , pos , Vec2i( 200 , 50 ) , parent )
	{
		GButton* button;
		button = new GButton( UI_LAND_OBJ , Vec2i( 5 , 5 ) , Vec2i( 80 , 20 ) , this );
		button->setTitle( "Land" );
	}

}//namespace Rich
