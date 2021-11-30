#include "TDPCH.h"
#include "TDStage.h"

#include "TDRender.h"

#include "GameGUISystem.h"
#include "Widget/WidgetUtility.h"

namespace TowerDefend
{
	bool LevelStage::onInit()
	{ 
		if( !BaseClass::onInit() )
			return false;

		CInputControl& inputConrol = static_cast< CInputControl& >(
			getGame()->getInputControl() );

		mLevel = new Level;
		getActionProcessor().setLanucher( mLevel );
		mLevel->setInputControl( &inputConrol);

		mRenderer = new Renderer();
		return true; 
	}

	void LevelStage::setupScene( IPlayerManager& playerManager )
	{
		::Global::GUI().cleanupWidget();

		CInputControl& inputConrol = static_cast< CInputControl& >(
			getGame()->getInputControl() );

		CControlUI* controlUI = new CControlUI(inputConrol, ::Global::GUI() );
		mLevel->setControlUI( controlUI );
	}

	CControlUI::CControlUI( CInputControl& inputConrol, GUISystem& guiSystem )
	{
		Vec2i panelPos = ::Global::GetScreenSize() - ActComPanel::PanelSize;
		mActComPanel = new ActComPanel( UI_ANY , inputConrol, panelPos , NULL );
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