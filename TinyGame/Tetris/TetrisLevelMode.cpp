#include "TetrisPCH.h"
#include "TetrisLevelMode.h"

#include "GamePlayer.h"
#include "GameGUISystem.h"
#include "GameWidgetID.h"
#include "GameClient.h"
#include "TetrisRecord.h"

#include "RenderUtility.h"
#include "TetrisGame.h"

namespace Tetris
{
	namespace
	{
		struct GravityInfo
		{
			short level;
			short value;
		};

		GravityInfo gGravityInfo[] =
		{
			{   0 ,   4 },{  30 ,   6 },{  35 ,   8 },{  40 ,  10 },{  50 ,  12 },
			{  60 ,  16 },{  70 ,  32 },{  80 ,  48 },{  90 ,  64 },{ 100 ,  80 },
			{ 120 ,  96 },{ 140 , 112 },{ 160 , 128 },{ 170 , 144 },{ 200 ,   4 },
			{ 220 ,  32 },{ 230 ,  64 },{ 233 ,  96 },{ 236 , 128 },{ 239 , 160 },
			{ 243 , 192 },{ 247 , 224 },{ 251 , 256 },{ 300 , 512 },{ 330 , 768 },
			{ 360 ,1024 },{ 400 ,1280 },{ 420 ,1024 },{ 450 , 768 },{ 500 ,5120 },
			{ 9999999 ,5120 },
		};

		int gPracticeGravityValue[] =
		{
			1 , 4 , 6 , 8 , 10 , 12 , 16 , 24 , 32 , 48 , 64 , 80 , 96,
			112 , 128 , 144 , 160 , 192 , 224 , 256 , 320 , 426 , 512 ,
			768 , 816 , 1024 , 1280 , 2048 , 4096 , 5120 
		};

	}


	void NormalModeData::changeNextLevel( Level* level )
	{
		++gravityLevel;
		updateGravityValue( level );
	}

	void NormalModeData::updateGravityValue( Level* level )
	{
		if ( gravityLevel >= gGravityInfo[ idxCurLvG ].level )
		{
			++idxCurLvG;
		}
		level->setGravityValue( gGravityInfo[ idxCurLvG ].value );
	}
	void NormalModeData::updateScoreCombo( int numLayer )
	{
		if ( numLayer )
		{
			scoreCombo += 2 * numLayer - 2;
		}
		else
		{
			scoreCombo = 1;
		}
	}
	void NormalModeData::calcScore( int numLayer )
	{
		score += (( gravityLevel + numLayer ) / 4 ) * scoreCombo * numLayer;
	}

	void ChallengeModeData::reset( LevelMode* mode , LevelData& lvData , bool beInit )
	{
		gravityLevel = static_cast< ChallengeMode* >( mode )->mInfo.startGravityLevel;
		score = 0;
		scoreCombo = 1;
		idxCurLvG = 0;
		updateGravityValue( lvData.getLevel() );
	}

	void ChallengeMode::init()
	{

	}

	void ChallengeMode::fireAction( LevelData& lvData , ActionTrigger& trigger )
	{
		if ( trigger.haveUpdateFrame() )
		{
			lvData.fireAction( trigger );
		}
	}

	void ChallengeMode::setupScene( unsigned flag )
	{
		

	}


	void ChallengeMode::setupSingleGame( MyController& controller )
	{
		controller.setPortControl( 0 , 0 );
	}

	void ChallengeMode::onLevelEvent( LevelData& lvData , Event const& event )
	{
		ChallengeModeData* modeData = static_cast< ChallengeModeData* >( lvData.getModeData() );

		switch ( event.id )
		{
		case eMARK_PIECE:
			modeData->updateScoreCombo( event.numLayer );
			break;
		case eCHANGE_PIECE:
			modeData->changeNextLevel( lvData.getLevel() );
			break;
		case eREMOVE_LAYER:
			{
				for( int i = 0 ; i < event.numLayer ; ++i )
					modeData->changeNextLevel( lvData.getLevel() );
				modeData->calcScore( event.numLayer );
			}
			break;
		}
	}

