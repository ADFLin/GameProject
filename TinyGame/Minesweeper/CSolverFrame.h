#ifndef CSolverFrame_h__
#define CSolverFrame_h__

#if 0
#include <atlbase.h>
#include <atlapp.h>
extern CAppModule _Module;
#include <atlwin.h>
#include <atlframe.h>
#include <atlcrack.h>
#include <atlmisc.h>
#include <atlctrls.h>

#include "MineSweeperSolver.h"
#include "GControlerWin7.h"
#include "ExpSolveStrategy.h"

#include "resource.h"


#include "ScanEditorDlg.h"
#include "ImageScanser.h"

enum
{
	IDR_MAINFRAME = 10000 ,
};

typedef CWinTraits<
	( WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS ) & ~( WS_THICKFRAME | WS_MAXIMIZEBOX ) ,
	WS_EX_APPWINDOW | WS_EX_WINDOWEDGE 
> MyWinTraits;

class CSolverFrame : public CFrameWindowImpl<CSolverFrame , CWindow ,MyWinTraits >
	, public CUpdateUI< CSolverFrame >
	//, public CMouseEventHandler< CSudokuFrame >
	, public CDoubleBufferImpl< CSolverFrame >
	, public CMessageFilter 
	, public CIdleHandler
{
public:

	typedef CSolverFrame ThisClass;
	typedef CFrameWindowImpl<ThisClass , CWindow ,MyWinTraits > BaseFrame;

	DECLARE_FRAME_WND_CLASS_EX( "MineSweeperSolverFrame" , IDR_MAINFRAME , CS_DBLCLKS , COLOR_WINDOW )

	BEGIN_UPDATE_UI_MAP(CSolverFrame)
	END_UPDATE_UI_MAP()

	BEGIN_MSG_MAP(CSolverFrame)
		MSG_WM_CREATE( OnCreate )
		MSG_WM_DESTROY( OnDestroy )
		//MSG_WM_PAINT( OnPaint )
		MSG_WM_TIMER( OnTimer )
		//MSG_WM_SIZING( OnSizing )
		MSG_WM_LBUTTONDOWN( OnMouseLDown )
		MSG_WM_LBUTTONDBLCLK( OnMouseDClick )
		MSG_WM_RBUTTONDOWN( OnMouseRDown )
		//CHAIN_MSG_MAP( CMouseEventHandler< CFractalFrame > )
		//CHAIN_MSG_MAP_MEMBER( m_selectRect )
		COMMAND_RANGE_HANDLER( ID_MENU_START , ID_MENU_END , OnMenuItem )

		CHAIN_MSG_MAP( CUpdateUI< CSolverFrame > )
		CHAIN_MSG_MAP( CDoubleBufferImpl< CSolverFrame > )
		CHAIN_MSG_MAP( BaseFrame )
	END_MSG_MAP()

	CSolverFrame();

	int     OnCreate( LPCREATESTRUCT lpCreateStruct );
	void    OnDestroy();
	LRESULT OnMenuItem( WORD code, WORD id, HWND hwnd, BOOL& bHandled);
	void    OnTimer(UINT_PTR nIDEvent);
	BOOL    OnIdle()
	{
		return TRUE;
	}

	bool init()
	{
		if ( !mControler.hookGame() )
		{
			::MessageBox(NULL, TEXT("Can't Find Game.") , TEXT("") , MB_OK );
			return false;
		}

		mScanBuilder.setGroup( &mBuildGroup );
		mScanBuilder.setHDC( mControler.hDC );
		mBuildHWND  = mControler.hWndStatic;

		return true;
	}

	void OnMouseHook( WPARAM wParam , MOUSEHOOKSTRUCT* mhs );

	int OnMouseRDown( UINT wParam , CPoint const& pt )
	{
		CPoint p = ( pt - mDrawOrg ) / LengthCell;
		mStragtegy.checkPos( p.x , p.y );
		return 0;
	}
	int OnMouseLDown( UINT wParam , CPoint const& pt )
	{
		mCellPos = ( pt - mDrawOrg ) / LengthCell;
		this->RedrawWindow();
		return 0;
	}
	int OnMouseDClick( UINT wParam , CPoint const& pt )
	{
		mCellPos = ( pt - mDrawOrg ) / LengthCell;
		::Sleep( 300 );
		try
		{
			//FIXME
			this->RedrawWindow();
		}
		catch ( ControlException& )
		{

		}
		return 0;
	}

	void DoPaint(CDCHandle dc );

	BOOL PreTranslateMessage(MSG* pMsg)
	{
		return BaseFrame::PreTranslateMessage(pMsg);
	}
	bool OnCheckMenu( WORD id );
	void OnKeyBoardHook( KBDLLHOOKSTRUCT* s );


	//ScanEditorDlg mDlg;

	CPoint            mCellPos;
	GControlerWin7    mControler;
	MineSweeperSolver mSolver;
	ExpSolveStrategy  mStragtegy;

	CMenu  mMenu;
	CPoint mDrawOrg;
	static int const LengthCell = 20;

	ScanBuilder mScanBuilder;
	ScanGroup   mBuildGroup;
	HWND        mBuildHWND ;

};

#endif

#endif // CSolverFrame_h__