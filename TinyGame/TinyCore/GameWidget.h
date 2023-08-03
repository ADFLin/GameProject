#ifndef GameWidget_h__
#define GameWidget_h__

#include "WidgetCore.h"
#include "WidgetCommon.h"

#include "GameConfig.h"
#include "GameWidgetID.h"
#include "GameGraphics2D.h"

#include "Singleton.h"
#include "TaskBase.h"
#include "GameControl.h"
#include "RenderUtility.h"
#include "Tween.h"


#include <functional>



class DrawEngine;
class StageManager;
class InputControl;
struct UserProfile;

typedef std::string String;

enum
{
	EVT_BUTTON_CLICK    ,
	EVT_TEXTCTRL_VALUE_CHANGED   ,
	EVT_TEXTCTRL_COMMITTED  ,
	EVT_CHOICE_SELECT   ,
	EVT_LISTCTRL_DCLICK ,
	EVT_LISTCTRL_SELECT ,
	EVT_SLIDER_CHANGE ,
	EVT_CHECK_BOX_CHANGE ,
	EVT_DIALOG_CLOSE,

	EVT_ENTER_UI ,
	EVT_EXIT_UI  ,

	EVT_BOX_YES ,
	EVT_BOX_NO  ,
	EVT_BOX_OK  ,

	EVT_FRAME_FOCUS ,

	EVT_EXTERN_EVENT_ID ,
};


class RenderCallBack ;
class UIMotionTask;
class WidgetRenderer;
class GWidget;

using WidgetEventDelegate = std::function< bool (int event, GWidget*) >;

struct WidgetColor
{
	WidgetColor()
	{
		bUseName = false;
		value = Color3ub(0, 0, 0);
	}

	void setName(int colorName) { name = colorName; bUseName = true; }
	void setValue(Color3ub const& colorValue){  value = colorValue;  bUseName = false;  }
	Color3ub getValue(int type = COLOR_NORMAL) const
	{
		if ( bUseName )
			RenderUtility::GetColor(name, type);
		return value;
	}

	void setupBrush(IGraphics2D& g, int type = COLOR_NORMAL) const
	{
		if( bUseName )
		{
			RenderUtility::SetBrush(g, name, type);
		}
		else
		{
			g.setBrush(value);
		}
	}
	void setupPen(IGraphics2D& g, int type = COLOR_NORMAL) const
	{
		if( bUseName )
		{
			RenderUtility::SetPen(g, name, type);
		}
		else
		{
			g.setPen(value);
		}
	}
	bool      bUseName;
	Color3ub  value;
	int       name;
};

class  GWidget : public WidgetCoreT< GWidget >
{
public:
	enum
	{
		NEXT_UI_ID = UI_WIDGET_ID ,
	};
	TINY_API GWidget( Vec2i const& pos , Vec2i const& size , GWidget* parent );
	TINY_API ~GWidget();
	int  getID(){ return mID; }


	virtual bool onChildEvent( int event , int id , GWidget* ui ){  return true; }

	WidgetEventDelegate onEvent;
	using WidgetRefreshDelegate = std::function< void(GWidget*) >;
	WidgetRefreshDelegate onRefresh;
	TINY_API void  refresh();

	TINY_API void  setRenderCallback( RenderCallBack* cb );

	intptr_t getUserData(){ return userData; }
	void     setUserData( intptr_t data ) { userData = data; }
	void     setFontType( int fontType ){ mFontType = fontType; }
	TINY_API void  setHotkey( ControlAction key );

	virtual void onMouse( bool beIn ){  /*sendEvent( ( beIn ) ? EVT_ENTER_UI : EVT_EXIT_UI );*/  }
	virtual void onRender(){}
	virtual void onUpdateUI(){}
	virtual MsgReply onKeyMsg(KeyMsg const& msg){ return MsgReply::Unhandled(); }
	virtual void onHotkey( unsigned key ){}
	virtual void onFocus( bool beF ){}
	virtual void onChangePos(Vec2i const& newPos, bool bParentMove) {}
	virtual void onDestroy(){}

	virtual void  updateFrame( int frame ){}



