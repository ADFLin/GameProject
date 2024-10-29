#ifndef TTStage_h__
#define TTStage_h__

#include "GameStage.h"

#include "TTLevel.h"
#include "TTScene.h"

#include "StageRegister.h"
#include "Widget/WidgetUtility.h"
#include "FileSystem.h"
#include "GameRenderSetup.h"

namespace TripleTown
{

	class LevelStage : public StageBase
		             , public PlayerData
		             , public IGameRenderSetup
	{
		typedef StageBase BaseClass;
	public:



		bool onInit() override;
		void onEnd() override;
		virtual void onRestart( bool beInit );
		void onRender( float dFrame ) override;

		//IGameRenderSetup
		ERenderSystem getDefaultRenderSystem() override
		{
			return ERenderSystem::None;
		}

		bool setupRenderResource(ERenderSystem systemName) override;

		void preShutdownRenderSystem(bool bReInit) override;
		//~IGameRenderSetup

		MsgReply onMouse( MouseMsg const& msg ) override;
		MsgReply onKey(KeyMsg const& msg) override;

		bool onWidgetEvent(int event, int id, GWidget* ui) override 
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
		void onUpdate(GameTimeSpan time) override
		{
			BaseClass::onUpdate(time);

			int frame = long(time) / gDefaultTickTime;
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

		FileIterator mFileIterator;

		Scene mScene;
		Level mLevel;

		int getPoints() const override { return mPlayerPoints; }
		int getCoins() const override { return mPlayerCoins; }
		void addPoints(int points) override
		{
			mPlayerPoints += points;
		}
		void addCoins(int coins) override
		{
			mPlayerCoins += coins;
		}

		int mPlayerPoints;
		int mPlayerCoins;

	};


}


REGISTER_STAGE_ENTRY("Triple Town Test", TripleTown::LevelStage, EExecGroup::Dev, "Game");

#endif // TTStage_h__
