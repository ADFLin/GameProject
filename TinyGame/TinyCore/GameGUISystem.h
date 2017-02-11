#ifndef GameGUISystem_h__
#define GameGUISystem_h__

#include "GameWidget.h"

class IGUIDelegate
{
public:
	virtual ~IGUIDelegate(){}
	virtual void addGUITask( TaskBase* task , bool bGlobal ) = 0;
	virtual void dispatchWidgetEvent(int event, int id, GWidget* ui) = 0;
};

class  GUISystem
{
public:
	GUISystem();

	GAME_API bool         init(IGUIDelegate&  guiDelegate);

	GAME_API int          getModalID();
	GAME_API GWidget*     showMessageBox( int id , char const* msg , unsigned flag = GMB_YESNO );
	GAME_API void         hideWidgets( bool bHide ){ mHideWidgets = bHide; }
	GAME_API void         update();
	GAME_API void         render();

	GAME_API void         sendMessage( int event , int id , GWidget* ui );
	GAME_API void         addTask( TaskBase* task , bool beGlobal = false );

	GAME_API void         addWidget( GWidget* widget );
	GAME_API void         cleanupWidget(bool bForceCleanup = false );

	UIManager&   getManager(){ return mUIManager; }

	GAME_API void         addHotkey( GWidget* ui , ControlAction key );
	GAME_API void         removeHotkey( GWidget* ui );
	GAME_API void         scanHotkey( GameController& controller );

	GAME_API int          getUpdateFrameNum(){ return mCurUpdateFrame; }

	GAME_API void         updateFrame( int frame , long tickTime );

	GAME_API bool         procMouseMsg(MouseMsg const& msg);
	GAME_API bool         procKeyMsg(unsigned key, bool isDown);
	GAME_API bool         procCharMsg(unsigned code);

	typedef Tween::GroupTweener< float > Tweener;
	Tweener&     getTweener(){ return mTweener; }
	template< class Fun >
	void         addMotion( GWidget* widget , Vec2i const& from , Vec2i const& to , long time , long delay = 0 , Tween::FinishCallback cb = Tween::FinishCallback() )
	{
		mTweener.tween< Fun , WidgetPos >( *widget , from , to , time , delay ).finishCallback( cb );
	}

	GAME_API GWidget* findTopWidget( int id );
	GAME_API static Vec2i  calcScreenCenterPos( Vec2i const& size );

	void  skipMouseEvent(bool bSkip) { mbSkipMouseEvent = bSkip;  }
	void  skipKeyEvent(bool bSkip) { mbSkipKeyEvent = bSkip; }
	void  skipCharEvent(bool bSkip) { mbSkipCharEvent = bSkip; }
private:
	struct HotkeyInfo
	{
		GWidget*      ui;
		ControlAction keyAction;
	};
	typedef std::list< HotkeyInfo > HotkeyList;
	int           mCurUpdateFrame;
	HotkeyList    mHotKeys;
	IGUIDelegate* mGuiDelegate;
	GUI::Manager  mUIManager;
	typedef Tween::GroupTweener< float > MyTweener;
	MyTweener     mTweener;
	bool          mHideWidgets;
	bool          mbSkipKeyEvent;
	bool          mbSkipCharEvent;
	bool          mbSkipMouseEvent;
};

#endif // GameGUISystem_h__