	void onPrevRender(){}
	void onPostRenderChildren(){}
	TINY_API void onPostRender();
	TINY_API bool doClipTest();
	static TINY_API WidgetRenderer& getRenderer();

	void setColor(Color3ub const& color) { mColor.setValue(color); }
	void setColorName(int color) { mColor.setName(color); }

	TINY_API GWidget*  findChild( int id , GWidget* start = NULL );
	template< class T >
	T* findChildT(int id, GWidget* start = nullptr)
	{
		return GUI::CastFast<T>(findChild(id, start));
	}

	void doModal(){  getManager()->startModal( this );  }

	template< class T >
	T* cast()
	{ 
		return GUI::CastFast<T>( this );
	}

protected:
	virtual bool preSendEvent(int event) { return true; }

	TINY_API void sendEvent( int eventID );
	TINY_API void removeMotionTask();


	bool      useHotKey;
	intptr_t  userData;


	WidgetColor mColor;

	RenderCallBack* callback;
	friend  class UIMotionTask;
	TaskBase* motionTask;
	int       mFontType;
	int       mID;
};

class WidgetPos
{
public:
	using DataType = GWidget;
	using ValueType = Vector2;
	void operator()( DataType& data , ValueType const& value ){ data.setPos( value ); }
};

class WidgetSize
{
public:
	using DataType = GWidget;
	using ValueType = Vector2;
	void operator()( DataType& data , ValueType const& value ){ data.setSize( value ); }
};

using GUI = TWidgetLibrary< GWidget >;
using UIManager = GUI::Manager;


class RenderCallBack 
{
public:
	virtual void postRender( GWidget* ui ) = 0;
	virtual void release(){}

	template < class T , class TMemFunc >
	static RenderCallBack* Create( T* ptr , TMemFunc func );

	template< class TFun >
	static RenderCallBack* Create(TFun func);

};

namespace EMessageButton
{
	enum Type
	{
		Ok,
		YesNo,
		None,
	};
}


class UIMotionTask;

namespace Private{

	template < class T , class TMemFunc >
	class TMemberRenderCallBack : public RenderCallBack
	{
		TMemFunc mFunc;
		T*       mPtr;
	public:
		TMemberRenderCallBack( T* ptr , TMemFunc func )
			:mPtr(ptr), mFunc( func ){}
		virtual void postRender( GWidget* ui ){  (mPtr->*mFunc)( ui );  }
		virtual void release(){ delete this; }
	};

	template< class TFun >
	class TFunctionRenderCallBack : public RenderCallBack
	{
	public:
		TFunctionRenderCallBack( TFun func ):mFunc(func){}
		TFun mFunc;
		virtual void postRender(GWidget* ui) { mFunc(ui); }
		virtual void release() { delete this; }
	};
}

template < class T , class TMemFunc >
RenderCallBack* RenderCallBack::Create( T* ptr , TMemFunc func)
{
	return new Private::TMemberRenderCallBack< T , TMemFunc >( ptr , func);
}

template < class TFun >
RenderCallBack* RenderCallBack::Create(TFun func)
{
	return new Private::TFunctionRenderCallBack< TFun >(func);
}


class  GButtonBase : public GUI::ButtonT< GButtonBase >
{
	typedef GUI::ButtonT< GButtonBase > BaseClass;
public:
	TINY_API GButtonBase( int id , Vec2i const& pos , Vec2i const& size  , GWidget* parent )
		:BaseClass( pos , size , parent )
	{
		mID = id;
	}
	virtual void onHotkey( unsigned key ){ onClick();  }
	virtual void onClick(){  sendEvent( EVT_BUTTON_CLICK );  }
};

class  GButton : public GButtonBase
{
	typedef GButtonBase BaseClass;
public:
	TINY_API GButton( int id , Vec2i const& pos , Vec2i const& size  , GWidget* parent );

	void setTitle( char const* str ){ mTitle = str; }
	virtual void onMouse( bool beInside );
	TINY_API void onRender();
	String mTitle;
};