	int ChallengeMode::markRecord( RecordManager& manager, GamePlayer* player )
	{
		LevelData* lvData = getWorld()->getLevelData( player->getActionPort() );
		if ( !lvData )
			return -1;

		Record* record = new Record;

		NormalModeData* modeData = static_cast< ChallengeModeData* >( lvData->getModeData() );
		record->level   = modeData->gravityLevel;
		record->score   = modeData->score;
		record->durtion = lvData->getLevel()->getTimeDuration();

		strcpy_s( record->name , "???" );

		return manager.addRecord( record );
	}

	void ChallengeMode::renderStats( GWidget* ui )
	{
		Vec2i posPanel = ui->getWorldPos();

		int x0 = posPanel.x + 10;
		int y0 = posPanel.y + 5;

		Graphics2D& g = Global::GetGraphics2D();

		LevelData* lvData =  reinterpret_cast< LevelData* >( ui->getUserData() );
		Level* level  = lvData->getLevel();
		NormalModeData* modeData = static_cast< NormalModeData* >( lvData->getModeData() );


		FixString< 256 > str;

		int d = 19;
		int x = x0;
		int y = y0 - d;

		RenderUtility::SetFont( g , FONT_S12 );
		g.setTextColor(Color3ub(255, 255 , 0) );

		g.drawText( x , y += d , "Level:"  );
		str.format( "             %3d" , modeData->gravityLevel );
		g.drawText( x , y += d , str );

		g.drawText( x , y += d ,  "Score:" );
		str.format( "           %5d" , modeData->score  );
		g.drawText( x , y += d , str );

		long sec = ( level->getTimeDuration() / 1000 );
		long min = ( sec / 60 ); 
		g.drawText( x , y += d , "Time:" );
		str.format( "    %02d:%02d:%03d" , min , sec % 60 , level->getTimeDuration()  % 1000 );
		g.drawText( x , y += d , str );

		g.drawText( x , y += d , "Piece:"  );
		str.format( "             %3d" , level->getUsePieceNum() );
		g.drawText( x , y += d , str );

		g.drawText( x , y += d , "Layer:" );
		str.format( "             %3d" , level->getTotalRemoveLayerNum() );
		g.drawText( x , y += d , str );
	}


	void BattleModeData::reset( LevelMode* mode , LevelData& lvData , bool beInit )
	{
		if ( beInit )
		{
			numWinRound = 0;
		}
		numAddLayer = 0;
		lvData.getLevel()->setGravityValue( 10 );
	}

	void BattleModeData::checkAddLayer( Level* level )
	{
		if ( numAddLayer )
		{
			unsigned leakBit = 1  << ( Global::RandomNet() % level->getBlockStorage().getSizeX() );
			for( int i = 0 ; i < numAddLayer ; ++i )
			{
				if ( !level->getBlockStorage().addLayer( 0 , leakBit , EColor::Gray ) )
					break;
			}
			level->fixPiecePos( numAddLayer );
			numAddLayer = 0;
		}
	}

	void BattleMode::renderStats( GWidget* ui )
	{
		Vec2i posPanel = ui->getWorldPos();

		int x0 = posPanel.x + 10;
		int y0 = posPanel.y + 5;

		DrawEngine& de = Global::GetDrawEngine();

		LevelData* lvData = reinterpret_cast< LevelData* >( ui->getUserData() );
		Level* level  = lvData->getLevel();
		BattleModeData* modeData = static_cast< BattleModeData* >( lvData->getModeData() );


		int d = 19;
		int x = x0;
		int y = y0 - d;
	}

	void BattleMode::init( )
	{

	}

	void BattleMode::setupSingleGame( MyController& controller )
	{
		for( int i = 0 ; i < 2 ; ++i )
		{
			controller.setPortControl( i , i );
		}
	}

	bool BattleMode::checkOver()
	{
		int num = getWorld()->getLevelNum() - getWorld()->getLevelOverNum();

		if ( num > 1 )
			return false;

		if ( num == 1 )
		{

		}
		else
		{


		}

		return true;
	}

	void BattleMode::fireAction( LevelData& lvData , ActionTrigger& trigger )
	{
		if ( trigger.haveUpdateFrame() )
		{
			lvData.fireAction( trigger );
		}
	}

	void BattleMode::onLevelEvent( LevelData& lvData , Event const& event )
	{
		switch( event.id )
		{
		case eREMOVE_LAYER:
			{
				LevelData* other = getWorld()->getLevelData( ( lvData.getId() + 1 ) % 2 );
				BattleModeData* otherData = static_cast< BattleModeData* >( other->getModeData() );
				otherData->numAddLayer += event.numLayer;
				otherData->checkAddLayer( other->getLevel());
			}
			break;
		}

	}

