#include "TetrisPCH.h"
#include "TetrisSettingHelper.h"

#include "TetrisStage.h"
#include "TetrisLevelManager.h"

#include "DataStreamBuffer.h"
#include "GameSettingPanel.h"
#include "Widget/GameRoomUI.h"

#include "GameWidgetID.h"

namespace Tetris
{

	bool CNetRoomSettingHelper::onWidgetEvent( int event ,int id , GWidget* widget )
	{
		switch( id )
		{
		case UI_MODE_CHOICE:
			{
				int num = LevelMode::getMaxPlayerNumber( mInfo.mode );
				setMaxPlayerNum( num );

				mMaxPlayerNum = num;
				getWidget< GChoice >( UI_PLAYER_NUMBER_CHOICE )->setSelection( mMaxPlayerNum - 1 );
			}
			mSettingPanel->removeChildWithMask( MASK_MODE );
			mSettingPanel->adjustChildLayout();
			switch( GUI::CastFast< GChoice >( widget )->getSelection() )
			{
			case 0:  
				mInfo.mode = MODE_TS_CHALLENGE;
				mInfo.modeNormal.startGravityLevel = 0;
				setupNormalModeUI(); 
				break;
			case 1: 
				mInfo.mode = MODE_TS_BATTLE; 
				break;
			}
			modifyCallback( getSettingPanel() );
			return false;
		case UI_PLAYER_NUMBER_CHOICE:
			{
				int num = GUI::CastFast< GChoice >( widget )->getSelection() + 1;
				setMaxPlayerNum( num );
				mMaxPlayerNum = num;

				modifyCallback( getSettingPanel() );
			}
			return false;
		case UI_GRAVITY_LEVEL_SLIDER:
			mInfo.modeNormal.startGravityLevel = GUI::CastFast< GSlider >( widget )->getValue();
			modifyCallback( getSettingPanel() );
			return false;
		}
		return true;
	}


	void CNetRoomSettingHelper::doSetupSetting( bool beServer )
	{
		mInfo.mode    = MODE_TS_CHALLENGE;
		mMaxPlayerNum = LevelMode::getMaxPlayerNumber( mInfo.mode );
		setupBaseUI();
		mInfo.modeNormal.startGravityLevel = 0;
		setupNormalModeUI();
	}

	void CNetRoomSettingHelper::setupGame( StageManager& manager , StageBase* stage )
	{
		if( !static_cast<LevelStage*>(stage)->setupGame(mInfo) )
		{

		}
	}

	void CNetRoomSettingHelper::doImportSetting( DataStreamBuffer& buffer )
	{
		mSettingPanel->removeChildWithMask( MASK_BASE | MASK_MODE );
		mSettingPanel->adjustChildLayout();

		buffer.take( mInfo.mode );
		buffer.take( mMaxPlayerNum );
		setupBaseUI();

		switch( mInfo.mode )
		{
		case MODE_TS_CHALLENGE:
			buffer.take( mInfo.modeNormal );
			setupNormalModeUI();
			break;
		case MODE_TS_BATTLE:
			break;
		}

	}

	bool CNetRoomSettingHelper::checkSettingSV()
	{
		if( mInfo.mode == MODE_TS_BATTLE )
		{

		}
		return true;
	}

	void CNetRoomSettingHelper::doExportSetting(DataStreamBuffer& buffer)
	{
		buffer.fill( mInfo.mode );
		buffer.fill( mMaxPlayerNum );

		switch( mInfo.mode )
		{
		case MODE_TS_CHALLENGE:  
			buffer.fill( mInfo.modeNormal ); 
			break;
		case MODE_TS_BATTLE: 
			break;
		}
	}

	void CNetRoomSettingHelper::setupBaseUI()
	{
		{
			GChoice* choice = mSettingPanel->addChoice( UI_MODE_CHOICE , LOCTEXT("Game Mode") , MASK_BASE );
			choice->addItem( LOCTEXT("Challenge Mode") );
			choice->addItem( LOCTEXT("Battle Mode") );
			switch( mInfo.mode )
			{
			case MODE_TS_CHALLENGE: choice->setSelection(0); break;
			case MODE_TS_BATTLE: choice->setSelection(1); break;
			}
		}
		{
			GChoice* choice = mSettingPanel->addChoice( UI_PLAYER_NUMBER_CHOICE , LOCTEXT("Player Number") , MASK_BASE );
			setupMaxPlayerNumUI( mMaxPlayerNum );
			choice->setSelection( mMaxPlayerNum - 1 );
			setMaxPlayerNum( mMaxPlayerNum );
		}
	}

	void CNetRoomSettingHelper::setupNormalModeUI()
	{
		GSlider* slider = mSettingPanel->addSlider( UI_GRAVITY_LEVEL_SLIDER , LOCTEXT("Gravity Level") , MASK_MODE );
		slider->setRange( 0 , 500 );
		slider->setValue( mInfo.modeNormal.startGravityLevel );
	}

	void CNetRoomSettingHelper::clearUserUI()
	{
		mSettingPanel->removeChildWithMask( MASK_BASE | MASK_MODE );
	}

	void CNetRoomSettingHelper::setupMaxPlayerNumUI( int num )
	{
		GChoice* choice = getWidget< GChoice >( UI_PLAYER_NUMBER_CHOICE );
		choice->removeAllItem();
		for( int i = 0 ; i < num ; ++i )
		{
			char str[16];
			sprintf_s( str , "%d" , i + 1 );
			choice->addItem( str );
		}
	}

}//namespace Tetris
