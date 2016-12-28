#include "CmtPCH.h"
#include "CmtStage.h"

#include "GameShare.h"

#include "CmtDeviceFactory.h"
#include "CmtDeviceID.h"
#include "CmtLevel.h"
#include "CmtDefine.h"

#include <sstream>
#include "GameData1.h"
#include "GameData2.h"
#include "GameData3.h"
#include "GameData4.h"

#include "GameGUISystem.h"
#include "GameWidgetID.h"
#include "RenderUtility.h"
#include "PropertyKey.h"



namespace Chromatron
{
	int const IdxCreateMode = -1;

	unsigned char const* GameDataPackage[] = 
	{
		GameData1 ,
		GameData2 ,
		GameData3 ,
		GameData4 ,
	};


	LevelStage::LevelStage()
	{
		mIndexLevel = -1;
		mNumLevel   = 0;
		std::fill_n( mLevelStorage , MaxNumLevel , (Level*) 0 );
	}

	bool LevelStage::onInit()
	{

		::Global::GUI().cleanupWidget();

		Vec2i buttonSize( 60 , 25 );

		int screenWidth = Global::getDrawEngine()->getScreenWidth();
		GPanel* panel = new GPanel( UI_ANY , Vec2i(0,0) , Vec2i( screenWidth , 30 ) , NULL );
		panel->setRenderType( GPanel::eRectType );
		::Global::GUI().addWidget( panel );

		Vec2i pos( ( screenWidth - 2 * buttonSize.x - 300 ) / 2 , 2 );

		buttonSize.x = 100;
		mNextLevelButton = new GButton( UI_RESTART_LEVEL , pos , buttonSize , panel );
		mNextLevelButton->setTitle( LAN("Restart" ) );
		pos.x += buttonSize.x;

		buttonSize.x = 60;
		mPrevLevelButton = new GButton( UI_PREV_LEVEL_BUTTON , pos , buttonSize , panel );
		mPrevLevelButton->setTitle( "<=" );
		pos.x += buttonSize.x;

		mLevelChoice = new GChoice( UI_LEVEL_CHOICE , pos , Vec2i( 100 , 25 ) , panel );
		pos.x += 100;

		mNextLevelButton = new GButton( UI_NEXT_LEVEL_BUTTON , pos , buttonSize , panel );
		mNextLevelButton->setTitle( "=>" );
		pos.x += buttonSize.x;

		buttonSize.x = 100;
		mNextLevelButton = new GButton( UI_GO_UNSOLVED_LEVEL_BUTTON , pos , buttonSize , panel );
		mNextLevelButton->setTitle( LAN("Go Unsolved" ) );
		pos.x += buttonSize.x;

		buttonSize.x = 100;
		pos.x =  screenWidth - buttonSize.x - 5;
		GButton* button = new GButton( UI_MAIN_MENU , pos , buttonSize , panel );
		button->setTitle( LAN( "Main Menu" ) );

		pos.x = 5;
		GChoice* choice = new GChoice( UI_GAME_PACKAGE_CHOICE , pos , Vec2i( 100 , 25 ) , panel );
		for( int i = 0; i < ARRAY_SIZE( GameDataPackage ) ; ++i )
		{
			FixString< 32 > str;
			str.format( "Chromatron %d" , i + 1 );
			unsigned id = choice->appendItem( str );
		}
		choice->appendItem( "Create Mode" );

		GPanel* statusPanel = new GPanel( UI_ANY , Vec2i( 10 , 120 ) , Vec2i( 150 , 300 ) , NULL );
		statusPanel->setRenderCallback( RenderCallBack::create( this , &LevelStage::drawStatusPanel ) );
		::Global::GUI().addWidget( statusPanel );

		mScene.setWorldPos( Vec2i( 190 , 80 ) );

		int idxPackage = Global::GameSetting().getIntValue( "LastPlayPackage" , CHROMATRON_NAME , 0 );
		if ( !loadGameData( idxPackage , true ) )
		{
			if ( !loadGameData( 0 , true ) )
				return false;
		}

		if ( mIndexGamePackage != IdxCreateMode )
			choice->setSelection( mIndexGamePackage );
		else
			choice->setSelection( choice->getItemNum() - 1 );

		int level = Global::GameSetting().getIntValue( "LastPlayLevel" , CHROMATRON_NAME , 0 );
		changeLevel( level );


		return true;
	}

	void LevelStage::onEnd()
	{
		Global::GameSetting().setKeyValue( "LastPlayLevel" , CHROMATRON_NAME , mIndexLevel );
		Global::GameSetting().setKeyValue( "LastPlayPackage" , CHROMATRON_NAME , mIndexGamePackage );
		saveGameLevelState();
		cleanupGameData();
	}

