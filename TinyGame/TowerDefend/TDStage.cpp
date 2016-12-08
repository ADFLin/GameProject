#include "TDPCH.h"
#include "TDStage.h"

#include "TDRender.h"

#include "GameGUISystem.h"
#include "WidgetUtility.h"

namespace TowerDefend
{
	bool LevelStage::onInit()
	{ 
		if( !BaseClass::onInit() )
			return false;

		Controller& controller = static_cast< Controller& >(
			getGame()->getController() );

		mLevel = new Level;
		getActionProcessor().setLanucher( mLevel );
		mLevel->setController( &controller );

		mRenderer = new Renderer();
		return true; 
	}

	void LevelStage::setupScene( IPlayerManager& playerManager )
	{
		::Global::GUI().cleanupWidget();

		Controller& controller = static_cast< Controller& >(
			getGame()->getController() );

		CControlUI* controlUI = new CControlUI( controller , ::Global::GUI() );
		mLevel->setControlUI( controlUI );
	}

	CControlUI::CControlUI( Controller& controller , GUISystem& guiSystem )
	{
		Vec2i panelPos = Global::getDrawEngine()->getScreenSize() - ActComPanel::PanelSize;
		mActComPanel = new ActComPanel( UI_ANY , controller , panelPos , NULL );
		mActComPanel->setAlpha( 1.0f );
		guiSystem.addWidget( mActComPanel );
	}

	void CControlUI::setComMap( ComMapID id )
	{
		mActComPanel->setComMap( id );
	}

	void CControlUI::onFireCom( int idx , int comId )
	{
		mActComPanel->glowButton( idx );
	}

}//namespace TowerDefend