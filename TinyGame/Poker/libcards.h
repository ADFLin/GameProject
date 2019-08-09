#ifndef libcards_h__
#define libcards_h__

#include <Windows.h>

//Card Backs
/* use in the card field
CAUTION: when card > 53 then draw must be = 1 (C_BACKS) */
#define cbCROSSHATCH  53
#define cbWEAVE1      54
#define cbWEAVE2      55
#define cbROBOT       56
#define cbFLOWERS     57
#define cbVINE1       58
#define cbVINE2       59
#define cbFISH1       60
#define cbFISH2       61
#define cbSHELLS      62
#define cbCASTLE      63
#define cbISLAND      64
#define cbCARDHAND    65
#define cbUNUSED      66
#define cbTHE_X       67
#define cbTHE_O       68

#define csCLUBS    0
#define csDIAMONDS 1
#define csHEARTS   2
#define csSPADES   3

#define mdFACES         0
#define mdBACKS         1
#define mdINVERT        2
#define mdGHOST         3
#define mdREMOVE        4
#define mdGHOST_INVERT  5
#define mdDECK_X        6
#define mdDECK_O        7


#define cdVALUE(face, suit) ( (face) * 4 + (suit) )

extern BOOL (WINAPI* cdtAnimate)(HDC hDC, int cardBack, int xOrg, int yOrg, int state);
extern BOOL (WINAPI* cdtDraw)(HDC, int, int, int, int, COLORREF);
extern BOOL (WINAPI* cdtDrawExt)(HDC hDC, int xOrg, int yOrg, int width, int height, int card, int draw, DWORD color);
extern BOOL (WINAPI* cdtInit)(int*, int*);
extern BOOL (WINAPI* cdtTerm)(void);


/*-----------------------------------------------------------------------------
|    cdtInit
|
|        Initialize cards.dll -- called once at app boot time.
|
|    Arguments:
|        int FAR *pdxCard: returns card width
|        int FAR *pdyCard: returns card height
|
|    Returns:
|        TRUE if successful.
BOOL _declspec(dllexport) cdtInit(int FAR *pdxCard, int FAR *pdyCard);
-------------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
|    cdtDraw
|
|        Draw a card
|
|    Arguments:
|        HDC hdc
|        int x: upper left corner of the card
|        int y: upper left corner of the card
|        int cd: card to draw (depends on md)
|        int md: mode
|           mdFaceUp:    draw face up card (cd in cdAClubs..cdKSpades)
|           mdFaceDown:  draw face down card (cd in cdFaceDown1..cdFaceDown12)
|           mdHilite:    draw face up card inversely
|           mdGhost:     draw a ghost card, cd ignored
|           mdRemove:    draw rectangle of background color at x,y
|           mdDeckX:     draw an X
|           mdDeckO:     draw an O
|        DWORD rgbBgnd: table background color (only required for mdGhost and
|                                                mdRemove)
|
|    Returns:
|        TRUE if successful
BOOL _declspec(dllexport) cdtDraw(HDC hdc, int x, int y, int cd, int md, DWORD rgbBgnd);
-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
|    cdtDrawExt
|
|        Same as cdtDraw except will stretch the cards to an arbitray extent
|
|    Arguments:
|        HDC hdc
|        int x
|        int y
|        int dx
|        int dy
|        int cd
|        int md
|        DWORD rgbBgnd:
|
|    Returns:
|        TRUE if successful
BOOL _declspec(dllexport) cdtDrawExt(HDC hdc, int x, int y, int dx, int dy,
int cd, int md, DWORD rgbBgnd);
-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
|    cdtAnimate
|
|        Draws the animation on a card.  Four cards support animation:
|
|      cd         #frames    description
|   cdFaceDown3   4          robot meters
|   cdFaceDown10  2          bats flapping
|   cdFaceDown11  4          sun sticks tongue out
|   cdFaceDown12  4          cards running up and down sleave
|
|    Call cdtAnimate every 250 ms for proper animation speed.
|
|    Arguments:
|        HDC hdc
|        int cd    cdFaceDown3, cdFaceDown10, cdFaceDown11 or cdFaceDown12
|        int x:    upper left corner of card
|        int y
|        int ispr  sprite to draw (0..1 for cdFaceDown10, 0..3 for others)
|
|    Returns:
|       TRUE if successful
BOOL _declspec(dllexport) cdtAnimate(HDC hdc, int cd, int x, int y, int ispr);
-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
|    cdtTerm
|
|        Call once at app termination
VOID _declspec(dllexport) cdtTerm(VOID);
-----------------------------------------------------------------------------*/



class CardLib
{
public:
	CardLib();
	~CardLib();
	static bool isInitialized(){ return hCardDll != NULL; }
	static int  CardSizeX;
	static int  CardSizeY;
private:
	static HMODULE hCardDll;
	static bool InitCardsDll32();
};
#endif // libcards_h__
