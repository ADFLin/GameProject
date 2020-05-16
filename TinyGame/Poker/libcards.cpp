#include "libcards.h"

int CardLib::CardSizeX = 0;
int CardLib::CardSizeY = 0;
HMODULE CardLib::hCardDll = NULL;

BOOL (WINAPI* cdtAnimate)(HDC hDC, int cardBack, int xOrg, int yOrg, int state);
BOOL (WINAPI* cdtDraw)(HDC, int, int, int, int, COLORREF);
BOOL (WINAPI* cdtDrawExt)(HDC hDC, int xOrg, int yOrg, int width, int height, int card, int draw, DWORD color);
BOOL (WINAPI* cdtInit)(int*, int*);
BOOL (WINAPI* cdtTerm)(void);


template<class TFunc >
static inline void GetFunctionAddress( HMODULE dll , TFunc& func , char* funcName )
{
	func = (TFunc) GetProcAddress( dll , funcName );
}

bool CardLib::InitCardsDll32()
{
	if ( hCardDll ) return true;

	//this only works with NT
	if(GetVersion() >= 0x80000000)
		return false;

	if ( ( hCardDll = ::LoadLibraryA("cards.dll") ) == 0 ) 
		return false;

	GetFunctionAddress( hCardDll , cdtInit , "cdtInit" );
	GetFunctionAddress( hCardDll , cdtDraw , "cdtDraw" );
	GetFunctionAddress( hCardDll , cdtDrawExt , "cdtDrawExt" );
	GetFunctionAddress( hCardDll , cdtAnimate , "cdtAnimate" );
	GetFunctionAddress( hCardDll , cdtTerm , "cdtTerm" );


	cdtInit( &CardSizeX , &CardSizeY );
	return true;
}

CardLib::CardLib()
{
	InitCardsDll32();
}

CardLib::~CardLib()
{
	if ( hCardDll )
	{
		cdtTerm();
		FreeLibrary(hCardDll);
		hCardDll = NULL;
	}
}
