#ifndef SysMsg_h__
#define SysMsg_h__

#include "PlatformConfig.h"
#include "Math/TVector2.h"
#include "Core/IntegerType.h"

#if SYS_PLATFORM_WIN
#include "WindowsHeader.h"
#endif

using Vec2i = TVector2< int >;

#ifndef BIT
#define BIT(i) ( 1 << (i) )
#endif


enum MsgType
{
	MSG_TYPE_CHAR  ,
	MSG_TYPE_MOUSE ,
	MSG_TYPE_KEY   ,
};

namespace EKeyCode 
{
	enum Type
	{
		Cancel         = PLATFORM_WIN_VALUE(VK_CANCEL, 0x03 ) ,
		Back           = PLATFORM_WIN_VALUE(VK_BACK,  0x08 ) ,
		Tab            = PLATFORM_WIN_VALUE(VK_TAB, 0x09 ) ,
		Clear          = PLATFORM_WIN_VALUE(VK_CLEAR, 0x0C) ,
		Return         = PLATFORM_WIN_VALUE(VK_RETURN, 0x0D ),

		Shift          = PLATFORM_WIN_VALUE(VK_SHIFT, 0x10 ),
		Control        = PLATFORM_WIN_VALUE(VK_CONTROL, 0x11 ),
		Menu           = PLATFORM_WIN_VALUE(VK_MENU, 0x12 ),
		Pause          = PLATFORM_WIN_VALUE(VK_PAUSE, 0x13 ),
		Capital        = PLATFORM_WIN_VALUE(VK_CAPITAL, 0x14 ),

		Kana           = PLATFORM_WIN_VALUE(VK_KANA, 0x15 ),
		Junja          = PLATFORM_WIN_VALUE(VK_JUNJA, 0x17 ),
		Final          = PLATFORM_WIN_VALUE(VK_FINAL, 0x18 ),
		Hanja          = PLATFORM_WIN_VALUE(VK_HANJA, 0x19 ),

		Escape         = PLATFORM_WIN_VALUE(VK_ESCAPE, 0x1B ),

		Convert        = PLATFORM_WIN_VALUE(VK_CONVERT, 0x1C ),
		NonConvert     = PLATFORM_WIN_VALUE(VK_NONCONVERT, 0x1D ),
		Accept         = PLATFORM_WIN_VALUE(VK_ACCEPT, 0x1E ),
		ModeChange     = PLATFORM_WIN_VALUE(VK_MODECHANGE, 0x1F ),

		Space          = PLATFORM_WIN_VALUE(VK_SPACE, 0x20 ),
		Prior          = PLATFORM_WIN_VALUE(VK_PRIOR, 0x21 ),
		Next           = PLATFORM_WIN_VALUE(VK_NEXT, 0x22 ),
		End            = PLATFORM_WIN_VALUE(VK_END, 0x23 ),
		Home           = PLATFORM_WIN_VALUE(VK_HOME, 0x24 ),
		Left           = PLATFORM_WIN_VALUE(VK_LEFT, 0x25 ),
		Up             = PLATFORM_WIN_VALUE(VK_UP, 0x26 ),
		Right          = PLATFORM_WIN_VALUE(VK_RIGHT, 0x27 ),
		Down           = PLATFORM_WIN_VALUE(VK_DOWN, 0x28 ),
		Select         = PLATFORM_WIN_VALUE(VK_SELECT, 0x29 ),
		Print          = PLATFORM_WIN_VALUE(VK_PRINT, 0x2A ),
		Execute        = PLATFORM_WIN_VALUE(VK_EXECUTE, 0x2B ),
		Snapshot       = PLATFORM_WIN_VALUE(VK_SNAPSHOT, 0x2C ),
		Insert         = PLATFORM_WIN_VALUE(VK_INSERT, 0x2D ),
		Delete         = PLATFORM_WIN_VALUE(VK_DELETE, 0x2E ),
		Help           = PLATFORM_WIN_VALUE(VK_HELP, 0x2F ),

		Num0           = '0' ,
		Num1 , Num2 , Num3 , Num4 , Num5 , Num6 , Num7 , Num8 , Num9 ,

		A              = 'A',
		B , C , D , E , F , G , H , I , J , K ,
		L , M , N , O , P , Q , R , S , T , U , V , 
		W , X , Y , Z ,

		Sleep          = 0x5F ,

		Numpad0        = 0x60 ,
		Numpad1 , Numpad2 , Numpad3 , Numpad4 , Numpad5 , Numpad6 , Numpad7 , Numpad8 , Numpad9 ,