	void LevelStage::onRender( float dFrame )
	{
		Graphics2D& g = Global::getGraphics2D();
		mScene.render( g );		
	}

	bool LevelStage::tryUnlockLevel()
	{
		return false;
	}

	void LevelStage::tick()
	{
		mScene.tick();

		State state = mLevelState[ mIndexLevel ];
		switch( state )
		{
		case eHAVE_SOLVED:
		case eUNSOLVED:
			if ( getCurLevel().isGoal() )
			{
				++mNumLevelSolved;
				mLevelState[ mIndexLevel ] = eSOLVED;
				if ( state == eUNSOLVED )
				{
					tryUnlockLevel();
				}
			}
			break;
		case eSOLVED:
			if ( !getCurLevel().isGoal() )
			{
				--mNumLevelSolved;
				mLevelState[ mIndexLevel ] = eHAVE_SOLVED;
			}
			break;
		}
	}

	void LevelStage::updateFrame( int frame )
	{
		mScene.updateFrame( frame );
	}

	bool LevelStage::onMouse( MouseMsg const& msg )
	{
		if ( !BaseClass::onMouse( msg ) )
			return false;

		mScene.procMouseMsg( msg );
		return false;
	}

	bool LevelStage::onKey( unsigned key , bool isDown )
	{
		if ( !isDown )
			return false;

		switch( key )
		{
		case 'R': break;
		}
		return false;
	}

	bool LevelStage::onWidgetEvent( int event , int id , GWidget* ui )
	{
		switch ( id )
		{
		case UI_LEVEL_CHOICE:
			changeLevel( mMinChoiceLevel + GUI::castFast< GChoice* >( ui )->getSelection() );
			return false;
		case UI_NEXT_LEVEL_BUTTON:
			changeLevel( mIndexLevel + 1 );
			return false;
		case UI_PREV_LEVEL_BUTTON:
			changeLevel( mIndexLevel - 1 );
			return false;
		case UI_GAME_PACKAGE_CHOICE:
			if ( mIndexGamePackage != GUI::castFast< GChoice* >( ui )->getSelection() )
			{
				GChoice* choice = GUI::castFast< GChoice* >( ui );
				int index = choice->getSelection();
				saveGameLevelState();

				if ( index == choice->getItemNum() - 1 )
					index = IdxCreateMode;

				if ( !loadGameData( index , true ) )
				{
					loadGameData( 0 , true );
					GUI::castFast< GChoice* >( ui )->setSelection( 0 );
				}

			}
			return false;
		case UI_GO_UNSOLVED_LEVEL_BUTTON:
			{
				for( int i = 1 ; i < mNumLevel - 1 ; ++i )
				{
					int level = mIndexLevel + i;
					if ( level >= mNumLevel )
						level -= mNumLevel;

					if ( !mLevelStorage[level]->isGoal() )
					{
						changeLevel( level );
						break;
					}
				}
			}
			return false;
		case UI_RESTART_LEVEL:
			mScene.restart();
			return false;
		}

		return BaseClass::onWidgetEvent( event , id , ui );
	}

	void LevelStage::cleanupGameData()
	{
		for( int i = 0 ; i < mNumLevel ; ++i )
		{
			assert( mLevelStorage[i] );
			delete mLevelStorage[i];
			mLevelStorage[i] = NULL;
		}
		mNumLevel = 0;
	}

	bool LevelStage::loadGameData( int idxPackage , bool loadState )
	{
		cleanupGameData();

		bool beCreateMode = false;

		if ( idxPackage == IdxCreateMode )
		{
			beCreateMode = true;

			mLevelStorage[ 0 ] = new Level;
			mNumLevel = 1;
		}
		else
		{
			unsigned char const* data = GameDataPackage[ idxPackage ];

			GameInfoHeader* header = (GameInfoHeader*) data;
			std::string  str( (char*) data , header->totalSize );
			assert( header->numLevel <= MaxNumLevel );
			std::istringstream ss( str , std::ios::binary  );

			mNumLevel = Level::loadLevelData( ss , mLevelStorage , MaxNumLevel );

			if ( mNumLevel != header->numLevel )
			{


			}
		}

		if ( mNumLevel == 0 )
			return false;

		mIndexGamePackage = idxPackage;

		if ( loadState )
			loadGameLevelState();

		mNumLevelSolved = 0;
		for( int i = 0 ; i < mNumLevel ; ++i )
		{
			mLevelStorage[i]->updateWorld();
			if ( mLevelStorage[i]->isGoal() )
			{
				++mNumLevelSolved;
				mLevelState[ i ] = eSOLVED;
			}
			else
			{
				mLevelState[ i ] = eUNSOLVED;
			}
		}
		changeLevelInternal( 0 , true , beCreateMode );

		return true;
	}