	void BattleMode::setupScene(  unsigned flag )
	{
		

	}


	void PracticeModeData::reset( LevelMode* mode , LevelData& data , bool beInit )
	{
		data.getLevel()->setGravityValue( static_cast< PracticeMode* >( mode )->mGravityValue );
	}

	void PracticeMode::init()
	{
		mGravityValue     = 512;
		mMaxClearLayerNum = 0;
	}

	void PracticeMode::fireAction( LevelData& lvData , ActionTrigger& trigger )
	{
		if ( trigger.haveUpdateFrame() )
		{
			lvData.fireAction( trigger );
		}
	}

	void PracticeMode::setupSingleGame( MyController& controller )
	{
		controller.setPortControl( 0 , 0 );
	}

	void PracticeMode::setupScene( unsigned flag )
	{
		
		LevelData* lvData = getWorld()->findPlayerData( 0 );

		Vec2i const uiSize( 140 , 20 );
		int offset = uiSize.y + 8;


		GPanel* panel = new GPanel( UI_PANEL , lvData->getScene()->getSurfacePos() + Vec2i( 250 , 10 ) , Vec2i( uiSize.x + 20 , 350 ) , NULL );
		panel->setRenderCallback( RenderCallBack::Create( this , &PracticeMode::renderControl ) );
		panel->setUserData( intptr_t(lvData) );
		::Global::GUI().addWidget( panel );

		Level* level = lvData->getLevel();
		Vec2i off( 0 , 40 );
		Vec2i pos = Vec2i( 10 , 10 + 30 );

		GSlider* slider;
		slider = new GSlider( UI_GRAVITY_VALUE_SLIDER , pos , uiSize.x , true , panel );
		slider->setRange( 0 , ARRAY_SIZE( gPracticeGravityValue ) - 1 );
		slider->setValue( 20 );

		pos +=  off;
		slider = new GSlider( UI_LOCK_PIECE_SLIDER  , pos , uiSize.x , true , panel );
		slider->setRange( 0 ,  LevelRule::DefaultTimeLockPiece );
		slider->setValue( level->getLockPieceTime() );

		pos +=  off;
		slider = new GSlider( UI_ENTRY_DELAY_SLIDER  , pos , uiSize.x , true , panel );
		slider->setRange( 0 ,  LevelRule::DefaultTimeEntryDelay );
		slider->setValue( level->getEntryDelayTime() );

		pos +=  off;
		slider = new GSlider( UI_CLEAR_LAYER_SLIDER  , pos , uiSize.x , true , panel );
		slider->setRange( 0 ,  LevelRule::DefaultTimeClearLayer );
		slider->setValue( level->getClearLayerTime() );

		pos +=  off;
		slider = new GSlider( UI_MAP_SIZE_X_SLIDER , pos , uiSize.x , true , panel );
		slider->setRange( 5 , 20 );
		slider->setValue( level->getBlockStorage().getSizeX() );

		pos +=  off;
		slider = new GSlider( UI_MAP_SIZE_Y_SLIDER  , pos , uiSize.x , true , panel );
		slider->setRange( 5 , 30 );
		slider->setValue( level->getBlockStorage().getSizeY() );
	}

	bool PracticeMode::onWidgetEvent( int event , int id , GWidget* ui )
	{
		LevelData* lvData = getWorld()->getLevelData( 0 );
		switch( id )
		{
		case UI_GRAVITY_VALUE_SLIDER:
			mGravityValue = gPracticeGravityValue[ GUI::CastFast< GSlider >( ui )->getValue() ];
			lvData->getLevel()->setGravityValue( mGravityValue );
			mMaxClearLayerNum = 0;
			return false;
		case UI_ENTRY_DELAY_SLIDER:
			lvData->getLevel()->setEntryDelayTime( GUI::CastFast< GSlider >( ui )->getValue() );
			mMaxClearLayerNum = 0;
			return false;
		case UI_CLEAR_LAYER_SLIDER:
			lvData->getLevel()->setClearLayerTime( GUI::CastFast< GSlider >( ui )->getValue() );
			mMaxClearLayerNum = 0;
			return false;
		case UI_LOCK_PIECE_SLIDER:
			lvData->getLevel()->setLockPieceTime( GUI::CastFast< GSlider >( ui )->getValue() );
			mMaxClearLayerNum = 0;
			return false;
		case UI_MAP_SIZE_X_SLIDER:
			{
				int sizeX = GUI::CastFast< GSlider >( ui )->getValue();
				int sizeY = lvData->getLevel()->getBlockStorage().getSizeY();
				lvData->getLevel()->resetMap( sizeX , sizeY );
				mMaxClearLayerNum = 0;
			}
			return false;
		case UI_MAP_SIZE_Y_SLIDER:
			{
				int sizeX = lvData->getLevel()->getBlockStorage().getSizeX();
				int sizeY = GUI::CastFast< GSlider >( ui )->getValue();
				lvData->getLevel()->resetMap( sizeX , sizeY );
				mMaxClearLayerNum = 0;
			}
			return false;
		}
		return true;
	}

