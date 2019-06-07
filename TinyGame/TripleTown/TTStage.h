#ifndef TTStage_h__
#define TTStage_h__

#include "GameStage.h"

#include "TTLevel.h"
#include "TTScene.h"

#include "StageRegister.h"

#include "Widget/WidgetUtility.h"

#include "FileSystem.h"

namespace TripleTown
{

	class LevelStage : public StageBase
		             , public PlayerData
	{
		typedef StageBase BaseClass;
	public:
		virtual bool onInit();
		virtual void onEnd();
		virtual void onRestart( bool beInit );
		virtual void onRender( float dFrame );

		virtual bool onMouse( MouseMsg const& msg );

		virtual bool onKey( unsigned key , bool isDown );
		virtual bool onWidgetEvent(int event, int id, GWidget* ui) 
		{
			switch( id )
			{
			case UI_RESTART_GAME:
				mLevel.restart();
				return false;
			default:
				break;
			}
			return BaseClass::onWidgetEvent(event, id, ui); 
		}
		virtual void onUpdate( long time )
		{
			BaseClass::onUpdate( time );

			int frame = time / gDefaultTickTime;
			for( int i = 0 ; i < frame ; ++i )
				tick();
			updateFrame( frame );
		}
		virtual void tick()
		{

		}
		virtual void updateFrame( int frame )
		{
			mScene.updateFrame( frame );
		}

		FileIterator mIterator;

		Scene mScene;
		Level mLevel;

		virtual int getPoints() const override { return mPlayerPoints; }
		virtual int getCoins() const override { return mPlayerCoins; }
		virtual void addPoints(int points) override
		{
			mPlayerPoints += points;
		}
		virtual void addCoins(int coins) override
		{
			mPlayerCoins += coins;
		}

		int mPlayerPoints;
		int mPlayerCoins;

	};


}


REGISTER_STAGE("Triple Town Test", TripleTown::LevelStage, EStageGroup::Dev);

#endif // TTStage_h__
