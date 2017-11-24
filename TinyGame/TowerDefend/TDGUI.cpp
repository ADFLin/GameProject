#include "TDPCH.h"
#include "TDGUI.h"

#include "TDController.h"
#include "TDLevel.h"

#include "RenderUtility.h"

namespace TowerDefend
{
	Vec2i ActComPanel::ButtonSize( 40 , 40 );
	Vec2i ActComPanel::PanelSize( 5 * ButtonSize.x + 10 , 3 * ButtonSize.y + 10 );


	ActComPanel::ActComPanel( int id , Controller& controller , Vec2i const& pos , GWidget* parent ) 
		:BaseClass( id , pos , PanelSize , parent )
	{
		setRenderType( GPanel::eRectType );
		mController = &controller;

		for( int i = 0 ; i < COM_MAP_ELEMENT_NUN ; ++i )
		{
			Vec2i pos( i % 5 * ButtonSize.x + 5 , i / 5 * ButtonSize.y + 5 );
			mButton[i] = new ActComButton( UI_COM_BUTTON , i , pos , ButtonSize , this );
		}
		setComMap( COM_MAP_NULL );
	}

	void ActComPanel::setComMap( ComMapID id )
	{
		ComMap& comMap = Actor::getComMap( id );
		for( int i = 0 ; i < COM_MAP_ELEMENT_NUN ; ++i )
		{
			ComMap* curMap = &comMap;
			while( 1 )
			{
				ComMap::KeyValue& value = curMap->value[i];
				mButton[i]->setComKey( &value );
				if ( value.comID != CID_NULL )
					break;
				if ( curMap->baseID == COM_MAP_NULL )
					break;
				curMap = &Actor::getComMap( curMap->baseID );
			}

			mButton[i]->show( mButton[i]->getComKey()->comID != CID_NULL );
		}
	}

	bool ActComPanel::onChildEvent( int event , int id , GWidget* ui )
	{
		switch( id )
		{
		case UI_COM_BUTTON:
			{
				ActComButton* button = GUI::CastFast< ActComButton >( ui );
				mController->addUICom( button->getIndex() , button->getComKey()->comID );
			}
			break;
		}
		return false;
	}

	void ActComPanel::glowButton( int index )
	{
		mButton[index]->glow();
	}


	void ActComButton::onRender()
	{
		Graphics2D& g = Global::getGraphics2D();
		Vec2i wPos = getWorldPos();

		if ( mGlowFrame > 0 )
			RenderUtility::DrawBlock( g , wPos, getSize() , Color::eGreen );
		else
			RenderUtility::DrawBlock( g , wPos, getSize() , Color::eBlue );
		if ( getComKey() && getComKey()->comID != CID_NULL )
		{
			FixString< 32 > str;
			str.format( "%d(%c)" , getComKey()->comID , getComKey()->key );
			g.drawText( wPos.x , wPos.y , str );
		}
	}

	void ActComButton::updateFrame( int frame )
	{
		mGlowFrame -= frame;
	}

	void ActComButton::glow()
	{
		mGlowFrame = 10;
	}

	void ActComButton::onClick()
	{
		BaseClass::onClick();
	}

}//namespace TowerDefend
