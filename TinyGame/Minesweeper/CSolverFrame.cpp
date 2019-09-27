#include "CSolverFrame.h"

#if 0

CSolverFrame* gFrame = NULL;
LRESULT __stdcall CALLBACK MouseHookProc( int code , WPARAM wParam , LPARAM lParam )
{
	gFrame->OnMouseHook( wParam , (MOUSEHOOKSTRUCT*)lParam );
	return 0;
}


static LRESULT __stdcall CALLBACK KeyboardHookProc( int code , WPARAM wParam , LPARAM lParam)
{
	gFrame->OnKeyBoardHook( (KBDLLHOOKSTRUCT*) lParam );
	return 0;
}

CSolverFrame::CSolverFrame() 
	:mSolver( mStragtegy , mControler )
	,mDrawOrg( 20 , 40 )
{
	gFrame = this;
}

int CSolverFrame::OnCreate( LPCREATESTRUCT lpCreateStruct )
{
	InstallKeyBoardHook( KeyboardHookProc );
	InstallMouseHook( MouseHookProc );

	this->SetWindowText(TEXT("Minesweeper Solver - Beta"));
	RECT rect;
	this->GetWindowRect( &rect );
	rect.right = rect.left + 700;
	rect.bottom = rect.top + 550;
	this->MoveWindow( &rect , FALSE );

	mMenu.Attach( ::LoadMenu( _Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_MENU)) );
	SetMenu( mMenu );

	//mDlg.Create( NULL );
	//mDlg.ShowWindow( SW_SHOW );

	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop != NULL);
	pLoop->AddMessageFilter(this);
	pLoop->AddIdleHandler(this);

	this->RedrawWindow();
	return 0;
}

void CSolverFrame::OnDestroy()
{
	//mDlg.DestroyWindow();
	SetMsgHandled( FALSE );
}



void CSolverFrame::OnKeyBoardHook( KBDLLHOOKSTRUCT* s )
{
	if( s->vkCode == VK_F9 )
	{
		mSolver.enableSetting( ST_PAUSE_SOLVE , true );
	}
	else if ( s->vkCode == VK_F10 )
	{
		mSolver.enableSetting( ST_PAUSE_SOLVE , false );
	}

}
void CSolverFrame::DoPaint( CDCHandle dc )
{
	dc.SetBkMode( TRANSPARENT );

	CRect rect;
	CBrush brush;
	brush.CreateSolidBrush( RGB(255,255,255) );
	this->GetClientRect( &rect );
	dc.FillRect( &rect , brush );

	CFont fontSol;
	fontSol.CreatePointFont( 300 , _T("新細明體") );

	CFont fontPsb;
	fontPsb.CreatePointFont( 100 , _T("新細明體") );

	CPoint textOrg = mDrawOrg + CPoint(5,3);

	CBrush brYellow;
	brYellow.CreateSolidBrush( RGB( 255 , 255 , 0 ) );
	CBrush brBlue;
	brBlue.CreateSolidBrush( RGB( 125 , 125 , 255 ) );
	CBrush brRed;
	brRed.CreateSolidBrush( RGB( 255 , 124 , 125 ) );
	CBrush brLightBlue;
	brLightBlue.CreateSolidBrush( RGB( 165 , 165 , 255 ) );

	char str[256];

	int idx = 0;
	for( int j = 0 ; j < mStragtegy.getCellSizeY(); ++j )
	for( int i = 0 ; i < mStragtegy.getCellSizeX(); ++i , ++idx )
	{
		CPoint offset = CPoint( i * LengthCell , j *LengthCell );
		CPoint pt = textOrg + offset;
		ExpSolveStrategy::CellData const& data = mStragtegy.getCellData( idx );

		if ( data.flag & ExpSolveStrategy::eCheck )
		{
			CRect rect( mDrawOrg + offset , CSize( LengthCell , LengthCell ) );
			dc.SelectBrush( brYellow );
			dc.Rectangle( &rect );
		}

		ExpSolveStrategy::ProbInfo const* probInfo;

		switch( data.number )
		{
		case CN_FLAG:
			dc.SelectBrush( brRed );
			dc.Ellipse( CRect( mDrawOrg + offset + CPoint( 2,2 ) , CSize( LengthCell -4  , LengthCell- 4) ) );
			break;
		case CN_UNPROBLED:
			probInfo = mStragtegy.getProbInfo( data );

			dc.SelectBrush( ( probInfo ) ? brLightBlue : brBlue );
			dc.RoundRect( CRect( mDrawOrg + offset , CSize( LengthCell , LengthCell ) ) , CPoint( 3 , 3 ) );

			if ( probInfo && mStragtegy.getSolveState() == ExpSolveStrategy::eProbSolve )
			{
				sprintf_s( str , "%d" , int( 100 * probInfo->prob ) );
				dc.TextOut( pt.x -3 , pt.y , str );
			}
			
			break;
		case 0: 
			break;
		default:
			sprintf_s( str , "%d" , data.number );
			dc.TextOut( pt.x , pt.y , str );
		}
	}

	CPen penL;
	penL.CreatePen( PS_SOLID , 1 , RGB(0,0,0) );

	int TotalSize;
	TotalSize = mStragtegy.getCellSizeY() * LengthCell;
	for( int i = 0 ; i <= mStragtegy.getCellSizeX() ; ++i )
	{
		CPoint p1 = mDrawOrg + CPoint( i * LengthCell , 0 );
		dc.MoveTo( p1 , NULL );
		dc.LineTo( p1 + CPoint( 0 , TotalSize ) );
	}
	TotalSize = mStragtegy.getCellSizeX() * LengthCell;
	for( int i = 0 ; i <= mStragtegy.getCellSizeY() ; ++i )
	{
		CPoint p1 = mDrawOrg + CPoint( 0 , i * LengthCell );
		dc.MoveTo( p1 , NULL ); 
		dc.LineTo( p1 + CPoint( TotalSize , 0 ) );
	}

	ExpSolveStrategy::ProbInfo const& otherProb = mStragtegy.getOtherProbInfo();
	int cx , cy;
	if ( mStragtegy.getSolveState() == ExpSolveStrategy::eProbSolve )
	{
		mStragtegy.getPorbSelect( cx , cy );
		sprintf_s( str , "Other Prob = %d , Select = ( %d , %d )" , int( 100 * otherProb.prob ) , cx , cy );
		dc.TextOut( 0 , 20  , str );
	}

	
	mStragtegy.getLastCheckPos( cx , cy );

	sprintf_s( str , "( %d , %d ) index = %d" , 
		(int)mCellPos.x , (int)mCellPos.y , 
		(int)(mCellPos.x + mStragtegy.getCellSizeX() * mCellPos.y) );
	dc.TextOut( 0 , 0 , str );
	sprintf_s( str , "Bomb = %d , Open Cell = %d , LastCheck Cell = ( %d , %d )" ,
		mStragtegy.getSolvedBombNum() , mStragtegy.getOpenCellNum() , cx , cy );
	dc.TextOut( 150 , 0, str );
}

