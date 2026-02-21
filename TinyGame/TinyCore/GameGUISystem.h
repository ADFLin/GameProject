#ifndef GameGUISystem_h__
#define GameGUISystem_h__

#include "GameConfig.h"
#include "GameWidget.h"

class IGraphics2D;

class IGUIDelegate
{
public:
	virtual ~IGUIDelegate(){}
	virtual void addGUITask( TaskBase* task , bool bGlobal ) = 0;
	virtual void dispatchWidgetEvent(int event, int id, GWidget* ui) = 0;
};


class GameUIManager : public WidgetManagerT< GameUIManager, GWidget >
{
public:
	Vec2i transformToWorld(GWidget* widget, Vec2i const& pos)
	{
		auto layer = widget->getLayer();
		if (layer)
		{
			return layer->screenToWorld.transformPosition(pos);
		}
		return pos;
	}
};

class  GUISystem
{
public:
	GUISystem();

	TINY_API bool         initialize(IGUIDelegate&  guiDelegate);
	TINY_API void         finalize();

	TINY_API int          getModalId();
	TINY_API GWidget*     showMessageBox( int id , char const* msg , EMessageButton::Type buttonType = EMessageButton::YesNo );
	TINY_API void         hideWidgets( bool bHide ){ mHideWidgets = bHide; }
	TINY_API void         update();
	TINY_API void         render(IGraphics2D& g);

	TINY_API void         sendMessage( int event , int id , GWidget* ui );
	TINY_API void         addTask( TaskBase* task , bool beGlobal = false );

	TINY_API void         addWidget( GWidget* widget, WidgetLayoutLayer* layer = nullptr );
	TINY_API void         cleanupWidget(bool bForceCleanup = false , bool bPersistentIncluded = false);

	TINY_API void         doModel(GWidget* ui);

	GameUIManager&   getManager(){ return mUIManager; }

	TINY_API void         addHotkey( GWidget* ui , ControlAction key );
	TINY_API void         removeHotkey( GWidget* ui );
	TINY_API void         scanHotkey( InputControl& inputConrol);

	int          getUpdateFrameNum(){ return mCurUpdateFrame; }

	TINY_API void         updateFrame( int frame , long tickTime );

	TINY_API MsgReply     procMouseMsg(MouseMsg const& msg);
	TINY_API MsgReply     procKeyMsg(KeyMsg const& msg);
	TINY_API MsgReply     procCharMsg(unsigned code);

	typedef Tween::GroupTweener< float > Tweener;
	Tweener&     getTweener(){ return mTweener; }
	template< class TFun >
	auto     addMotion( GWidget* widget , Vec2i const& from , Vec2i const& to , long time , long delay = 0 , Tween::FinishCallback cb = Tween::FinishCallback() )
	{
		auto& tween = mTweener.tween< TFun , WidgetPos >( *widget , from , to , time , delay );
		auto pTween = &tween;
		widget->addTween(pTween);
		tween.finishCallback([widget, pTween, cb]()
		{
			widget->removeTween(pTween);
			if (cb) cb();
		});
		return pTween;
	}

	TINY_API GWidget* findTopWidget( int id );
	template< class WidgetClass >
	WidgetClass* findTopWidget()
	{
		for( auto child = mUIManager.createTopWidgetIterator(); child; ++child )
		{
			auto widget = dynamic_cast<WidgetClass*>(&*child);
			if( widget )
				return widget;
		}
		return nullptr;
	}
	TINY_API static Vec2i  calcScreenCenterPos( Vec2i const& size );

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


	GameUIManager  mUIManager;
	typedef Tween::GroupTweener< float > MyTweener;
	MyTweener     mTweener;
	bool          mHideWidgets;
	bool          mbSkipKeyEvent;
	bool          mbSkipCharEvent;
	bool          mbSkipMouseEvent;
};

#endif // GameGUISystem_h__