		Multiply       = 0x6A ,
		Add            = 0x6B ,
		Separator      = 0x6C ,
		Subtract       = 0x6D ,
		Decimal        = 0x6E ,
		Divide         = 0x6F ,

		F1             = 0x70 ,
		F2 , F3 , F4 , F5 , F6 , F7  , F8 , F9 , F10 , 
		F11 , F12 , F13 , F14 , F15 ,

		NumLock        = 0x90 ,
		Scroll         = 0x91 ,

		LShift         = 0xA0 ,
		RShift         = 0xA1 ,
		LControl       = 0xA2 ,
		RControl       = 0xA3 ,
		LMenu          = 0xA4 ,
		RMenu          = 0xA5 ,

		Oem3           = 0xC0 ,
	};
}

class SystemMessage
{
public:
	SystemMessage( MsgType type) : mType( type ){}
	MsgType getType(){ return mType; }
private:
	MsgType mType;
};

struct Mouse
{
	enum Enum
	{
		eLBUTTON = 0,
		eMBUTTON  ,
		eRBUTTON  ,
		eXBUTTON1 ,
		eXBUTTON2 ,
	};
};

enum MouseState
{
	MBS_LEFT         = BIT(0),
	MBS_MIDDLE       = BIT(1),
	MBS_RIGHT        = BIT(2),
	MBS_DOUBLE_CLICK = BIT(4),
	MBS_DOWN         = BIT(5),
	MBS_MOVING       = BIT(6),
	MBS_WHEEL        = BIT(7),

	MBS_BUTTON_MASK  = MBS_LEFT | MBS_MIDDLE | MBS_RIGHT,
	MBS_BUTTON_ACTION_MASK = MBS_DOWN | MBS_DOUBLE_CLICK ,
	MBS_ACTION_MASK  = MBS_BUTTON_ACTION_MASK | MBS_WHEEL | MBS_MOVING ,
};



class MouseMsg : public SystemMessage
{
public:
	MouseMsg()
		: SystemMessage( MSG_TYPE_MOUSE )
		, msg(0),state(0)
	{

	}
	MouseMsg( Vec2i const& pos , uint16 m , uint16 s )
		: SystemMessage( MSG_TYPE_MOUSE )
		, pos( pos ), msg( m ), state(s )
	{
	}
	MouseMsg( int x , int y , uint16 m , uint16 s )
		: SystemMessage( MSG_TYPE_MOUSE )
		,pos(x,y),msg( m ) ,state(s)
	{
	}
	int    x() const { return pos.x; }
	int    y() const { return pos.y; }
	Vec2i const& getPos() const { return pos; }
	void   setPos( Vec2i const& p ){ pos = p; }

	bool isLeftDown()   const { return ( state & MBS_LEFT ) != 0; }
	bool isMiddleDown() const { return ( state & MBS_MIDDLE ) != 0; }
	bool isRightDown()  const { return ( state & MBS_RIGHT ) != 0; }
	bool isDraging()    const { return ( msg & MBS_MOVING ) && ( state != 0 ); }

	bool onMoving()     const { return ( msg & MBS_MOVING ) != 0; }
	bool onWheelFront() const { return msg ==  MBS_WHEEL ;}
	bool onWheelBack()  const { return msg ==  ( MBS_WHEEL | MBS_DOWN );}

	bool onDown()       const { return ( msg & MBS_DOWN ) != 0; }
	bool onLeftUp()     const { return msg == MBS_LEFT; }
	bool onMiddleUp()   const { return msg == MBS_MIDDLE; }
	bool onRightUp()    const { return msg == MBS_RIGHT; }
	bool onLeftDClick() const { return msg ==( MBS_LEFT | MBS_DOUBLE_CLICK); }
	bool onRightDClick()const { return msg ==( MBS_RIGHT | MBS_DOUBLE_CLICK); }
	bool onLeftDown()   const { return msg ==( MBS_LEFT | MBS_DOWN ); }
	bool onRightDown()  const { return msg ==( MBS_RIGHT | MBS_DOWN ); }
	bool onMiddleDown() const { return msg ==( MBS_MIDDLE | MBS_DOWN ); }

	uint16  getState() const { return state; }
	uint16  getMsg()   const { return msg;  }
private:
	Vec2i  pos;
	uint16  msg;
	uint16  state;
};

class KeyMsg : public SystemMessage
{
public:
	KeyMsg(EKeyCode::Type code, bool bDown)
		:SystemMessage(MSG_TYPE_KEY)
		,mCode(code)
		,mbDown(bDown)
	{

	}

	EKeyCode::Type getCode() const { return mCode; }
	bool isDown() const { return mbDown; }


	EKeyCode::Type mCode;
	bool mbDown;
};

#endif // SysMsg_h__
