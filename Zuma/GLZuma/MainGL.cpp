#include "ZumaPCH.h"
#include "ZBase.h"

#include "GLZumaGame.h"


using namespace std;
using namespace Zuma;

GLZumaGame game;
#include <type_traits>

int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
	game.setUpdateTime( g_GameTime.updateTime );
	game.run();
}