class GCheckBox : public GButtonBase
{
	typedef GButtonBase BaseClass;
public:
	TINY_API GCheckBox( int id , Vec2i const& pos , Vec2i const& size  , GWidget* parent );
	virtual void onHotkey( unsigned key ){ onClick();  }
	TINY_API virtual void onClick();
	TINY_API virtual void onMouse(bool beInside) override;
	TINY_API void onRender();
	
	void setTitle( char const* str ){ mTitle = str; }
	bool bChecked;

protected:

	TINY_API void updateMotionState( bool bInside );
	String mTitle;
};

class  GPanel : public GUI::PanelT< GPanel >
{
	typedef GUI::PanelT< GPanel > BaseClass;
public:
	enum RenderType
	{
		eRoundRectType,
		eRectType,
	};

	TINY_API GPanel( int id , Vec2i const& pos , Vec2i const& size  , GWidget* parent );
	TINY_API ~GPanel();

	TINY_API void onRender();
	TINY_API MsgReply onMouseMsg(MouseMsg const& msg);
	void setAlpha( float alpha ){  mAlpha = alpha; }
	void setRenderType( RenderType type ){ mRenderType = type; }

private:
	RenderType  mRenderType;
	float       mAlpha;
};

class  GFrame : public GPanel
{
	typedef GPanel BaseClass;
public:
	TINY_API GFrame( int id , Vec2i const& pos , Vec2i const& size , GWidget* parent );
protected:
	TINY_API MsgReply onMouseMsg( MouseMsg const& msg );
	virtual void onFocus(bool beF) { if( beF ) sendEvent(EVT_FRAME_FOCUS); }
};


class  GNoteBook : public GUI::NoteBookT< GNoteBook >
{
public:
	TINY_API GNoteBook( int id , Vec2i const& pos , Vec2i const& size  , GWidget* parent );

	TINY_API void doRenderButton( PageButton* button );
	TINY_API void doRenderPage( Page* page );
	TINY_API void doRenderBackground();

	Vec2i getButtonSize(){ return Vec2i( 100 , 20 ); }
	Vec2i getButtonOffset(){ return Vec2i( getButtonSize().x + 5 , 0 ); }
	Vec2i getPagePos(){ return Vec2i( 0 , getButtonSize().y + 2 ); }
	Vec2i getBaseButtonPos(){ return Vec2i( 2 , 2 ); }
};

class  GSlider : public GUI::SliderT< GSlider >
{
	using BaseClass = GUI::SliderT< GSlider >;
public:
	TINY_API GSlider( int id , Vec2i const& pos , int length , bool beH , GWidget* parent );
	static Vec2i TipSize;

	void onScrollChange( int value ){ sendEvent( EVT_SLIDER_CHANGE ); }
	TINY_API void showValue();

	void renderValue( GWidget* widget );

	virtual void onRender();
};

class  GMsgBox : public GPanel
{
public:
	static Vec2i const BoxSize;

	TINY_API GMsgBox( int _id , Vec2i const& size  , GWidget* parent , EMessageButton::Type buttonType = EMessageButton::YesNo);

	void setTitle( char const* str ){ mTitle = str; }
	bool onChildEvent( int event , int id , GWidget* ui );
	TINY_API void onRender();

	String    mTitle;
};


class  GTextCtrl : public GUI::TextCtrlT< GTextCtrl >
{
	using BaseClass = GUI::TextCtrlT< GTextCtrl >;
public:
	static const int UI_Height = 20;
	TINY_API GTextCtrl( int id , Vec2i const& pos , int length , GWidget* parent );

	void onEditText(){ sendEvent( EVT_TEXTCTRL_VALUE_CHANGED ); }
	void onPressEnter(){ sendEvent( EVT_TEXTCTRL_COMMITTED ); }
	TINY_API void onRender();
};

class GText : public GUI::Widget
{
	using BaseClass = GUI::Widget;
public:
	TINY_API GText( Vec2i const& pos, Vec2i const& size, GWidget* parent);
	TINY_API void onRender();

	GText& setText(char const* pText) { mText = pText; return *this; }
	String mText;
};

class  GChoice : public GUI::ChoiceT< GChoice >
{
	using BaseClass = GUI::ChoiceT< GChoice >;

public:
	TINY_API GChoice( int id , Vec2i const& pos , Vec2i const& size , GWidget* parent );