LRESULT CSolverFrame::OnMenuItem( WORD code, WORD id, HWND hwnd, BOOL& bHandled )
{
	switch( id )
	{
	case ID_GAME_SOLVE:
		{
			int sx,sy;
			if ( !mControler.getCellSize(sx,sy) )
				return 0;
			mSolver.setCustomMode(sx,sy,99);
			mSolver.scanMap();
			mSolver.enableSetting( ST_PAUSE_SOLVE , false );

			this->SetTimer( 1 , 1 );
		}
		break;
	case ID_GAME_REFRESH:
		{
			int sx,sy;
			if ( !mControler.getCellSize(sx,sy) )
				return 0;

			
			//solver.setCellSize(sx,sy);
			//solver.refreshInformation();
		}
		break;
	case ID_GAME_RESTART:
		{
			mControler.setupCustomMode( 10 , 10 , 10 );
			mSolver.restart();
		}
		break;
	case ID_MARK_FLAG:
		mSolver.enableSetting( ST_MARK_FLAG , OnCheckMenu( id ) );
		break;
	case ID_FAST_OPEN:
		mSolver.enableSetting( ST_FAST_OPEN_CELL , OnCheckMenu( id ) );
		break;
	case ID_AUTO_RESTART:
		mSolver.enableSetting( ST_AUTO_RESTART  , OnCheckMenu( id ) );
		break;
	case ID_DISABLE_PROB_SOLVE:
		mSolver.enableSetting( ST_DISABLE_PROB_SOLVE  , OnCheckMenu( id ) );
		break;
	}
	return 0;
}
bool CSolverFrame::OnCheckMenu( WORD id )
{
	UINT state = mMenu.GetMenuState(id , MF_BYCOMMAND);
	if (state & MF_CHECKED)
		mMenu.CheckMenuItem( id , MF_UNCHECKED | MF_BYCOMMAND);
	else
		mMenu.CheckMenuItem( id , MF_CHECKED | MF_BYCOMMAND);
	return !(state & MF_CHECKED);
}

void CSolverFrame::OnTimer( UINT_PTR nIDEvent )
{
	bool result;
	int count = 0;
	try
	{
		do 
		{
			result = mSolver.setepSolve();
			this->RedrawWindow();
			++count;
			if ( count > 4 )
				break;
		} while ( result );

		if ( mSolver.getGameState() == GS_FAIL )
		{
			this->KillTimer( nIDEvent );
		}
	}
	catch ( ControlException& e )
	{
		mSolver.enableSetting( ST_PAUSE_SOLVE , true );
		//this->MessageBox( e.what() );
	}
}


void CSolverFrame::OnMouseHook( WPARAM wParam , MOUSEHOOKSTRUCT* mhs )
{
	if ( mBuildHWND == mhs->hwnd )
	{
		switch( wParam )
		{
		case WM_LBUTTONDOWN:
			break;
		}
		//POINT pt = mhs->pt;
		//::ScreenToClient( hWndStatic , &pt );

		//std::cout << "On Window!" << pt.x << "," << pt.y << std::endl;
		//COLORREF color = ::GetPixel( hDC , pt.x , pt.y );

		//float scale = calcCellScale();
		//int cx , cy;
		//WindowPosToCell( scale  , pt.x , pt.y , cx , cy );

		//int x , y;
		//CellToWindowPos( scale , cx , cy , x , y  );

		//std::cout << "Color(" << (int)GetRValue(color) << ","
		//	<< (int)GetGValue(color) << ","
		//	<< (int)GetBValue(color) << ")"
		//	<< "Num = " << getCellNumber( cx , cy , false ) << " "
		//	<< "pos = (" << pt.x - x << "," << pt.y - y  << ")"
		//	<< "cell = (" << cx << "," << cy << ")"
		//	<< std::endl;
	}
}

#endif