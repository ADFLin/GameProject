#pragma once
#ifndef WindowsMessageHander_H_B9615608_C535_4E09_881F_D1CDF5D3734E
#define WindowsMessageHander_H_B9615608_C535_4E09_881F_D1CDF5D3734E


#include "WindowsHeader.h"
#include "SystemMessage.h"
#include "Meta/MetaBase.h"
#include "Meta/Select.h"
#include "Core/CRTPCheck.h"


enum
{
	MSG_MOUSE    = BIT(0),
	MSG_KEY      = BIT(1),
	MSG_CHAR     = BIT(2),
	MSG_ACTIAVTE = BIT(3),
	MSG_PAINT    = BIT(4),	
	MSG_DESTROY  = BIT(5),
	MSG_DATA     = BIT(6),
};

namespace Private
{
	struct MouseData
	{
		MouseData()
		{
			mMouseState = 0;
		}
		unsigned mMouseState;
	};
}

#define MSG_DEUFLT MSG_CHAR | MSG_KEY | MSG_MOUSE | MSG_ACTIAVTE | MSG_PAINT

template< class T , unsigned MSG = MSG_DEUFLT >
class WindowsMessageHandlerT : private TSelect< MSG & MSG_MOUSE , Private::MouseData , Meta::EmptyType >::Type
{

	typedef WindowsMessageHandlerT< T , MSG >  ThisType;
public:
	typedef WindowsMessageHandlerT< T , MSG >  WindowsMessageHandler;

	WindowsMessageHandlerT()
	{
		sMsgHandler = this;
	}

public:
	CRTP_FUNC bool handleMouseEvent( MouseMsg const& msg ){ return true; }
	CRTP_FUNC bool handleKeyEvent(KeyMsg const& msg ){ return true; }
	CRTP_FUNC bool handleCharEvent( unsigned code ){ return true; }
	CRTP_FUNC bool handleWindowActivation( bool beA ){ return true; }
	CRTP_FUNC void handleWindowPaint( HDC hDC ){}
	CRTP_FUNC bool handleWindowDestroy(HWND hWnd) { return true; }
	CRTP_FUNC bool handleIMECharEvent(){ return true;}
	CRTP_FUNC bool handleDataPaste(){ return true; }

public:
	static LRESULT CALLBACK MsgProc( HWND hWnd , UINT msg , WPARAM wParam , LPARAM lParam );

private:
	static WindowsMessageHandlerT* sMsgHandler;

	static EKeyCode::Type ConvToKeyCode(WPARAM wParam) { return EKeyCode::Type(wParam); }

	inline bool _procDefault( HWND hWnd , UINT msg , WPARAM wParam , LPARAM lParam , LRESULT& result )
	{
		return true;
	}


	bool _procMouseMsg( HWND hWnd , UINT msg , WPARAM wParam , LPARAM lParam , LRESULT& result);

	inline bool _procKeyMsg( HWND hWnd , UINT msg , WPARAM wParam , LPARAM lParam , LRESULT& result )
	{
		return _this()->handleKeyEvent( KeyMsg(ConvToKeyCode(wParam) , msg == WM_KEYDOWN) );
	}

	inline bool _procCharMsg( HWND hWnd ,UINT msg , WPARAM wParam , LPARAM lParam , LRESULT& result )
	{
		return _this()->handleCharEvent( unsigned(wParam) );
	}
	inline bool _procActivateMsg( HWND hWnd ,UINT msg , WPARAM wParam , LPARAM lParam , LRESULT& result )
	{
		if ( wParam == WA_ACTIVE || wParam == WA_CLICKACTIVE )
			return _this()->handleWindowActivation( true );
		else if ( wParam == WA_INACTIVE )
			return _this()->handleWindowActivation( false );
		return true;
	}
	bool _procPaintMsg( HWND hWnd ,UINT msg , WPARAM wParam , LPARAM lParam , LRESULT& result )
	{
		HDC hDC = ::BeginPaint( hWnd , NULL );
		_this()->handleWindowPaint( hDC );
		::EndPaint( hWnd , NULL );
		return false;
	}

	bool _procDestroyMsg( HWND hWnd ,UINT msg , WPARAM , LPARAM , LRESULT& result )
	{
		if( _this()->handleWindowDestroy(hWnd) )
		{
			::PostQuitMessage(0);
		}
		return true;
	}

	bool _procPasteMsg( HWND hWnd ,UINT msg , WPARAM , LPARAM , LRESULT& result )
	{
		return _this()->handleDataPaste();
	}
	inline  T* _this(){ return static_cast< T* >( this ); }

	template< bool (ThisType::*FUNC)( HWND , UINT , WPARAM , LPARAM , LRESULT& ) >
	struct EvalFunc
	{
		inline bool operator()( ThisType* handler , HWND hWnd , UINT msg , WPARAM wParam , LPARAM lParam , LRESULT& result )
		{
			return (handler->*FUNC)( hWnd , msg , wParam , lParam , result );
		}
	};

};