	bool LevelStage::changeLevelInternal( int level , bool haveChangeData , bool beCreateMode )
	{
		if ( 0 > level || level >= mNumLevel )
			return false;

		if ( !haveChangeData && level == mIndexLevel )
			return false;

		mScene.setupLevel( *mLevelStorage[ level ] , beCreateMode );
		mIndexLevel = level;

		int const showLevelNum = 7;
		int num = 2 * showLevelNum + 1;

		if ( mIndexLevel - showLevelNum < 0 )
		{
			mMinChoiceLevel = 0;	
		}
		else if ( mIndexLevel + showLevelNum >= mNumLevel )
		{
			mMinChoiceLevel = mNumLevel - num;
			if ( mMinChoiceLevel < 0 )
				mMinChoiceLevel = 0;
		}
		else
		{
			mMinChoiceLevel = mIndexLevel - showLevelNum;
		}

		num = std::min( num , mNumLevel - mMinChoiceLevel );

		mLevelChoice->removeAllItem();

		for( int i = 0 ; i < num ; ++i )
		{
			FixString< 32 > str;
			int idx = mMinChoiceLevel + i;
			str.format( "Level %d %s" , idx + 1 , 
				idx != mIndexLevel && ( mLevelState[idx] == eSOLVED ) ? "(O)" : "" );
			mLevelChoice->appendItem( str );
		}
		mLevelChoice->setSelection( level - mMinChoiceLevel );
		return true;
	}

	void LevelStage::onUpdate( long time )
	{
		int frame = time / gDefaultTickTime;
		for( int i = 0 ; i < frame ; ++i )
			tick();

		updateFrame( frame );
	}

	bool LevelStage::saveGameLevelState()
	{
		if ( mIndexGamePackage != IdxCreateMode )
		{
			std::string code;
			for ( int i = 0 ; i < mNumLevel ; ++i )
			{
				char buffer[ 256 ];
				int len = mLevelStorage[i]->generateDCStateCode( buffer , 256 );
				buffer[ len ] = '\0';
				code += buffer;
			}

			FixString< 32 > str;
			str.format( "SaveData%d", mIndexGamePackage );
			Global::GameSetting().setKeyValue( str , CHROMATRON_NAME , code.c_str() );
		}
		return true;
	}

	bool LevelStage::loadGameLevelState()
	{
		if ( mIndexGamePackage != IdxCreateMode )
		{
			FixString< 32 > str;
			str.format( "SaveData%d", mIndexGamePackage );
			char const* code;
			if ( !Global::GameSetting().tryGetStringValue( str ,  CHROMATRON_NAME ,  code ) )
				return false;

			int maxLen = strlen( code );
			for ( int i = 0 ; i < mNumLevel ; ++i )
			{
				int len = mLevelStorage[i]->loadDCStateFromCode( code , maxLen );

				if ( len == 0 )
				{
					::Msg( "Level %d load State Faild" , i );
					return false;
				}
				code += len;
				maxLen -= len;
			}
		}
		return true;
	}

	void LevelStage::drawStatusPanel( GWidget* widget )
	{
		Vec2i pos = widget->getWorldPos() + Vec2i( 0 , 10 );
		Vec2i size = Vec2i( widget->getSize().x , 20 );
		Graphics2D& g = Global::getGraphics2D();

		RenderUtility::setFont( g , FONT_S12 );
		Vec2i offset( 0 , 30 );

		g.setTextColor( 255 , 255 , 0 );
		FixString< 128 > str;
		g.drawText( pos , size , "-Total Level-" );
		pos += offset;
		str.format( "%d" , mNumLevel );
		g.drawText( pos , size , str );
		pos += offset;
		g.drawText( pos , size , "-Solved Level-" );
		pos += offset;
		str.format( "%d" , mNumLevelSolved );
		g.drawText( pos , size , str );
		pos += offset;


		RenderUtility::setFont( g , FONT_S16 );
		g.setTextColor( 255 , 0 , 0 );
		pos = widget->getWorldPos() + Vec2i( 0 , widget->getSize().y - 30 );
		if ( mLevelState[ mIndexLevel ] == eSOLVED )
		{
			g.drawText( pos , size , "You Win!" );
		}
	}

}//namespace Chromatron