	void PracticeMode::renderControl( GWidget* ui )
	{
		Vec2i posPanel = ui->getWorldPos();

		int x0 = posPanel.x + 10;
		int y0 = posPanel.y + 20;

		int d = 40;
		int x = x0;
		int y = y0 - d;

		Graphics2D& g = Global::GetGraphics2D();

		Level* level = getWorld()->getLevelData(0)->getLevel();

		FixString< 256 > str;
		RenderUtility::SetFont( g , FONT_S10 );
		g.setTextColor(Color3ub(255 , 255 , 0) );

		str.format( "Gravity Value = %3d" , mGravityValue );
		g.drawText( x , y += d , str );

		str.format( "Lock Piece Time = %3d" , level->getLockPieceTime() );
		g.drawText( x , y += d , str );

		str.format( "Entry Delay Time = %3d" , level->getEntryDelayTime() );
		g.drawText( x , y += d , str );

		str.format( "Clear Layer Time = %3d" , level->getClearLayerTime() );
		g.drawText( x , y += d , str );

		str.format( "Map Size X = %3d" , level->getBlockStorage().getSizeX() );
		g.drawText( x , y += d , str );

		str.format( "Map Size Y = %3d" , level->getBlockStorage().getSizeY() );
		g.drawText( x , y += d , str );


	}

	void PracticeMode::renderStats( GWidget* ui )
	{

		Vec2i posPanel = ui->getWorldPos();

		int x0 = posPanel.x + 10;
		int y0 = posPanel.y + 5;

		Graphics2D& g = Global::GetGraphics2D();

		LevelData* lvData =  reinterpret_cast< LevelData* >( ui->getUserData() );
		Level* level  = lvData->getLevel();
		PracticeModeData* modeData = static_cast< PracticeModeData* >( lvData->getModeData() );

		FixString< 256 > str;

		int d = 19;
		int x = x0;
		int y = y0 - d;

		RenderUtility::SetFont( g , FONT_S12 );
		g.setTextColor(Color3ub(255 , 255 , 0));

		g.drawText( x , y += d , "Piece:" );
		str.format( "             %3d" , level->getUsePieceNum() );
		g.drawText( x , y += d , str );

		long ms  =  level->getTimeDuration() % 1000;
		long sec =  level->getTimeDuration() / 1000;
		long min = ( sec / 60 ); 

		g.drawText( x , y += d , "Time:" );
		str.format( "    %02d:%02d:%03d" , min , sec % 60 , ms );
		g.drawText( x , y += d , str );

		g.drawText( x , y += d , "Layer:" );
		str.format( "             %3d" , level->getTotalRemoveLayerNum() );
		g.drawText( x , y += d , str );

		g.drawText( x , y += d , "Max Layer:" );
		str.format( "             %3d" , mMaxClearLayerNum );
		g.drawText( x , y += d , str );
	}

	void PracticeMode::doRestart( bool beInit )
	{
		if ( beInit )
		{
			mMaxClearLayerNum = 0;
		}
	}

	void PracticeMode::onLevelEvent( LevelData& lvData , Event const& event )
	{
		switch( event.id )
		{
		case eLEVEL_OVER:
			if ( mMaxClearLayerNum < lvData.getLevel()->getTotalRemoveLayerNum() )
				mMaxClearLayerNum = lvData.getLevel()->getTotalRemoveLayerNum();
			break;
		}

	}

}//namespace Tetris
