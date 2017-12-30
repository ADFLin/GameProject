#ifndef TDGUI_h__
#define TDGUI_h__

#include "GameWidget.h"
#include "TDDefine.h"

class Graphics2D;

namespace TowerDefend
{
	class Controller;

	class ActComButton : public GButtonBase
	{
		typedef GButtonBase BaseClass;
	public:
		typedef ComMap::KeyValue KeyValue;
		ActComButton( int id , int idx , Vec2i const& pos , Vec2i const& size , GWidget* parent )
			:BaseClass( id , pos , size , parent )
		{ 
			mIndex    = idx;
			mGlowFrame = 0;
			mComKey    = NULL; 
		}

		int             getIndex(){ return mIndex; }
		void            setComKey( KeyValue const* key ){ mComKey = key; }
		KeyValue const* getComKey(){ return  mComKey;  }

		void            glow();
		void            updateFrame( int frame );

		void            onRender();
		void            onClick();


		int mIndex;
		int mGlowFrame;
		KeyValue const* mComKey;
	};

	class ActComPanel : public GPanel
	{
		enum
		{
			UI_COM_BUTTON = UI_WIDGET_ID ,
		};
		typedef GPanel BaseClass;
	public:
		ActComPanel( int id , Controller& controller , Vec2i const& pos , GWidget* parent );

		void    setComMap( ComMapID id );
		void    glowButton( int index );
		virtual bool  onChildEvent( int event , int id , GWidget* ui );

		static Vec2i ButtonSize;
		static Vec2i PanelSize;

		Controller*    mController;
		ActComButton*  mButton[ COM_MAP_ELEMENT_NUN ];
	};

	class ActorListPanel : public GPanel
	{



	};

}//namespace TowerDefend

#endif // TDGUI_h__