template< class T , unsigned MSG >
WindowsMessageHandlerT<T,MSG>* WindowsMessageHandlerT<T, MSG>::sMsgHandler = nullptr;

template< class T , unsigned MSG >
LRESULT CALLBACK WindowsMessageHandlerT<T, MSG>::MsgProc( HWND hWnd , UINT msg , WPARAM wParam , LPARAM lParam )
{
	LRESULT result = 0;

	using namespace Meta;
	typedef EvalFunc< &ThisType::_procDefault > DefaultEval;
	
	switch ( msg )                  /* handle the messages */
	{

#define PROC_MSG_FUNC(  CASE_MSG , FUNC )\
	{\
		typename TSelect< ( MSG & CASE_MSG ) != 0 , EvalFunc< &ThisType::FUNC > , DefaultEval >::Type evaler;\
		if ( !evaler( sMsgHandler , hWnd , msg , wParam , lParam , result ) )\
		return result;\
	}


	case WM_CREATE:
		break;

	case WM_ACTIVATE: 
		PROC_MSG_FUNC( MSG_ACTIAVTE , _procActivateMsg ); 
		break;
	case WM_KEYDOWN: case WM_KEYUP: 
		PROC_MSG_FUNC( MSG_KEY   , _procKeyMsg ); 
		break;
	case WM_CHAR:
		PROC_MSG_FUNC( MSG_CHAR ,  _procCharMsg ); 
		break;
	case WM_MBUTTONDOWN : case WM_MBUTTONUP : case WM_MBUTTONDBLCLK :
	case WM_LBUTTONDOWN : case WM_LBUTTONUP : case WM_LBUTTONDBLCLK :
	case WM_RBUTTONDOWN : case WM_RBUTTONUP : case WM_RBUTTONDBLCLK :
	case WM_MOUSEMOVE:    case WM_MOUSEWHEEL:
		PROC_MSG_FUNC( MSG_MOUSE , _procMouseMsg ); 
		break;
	case WM_PAINT:
		PROC_MSG_FUNC( MSG_PAINT , _procPaintMsg ); 
		break;
	case WM_PASTE:
		PROC_MSG_FUNC( MSG_DATA , _procPasteMsg );
		break;
	case WM_DESTROY:
		{
			typename TSelect< ( MSG & MSG_DESTROY ) != 0 , EvalFunc< &ThisType::_procDestroyMsg > , DefaultEval >::Type evaler;
			evaler( sMsgHandler , hWnd , msg , wParam , lParam , result );
		}
		break;
	case WM_IME_CHAR:
		{

		}
		break;
#undef PROC_MSG_FUN
	}

	return ::DefWindowProc ( hWnd , msg , wParam, lParam );
}

template< class T , unsigned MSG >
bool WindowsMessageHandlerT<T,MSG>::_procMouseMsg( HWND hWnd , UINT msg , WPARAM wParam , LPARAM lParam , LRESULT& result )
{
	unsigned button = 0;

	auto EnableState = [this](MouseState state , bool bEnable)
	{
		if (bEnable)
		{
			mMouseState |= state;
		}
		else
		{
			mMouseState &= ~state;
		}
	};

	EnableState(MBS_CONTROL, wParam & MK_CONTROL);
	EnableState(MBS_SHIFT, wParam & MK_SHIFT);

	switch ( msg )
	{
#define CASE_MOUSE_MSG( BUTTON , BUTTON_TYPE )\
	case WM_##BUTTON_TYPE##DOWN:\
		mMouseState |= BUTTON;\
		button = BUTTON | MBS_DOWN;\
		::SetCapture( hWnd );\
		break;\
	case WM_##BUTTON_TYPE##UP:\
		mMouseState &= ~BUTTON ;\
		button = BUTTON ;\
		::ReleaseCapture();\
		break;\
	case WM_##BUTTON_TYPE##DBLCLK:\
		button = BUTTON | MBS_DOUBLE_CLICK ;\
		break;

		CASE_MOUSE_MSG( MBS_MIDDLE , MBUTTON )
		CASE_MOUSE_MSG( MBS_LEFT   , LBUTTON )
		CASE_MOUSE_MSG( MBS_RIGHT  , RBUTTON )
#undef CASE_MOUSE_MSG

	case WM_MOUSEMOVE:
		button = MBS_MOVING;
		break;
	case WM_MOUSEWHEEL:
		{
			int zDelta = HIWORD(wParam);
			if ( zDelta == WHEEL_DELTA )
				button = MBS_WHEEL;
			else
				button = MBS_WHEEL | MBS_DOWN;
		}
		break;
	}

	int x = (int) LOWORD(lParam);
	int y = (int) HIWORD(lParam);

	if (x >= 32767) x -= 65536;
	if (y >= 32767) y -= 65536;

	return _this()->handleMouseEvent( MouseMsg( x , y , button , mMouseState ) );
}

#endif // WindowsMessageHander_H_B9615608_C535_4E09_881F_D1CDF5D3734E
