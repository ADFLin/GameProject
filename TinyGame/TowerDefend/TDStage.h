#ifndef TDStage_h__
#define TDStage_h__

#include "GameStage.h"

#include "GamePackage.h"
#include "GameControl.h"
#include "GameGUISystem.h"

#include "TDDefine.h"
#include "TDLevel.h"
#include "TDGUI.h"

namespace TowerDefend
{
	class CControlUI : public IControlUI
	{

	public:
		CControlUI( Controller& controller , GUISystem& guiSystem );
		void setComMap( ComMapID id ) override;
		void onFireCom( int idx , int comId ) override;
		ActComPanel* mActComPanel;
	};

	class LevelStage : public GameSubStage
	{
		typedef GameSubStage BaseClass;


		virtual void tick()
		{
			BaseClass::tick();

			switch( getState() )
			{
			case GS_RUN:
				mLevel->tick();
				break;
			case GS_START:
				changeState( GS_RUN );
				break;
			}
		}
		virtual void updateFrame( int frame )
		{
			BaseClass::updateFrame( frame );
			mLevel->update( frame );
			::Global::getGUI().updateFrame( frame , getTickTime() );
		}
		virtual bool getAttribValue( AttribValue& value )
		{ 
			return BaseClass::getAttribValue( value ); 
		}
		virtual bool setupAttrib( AttribValue const& value )
		{
			return BaseClass::setupAttrib( value );
		}

		virtual bool onInit();
		virtual void onEnd()
		{
			delete mLevel;
		}
		virtual void onRestart( uint64 seed , bool beInit )
		{
			mLevel->restart();
		}
		virtual void onRender( float dFrame )
		{

			Level::RenderParam param;
			param.drawMouse = ::Global::getGUI().getManager().getMouseUI() == NULL;
			mLevel->render( *mRenderer , param );

			Controller& controller = static_cast< Controller& >(
				getGame()->getController() );

			controller.renderMouse( *mRenderer );

		}
		virtual void setupScene( IPlayerManager& playerManager );
		virtual void onChangeState( GameState state )
		{

		}

	protected:
		virtual bool onWidgetEvent( int event , int id , GWidget* ui ){ return true; }

		Renderer*     mRenderer;
		Level*  mLevel;
	};

}//namespace TowerDefend

#endif // TDStage_h__
