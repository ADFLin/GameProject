#include "common.h"
#include "TGame.h"
#include "TGameStage.h"

WNDPROC g_flyWinProc = 0;
extern char const* g_WinTitle;

#ifndef WM_MOUSEWHEEL 
#define WM_MOUSEWHEEL 0x020A
#endif
#ifndef WHEEL_DELTA
#define WHEEL_DELTA 120
#endif


LRESULT CALLBACK TFlyWinProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
void InvokeGame();

static WORLDid wID;

static void GameAI(int skip)
{
	g_Game->update( skip );

	if ( g_exitGame )
	{
		delete g_Game;
		FyWin32EndWorld( wID );
	}
}

static void GameRender(int skip)
{
	g_Game->render( skip );
}

void main(int argc, char **argv)
{
	InvokeGame();

	wID = FyWin32CreateWorld( ( char*) g_WinTitle , 0 ,0 , g_ScreenWidth , g_ScreenHeight , 32 , FALSE );

	if ( !g_Game->OnInit( wID ) )
		return;

	HWND hwnd =  FyGetWindowHandle( wID );
	g_flyWinProc = (WNDPROC)SetWindowLong( hwnd , GWL_WNDPROC , (LONG) TFlyWinProc );

	FyBindTimer(0, 30.0f, GameAI, TRUE);
	FyBindTimer(1, 30.0f, GameRender, FALSE);

	FyInvokeTheFly(TRUE);


}


LRESULT CALLBACK  TFlyWinProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static unsigned state = 0;
	static bool drag = false;
	int x , y;

	switch (message)
	{
	case WM_KEYUP:
		g_Game->OnMessage( wParam , false );
		return 0;
	case WM_KEYDOWN:
		g_Game->OnMessage( wParam , true );
		return 0;
	case  WM_CHAR:
		g_Game->OnMessage( wParam );
		return 0;

#define GET_MOUSE_POS()\
		x = (int) LOWORD(lParam);\
		y = (int) HIWORD(lParam);
		//if (x >= 32767) x -= 65536;\
		//if (y >= 32767) y -= 65536;


#define CASE_MOUSE_MSG( BUTTON , WDOWN , WUP , WDCLICK )\
	case WDOWN:\
		state |= BUTTON;\
		GET_MOUSE_POS()\
		SetCapture(hwnd);\
		g_Game->OnMessage( MouseMsg( x, y , BUTTON | MBS_DOWN  , state ) );\
		return 0;\
	case WUP:\
		GET_MOUSE_POS()\
		state &= ~BUTTON ;\
		g_Game->OnMessage( MouseMsg( x, y , BUTTON , state ) );\
		ReleaseCapture();\
		return 0;\
	case WDCLICK:\
		GET_MOUSE_POS()\
		g_Game->OnMessage( MouseMsg( x, y , BUTTON | MBS_DOUBLE_CLICK  , state ) );\
		return 0;

	CASE_MOUSE_MSG( MBS_MIDDLE , WM_MBUTTONDOWN , WM_MBUTTONUP , WM_MBUTTONDBLCLK )
	CASE_MOUSE_MSG( MBS_LEFT   , WM_LBUTTONDOWN , WM_LBUTTONUP , WM_LBUTTONDBLCLK )
	CASE_MOUSE_MSG( MBS_RIGHT  , WM_RBUTTONDOWN , WM_RBUTTONUP , WM_RBUTTONDBLCLK )


	case WM_MOUSEMOVE:
		if ( state )
		{
			GET_MOUSE_POS()
			g_Game->OnMessage( MouseMsg( x , y , MBS_DRAGING | MBS_MOVING , state ) );
		}
		else
		{
			GET_MOUSE_POS()
			g_Game->OnMessage( MouseMsg( x , y , MBS_MOVING , state ) );
		}
		return 0;
	case WM_MOUSEWHEEL:
		{
			GET_MOUSE_POS()
			int zDelta = HIWORD(wParam);
			if ( zDelta == WHEEL_DELTA )
			{
				g_Game->OnMessage( MouseMsg( x , y , MBS_WHEEL , state ) );
			}
			else
			{
				g_Game->OnMessage( MouseMsg( x , y , MBS_WHEEL | MBS_DOWN , state ) );
			}
		}

		return 0;
#undef CASE_MOUSE_MSG
#undef GET_MOUSE_POS
	}

	return CallWindowProc( g_flyWinProc , hwnd, message , wParam , lParam );
}

