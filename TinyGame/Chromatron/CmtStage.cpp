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
#include "PropertySet.h"



namespace Chromatron
{
	int const IdxCreateMode = -1;

	unsigned char const* GameDataPackages[] = 
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

		LevelData data;
		data.level = nullptr;
		data.state = eLOCK;
		std::fill_n( mLevels , MaxNumLevel , data );
	}

	bool LevelStage::onInit()
	{

		::Global::GUI().cleanupWidget();

		Vec2i buttonSize( 60 , 25 );

		int screenWidth = Global::GetScreenSize().x;
		GPanel* panel = new GPanel( UI_ANY , Vec2i(0,0) , Vec2i( screenWidth , 30 ) , nullptr );
		panel->setRenderType( GPanel::eRectType );
		::Global::GUI().addWidget( panel );

		Vec2i pos( ( screenWidth - 2 * buttonSize.x - 300 ) / 2 , 2 );

		buttonSize.x = 100;
		mNextLevelButton = new GButton( UI_RESTART_LEVEL , pos , buttonSize , panel );
		mNextLevelButton->setTitle( LOCTEXT("Restart" ) );
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
		mNextLevelButton->setTitle( LOCTEXT("Go Unsolved" ) );
		pos.x += buttonSize.x;

		buttonSize.x = 100;
		pos.x =  screenWidth - buttonSize.x - 5;
		GButton* button = new GButton( UI_MAIN_MENU , pos , buttonSize , panel );
		button->setTitle( LOCTEXT( "Main Menu" ) );

		pos.x = 5;
		GChoice* choice = new GChoice( UI_GAME_PACKAGE_CHOICE , pos , Vec2i( 100 , 25 ) , panel );
		for( int i = 0; i < ARRAY_SIZE( GameDataPackages ) ; ++i )
		{
			InlineString< 32 > str;
			str.format( "Chromatron %d" , i + 1 );
			unsigned id = choice->addItem( str );
		}
		choice->addItem( "Create Mode" );

		GPanel* statusPanel = new GPanel( UI_ANY , Vec2i( 10 , 120 ) , Vec2i( 150 , 300 ) , nullptr );
		statusPanel->setRenderCallback( RenderCallBack::Create( this , &LevelStage::drawStatusPanel ) );
		::Global::GUI().addWidget( statusPanel );

		mScene.setWorldPos( Vec2i( 190 , 80 ) );

		int idxPackage = Global::GameConfig().getIntValue( "LastPlayPackage" , CHROMATRON_NAME , 0 );
		if ( !loadGameData( idxPackage , true ) )
		{
			if ( !loadGameData( 0 , true ) )
				return false;
		}

		if ( mIndexGamePackage != IdxCreateMode )
			choice->setSelection( mIndexGamePackage );
		else
			choice->setSelection( choice->getItemNum() - 1 );

		int level = Global::GameConfig().getIntValue( "LastPlayLevel" , CHROMATRON_NAME , 0 );
		changeLevel( level );


		return true;
	}

	void LevelStage::onEnd()
	{
		Global::GameConfig().setKeyValue( "LastPlayLevel" , CHROMATRON_NAME , mIndexLevel );
		Global::GameConfig().setKeyValue( "LastPlayPackage" , CHROMATRON_NAME , mIndexGamePackage );
		saveGameLevelState();
		cleanupGameData();
	}

	void LevelStage::onRender( float dFrame )
	{
		Graphics2D& g = Global::GetGraphics2D();
		mScene.render( g );		
	}

	bool LevelStage::tryUnlockLevel()
	{
		return false;
	}

	void LevelStage::tick()
	{
		mScene.tick();

		State& state = mLevels[ mIndexLevel ].state;
		switch( state )
		{
		case eHAVE_SOLVED:
		case eUNSOLVED:
			if ( getCurLevel().isGoal() )
			{
				++mNumLevelSolved;
				state = eSOLVED;
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
				state = eHAVE_SOLVED;
			}
			break;
		}
	}

	void LevelStage::updateFrame( int frame )
	{
		mScene.updateFrame( frame );
	}

	MsgReply LevelStage::onMouse( MouseMsg const& msg )
	{
		mScene.procMouseMsg( msg );
		return BaseClass::onMouse(msg);
	}

	MsgReply LevelStage::onKey(KeyMsg const& msg)
	{
		if (msg.isDown())
		{
			switch (msg.getCode())
			{
			case EKeyCode::R: break;
			}
		}
		return BaseClass::onKey(msg);
	}

	bool LevelStage::onWidgetEvent( int event , int id , GWidget* ui )
	{
		switch ( id )
		{
		case UI_LEVEL_CHOICE:
			changeLevel( mMinChoiceLevel + GUI::CastFast< GChoice >( ui )->getSelection() );
			return false;
		case UI_NEXT_LEVEL_BUTTON:
			changeLevel( mIndexLevel + 1 );
			return false;
		case UI_PREV_LEVEL_BUTTON:
			changeLevel( mIndexLevel - 1 );
			return false;
		case UI_GAME_PACKAGE_CHOICE:
			if ( mIndexGamePackage != GUI::CastFast< GChoice >( ui )->getSelection() )
			{
				GChoice* choice = GUI::CastFast< GChoice >( ui );
				int index = choice->getSelection();
				saveGameLevelState();

				if ( index == choice->getItemNum() - 1 )
					index = IdxCreateMode;

				if ( !loadGameData( index , true ) )
				{
					loadGameData( 0 , true );
					GUI::CastFast< GChoice >( ui )->setSelection( 0 );
				}

			}
			return false;
		case UI_GO_UNSOLVED_LEVEL_BUTTON:
			{
				for( int i = 1 ; i < mNumLevel - 1 ; ++i )
				{
					int idxLevel = mIndexLevel + i;
					if ( idxLevel >= mNumLevel )
						idxLevel -= mNumLevel;

					if ( !mLevels[idxLevel].level->isGoal() )
					{
						changeLevel( idxLevel );
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
			delete mLevels[i].level;
			mLevels[i].level = nullptr;
		}
		mNumLevel = 0;
	}

	bool LevelStage::loadGameData( int idxPackage , bool loadState )
	{
		cleanupGameData();

		bool bCreationMode = false;

		if ( idxPackage == IdxCreateMode )
		{
			bCreationMode = true;

			mLevels[ 0 ].level = new Level;
			mNumLevel = 1;
		}
		else
		{
			unsigned char const* data = GameDataPackages[ idxPackage ];

			GameInfoHeader* header = (GameInfoHeader*) data;
			std::string  str( (char*) data , header->totalSize );
			assert( header->numLevel <= MaxNumLevel );
			std::istringstream ss( str , std::ios::binary  );

		
			Level* loadedLevels[MaxNumLevel];
			mNumLevel = Level::LoadData( ss , loadedLevels, MaxNumLevel );
			for( int i = 0; i < mNumLevel; ++i )
			{
				mLevels[i].level = loadedLevels[i];
			}

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
			Level* level = mLevels[i].level;
			level->updateWorld();
			if ( level->isGoal() )
			{
				++mNumLevelSolved;
				mLevels[ i ].state = eSOLVED;
			}
			else
			{
				mLevels[i].state = eUNSOLVED;
			}
		}
		changeLevelInternal( 0 , true , bCreationMode );

		return true;
	}

	bool LevelStage::changeLevelInternal( int indexLevel , bool haveChangeData , bool bCreationMode )
	{
		if ( 0 > indexLevel || indexLevel >= mNumLevel )
			return false;

		if ( !haveChangeData && indexLevel == mIndexLevel )
			return false;

		mScene.setupLevel( *mLevels[ indexLevel ].level , bCreationMode );
		mIndexLevel = indexLevel;

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
			InlineString< 32 > str;
			int idx = mMinChoiceLevel + i;
			str.format( "Level %d %s" , idx + 1 , 
				idx != mIndexLevel && ( mLevels[idx].state == eSOLVED ) ? "(O)" : "" );
			mLevelChoice->addItem( str );
		}
		mLevelChoice->setSelection( indexLevel - mMinChoiceLevel );
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
				int len = mLevels[i].level->generateDCStateCode( buffer , 256 );
				buffer[ len ] = '\0';
				code += buffer;
			}

			InlineString< 32 > str;
			str.format( "SaveData%d", mIndexGamePackage );
			Global::GameConfig().setKeyValue( str , CHROMATRON_NAME , code.c_str() );
		}
		return true;
	}

	bool LevelStage::loadGameLevelState()
	{
		if ( mIndexGamePackage != IdxCreateMode )
		{
			InlineString< 32 > str;
			str.format( "SaveData%d", mIndexGamePackage );
			char const* code;
			if ( !Global::GameConfig().tryGetStringValue( str ,  CHROMATRON_NAME ,  code ) )
				return false;

			int maxLen = FCString::Strlen( code );
			for ( int i = 0 ; i < mNumLevel ; ++i )
			{
				int len = mLevels[i].level->loadDCStateFromCode(code, maxLen);

				if ( len == 0 )
				{
					LogMsg( "Level %d load State Faild" , i );
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
		Graphics2D& g = Global::GetGraphics2D();

		RenderUtility::SetFont( g , FONT_S12 );
		Vec2i offset( 0 , 30 );

		g.setTextColor(Color3ub(255 , 255 , 0) );
		InlineString< 128 > str;
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


		RenderUtility::SetFont( g , FONT_S16 );
		g.setTextColor(Color3ub(255 , 0 , 0) );
		pos = widget->getWorldPos() + Vec2i( 0 , widget->getSize().y - 30 );
		if ( mLevels[ mIndexLevel ].state == eSOLVED )
		{
			g.drawText( pos , size , "You Win!" );
		}
	}

}//namespace Chromatron