	TINY_API void onRender();

	void onItemSelect( unsigned select ){  sendEvent( EVT_CHOICE_SELECT );  }
	void doRenderItem( Vec2i const& pos , Item& item , bool beLight );
	void doRenderMenuBG( Menu* menu );
	int  getMenuItemHeight(){ return 20; }

};

class  GListCtrl : public GUI::ListCtrlT< GListCtrl >
{
	using BaseClass = GUI::ListCtrlT< GListCtrl >;
public:
	TINY_API GListCtrl( int id , Vec2i const& pos , Vec2i const& size , GWidget* parent );

	void onItemSelect( unsigned pos ){ ensureVisible(pos); sendEvent( EVT_LISTCTRL_SELECT ); }
	void onItemLDClick( unsigned pos ){ sendEvent( EVT_LISTCTRL_DCLICK ); }
	void doRenderItem( Vec2i const& pos , Item& item , bool beSelected );
	void doRenderBackground( Vec2i const& pos , Vec2i const& size );
	int  getItemHeight(){ return 20; }
};

class GFileListCtrl : public GListCtrl
{
	using BaseClass = GListCtrl;
public:
	TINY_API GFileListCtrl(int id, Vec2i const& pos, Vec2i const& size, GWidget* parent);

	TINY_API String getSelectedFilePath() const;
	TINY_API void deleteSelectdFile();
	TINY_API void setDir(char const* dir);
	TINY_API void refreshFiles();

	String mSubFileName;
	String mCurDir;

};

template< class TFun >
class WidgetMotionTask : public TaskBase
{
public:
	WidgetMotionTask( GWidget* w , Vec2i const& f, Vec2i const& e , long t , long d )
		:widget( w ),from( f ),to( e ),time( t ),delay( d ){ bFinish = false; }

	void onStart()
	{
		::Global::GUI().addMotion< TFun >( widget , from , to , time , delay , std::tr1::bind( WidgetMotionTask< TFun >::onFinish , this ) ); 
	}
	bool onUpdate( long time ){ return false; }
	void onEnd( bool beComplete ){ if ( !beComplete ) widget->setPos( to ); }

	void onFinish(){ bFinish = true; }

	bool      bFinish;
	GWidget*  widget;
	Vec2i     from;
	Vec2i     to;
	long      time;
	long      delay;
};

enum WidgetMotionType
{
	WMT_LINE_MOTION ,
	WMT_SHAKE_MOTION ,
};

class UIMotionTask : public LifeTimeTask
{
public:
	using MotionFun = void (UIMotionTask ::*)();

	TINY_API UIMotionTask( GWidget* _ui , Vec2i const& _ePos , long time , WidgetMotionType type );
	void   release();
	
	GWidget*  ui;
	Vec2i     sPos;
	Vec2i     ePos;
	long      tParam;
	WidgetMotionType mType;
	
	void onStart();
	bool onUpdate( long time );
	void onEnd( bool beComplete );

	void  LineMotion();
	void  ShakeMotion();
};

class WidgetRenderer
{
public:
	TINY_API void  drawButton( IGraphics2D& g , Vec2i const& pos , Vec2i const& size , ButtonState state , WidgetColor const& color , bool beEnable = true );
	TINY_API void  drawButton2( IGraphics2D& g , Vec2i const& pos , Vec2i const& size , ButtonState state , WidgetColor const& color, bool beEnable = true );
	TINY_API void  drawPanel( IGraphics2D& g ,Vec2i const& pos , Vec2i const& size , WidgetColor const& color, float alpha );
	TINY_API void  drawPanel( IGraphics2D& g ,Vec2i const& pos , Vec2i const& size , WidgetColor const& color);
	TINY_API void  drawPanel2( IGraphics2D& g , Vec2i const& pos , Vec2i const& size , WidgetColor const& color);
	TINY_API void  drawSilder( IGraphics2D& g ,Vec2i const& pos , Vec2i const& size , Vec2i const& tipPos , Vec2i const& tipSize );
};

#endif // GameWidget_h__