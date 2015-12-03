#include "libcards.h"

int CardLib::CardSizeX = 0;
int CardLib::CardSizeY = 0;
HMODULE CardLib::hCardDll = NULL;

BOOL (WINAPI* cdtAnimate)(HDC hDC, int cardBack, int xOrg, int yOrg, int state);
BOOL (WINAPI* cdtDraw)(HDC, int, int, int, int, COLORREF);
BOOL (WINAPI* cdtDrawExt)(HDC hDC, int xOrg, int yOrg, int width, int height, int card, int draw, DWORD color);
BOOL (WINAPI* cdtInit)(int*, int*);
BOOL (WINAPI* cdtTerm)(void);


template<class FunType>
static inline void readProcAddress( HMODULE dll , FunType& fun , char* funName )
{
	fun = (FunType) GetProcAddress( dll , funName );
}

bool CardLib::InitCardsDll32()
{
	if ( hCardDll ) return true;

	//this only works with NT
	if(GetVersion() >= 0x80000000)
		return false;

	if ( ( hCardDll = ::LoadLibrary("cards.dll") ) == 0 ) 
		return false;

	readProcAddress( hCardDll , cdtInit , "cdtInit" );
	readProcAddress( hCardDll , cdtDraw , "cdtDraw" );
	readProcAddress( hCardDll , cdtDrawExt , "cdtDrawExt" );
	readProcAddress( hCardDll , cdtAnimate , "cdtAnimate" );
	readProcAddress( hCardDll , cdtTerm , "cdtTerm" );


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
