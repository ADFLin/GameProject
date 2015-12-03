#include "ZumaPCH.h"

#include "ZBase.h"
#include "DXZumaGame.h"

int const g_ScreenWidth  = 640;
int const g_ScreenHeight = 480;

int DBG_X = 0;
int DBG_Y = 0;

using namespace std;

DXZumaGame game;

int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
	g_GameTime.curTime    = 0;
	g_GameTime.updateTime = 20;
	g_GameTime.nextTime   = g_GameTime.curTime + g_GameTime.updateTime;

	game.setUpdateTime( g_GameTime.updateTime );
	game.run();
}