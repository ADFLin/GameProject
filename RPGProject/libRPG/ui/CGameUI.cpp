#include "ProjectPCH.h"
#include "CGameUI.h"

#include "CMiniMapUI.h"
#include "CVitalStateUI.h"
#include "CItemBagPanel.h"
#include "CFastPlayBar.h"
#include "CEquipFrame.h"
#include "CTextButton.h"

#include "CChestPanel.h"
#include "CStoreFrame.h"
#include "CSkillFrame.h"

#include "CBloodBar.h"

#include "CHero.h"
#include "TSkillBook.h"
#include "ObjectHandle.h"
#include "EventType.h"
#include "UtilityGlobal.h"

#include "ControlKey.h"


Vec2i CBuffBarUI::Size( 200 , 20 );
Vec2i CTalkWidget::Size( 800 - 10 * 2 - ActorPicSize - 100 , LineOffset * MaxLineNum + 20 );
CFont  CTalkWidget::sTextFont;

CGameUI::CGameUI( CHero* player )
{

	Vec2i resizePos(0,0);

	CUISystem& uiSystem = CUISystem::getInstance();

	equipPanel   = new CEquipFrame( player , Vec2i( 100 , 100 ) );
	equipPanel->show( false );
	uiSystem.addWidget( equipPanel );
	skillPanel   = new CSkillFrame( player , Vec2i( 300, 200 ) );
	skillPanel->show( false );
	uiSystem.addWidget( skillPanel  );
	
	itemBagPanel = new CItemBagFrame( player , Vec2i( 400 , 200 ) );
	itemBagPanel->show( false );
	uiSystem.addWidget( itemBagPanel );


	fastPlayBar   = new CFastPlayBar( resizePos , NULL  ); 
	uiSystem.addWidget( fastPlayBar  );
	vitalStateUI  = new CVitalStateUI( resizePos , this , player->getAbilityProp() );
	uiSystem.addWidget( vitalStateUI );

	CStoreFrame* panel = new CStoreFrame( Vec2i( 200 , 200 ) );
	uiSystem.addWidget( panel );
	panel->show( false );

	chestPanel   = new CChestPanel( player , Vec2i( 0 , 400 ) );
	uiSystem.addWidget( chestPanel );
	chestPanel->show( false );

	buffBarUI = new CBuffBarUI( resizePos , player ,this );
	uiSystem.addWidget( buffBarUI );
	buffBarUI->show( false );
	talkUI    = new CTalkWidget( resizePos );
	uiSystem.addWidget( talkUI );
	talkUI->show( false );

	int length = 100;
	showButton[ SB_ITEMBAG ]= new CTextButton( "Bag" , length , resizePos , NULL );
	uiSystem.addWidget( showButton[ SB_ITEMBAG ] );
	showButton[ SB_EQUIP ] = new CTextButton( "Equip" , length , resizePos , NULL );
	uiSystem.addWidget( showButton[ SB_EQUIP ] );
	showButton[ SB_SKILL ] = new CTextButton( "Skill" , length , resizePos , NULL );
	uiSystem.addWidget( showButton[ SB_SKILL ] );

	EvtCallBack callback( this , &CGameUI::showFrameEvent );
	UG_ConnectEvent( EVT_UI_BUTTON_CLICK , showButton[ SB_ITEMBAG ]->getID() , callback );
	UG_ConnectEvent( EVT_UI_BUTTON_CLICK , showButton[ SB_EQUIP ]->getID() , callback );
	UG_ConnectEvent( EVT_UI_BUTTON_CLICK , showButton[ SB_SKILL ]->getID() , callback );

	CTalkWidget::sTextFont.init( CUISystem::getInstance().getCFWorld() , TSTR("·s²Ó©úÅé") , 24 , true , false );

	
	miniMap  = new CMiniMapUI( resizePos );
	CUISystem::getInstance().addWidget( miniMap );
	miniMap->setFollowActor( player );

	//TSkillBook& book = player->getSkillBook();

	//char const* skillName[64];
	//int size = book.getAllSkillName( skillName , ARRAY_SIZE( skillName) );

	//for( int i = 0 ; i < size ; ++i )
	//{
	//	fastPlayBar->setButtonFun(i , player , skillName[i] );
	//}

	//skillPanel->show(false);
	//equipPanel->show( false );
	//itemBagPanel->show( false );

	UG_ConnectEvent( EVT_HOT_KEY , EVENT_ANY_ID , EvtCallBack( this , &CGameUI::onHotKey ) );
}

CGameUI::~CGameUI()
{
	UG_DisconnectEvent( EVT_UI_BUTTON_CLICK , EvtCallBack( this , &CGameUI::showFrameEvent ) );
	UG_DisconnectEvent( EVT_HOT_KEY , EvtCallBack( this , &CGameUI::onHotKey ) );
}

void CGameUI::resizeScreenArea( int w , int h )
{
	vitalStateUI->setPos( Vec2i(0,0) );
	fastPlayBar->setPos( Vec2i( ( w - CFastPlayBar::Size.x )/2  , h - CFastPlayBar::Size.y - 20 ) );

	buffBarUI->setPos( Vec2i( ( w - CBuffBarUI::Size.x )/ 2  , 200 ) );

	showButton[ SB_ITEMBAG ]->setPos( Vec2i( w - 140 , h - 50 ) ) ;
	showButton[ SB_EQUIP ]->setPos( Vec2i( w - 140 , h - 50 - 26 ) );
	showButton[ SB_SKILL ]->setPos( Vec2i( w - 140 , h - 50 - 2 * 26 )  );

	int miniMapSize = CMiniMapUI::Length;
	miniMap->setPos( Vec2i( w - miniMapSize , 2 ) );

	talkUI->setPos( Vec2i( ( w - CTalkWidget::Size.x )/ 2 - 100  , 60 )  );

}

void CGameUI::showFrameEvent( TEvent const& event )
{
	if ( event.id == showButton[ SB_SKILL ]->getID() )
		skillPanel->show( !skillPanel->isShow() );
	else if ( event.id == showButton[ SB_ITEMBAG ]->getID() )
		itemBagPanel->show( !itemBagPanel->isShow() );
	else if ( event.id == showButton[ SB_EQUIP ]->getID() )
		equipPanel->show( !equipPanel->isShow() );
}

void CGameUI::changeLevel( CSceneLevel* level )
{
	miniMap->setLevel( level );
}

void CGameUI::onHotKey( TEvent const& event )
{
	unsigned key = event.id;

	if ( key >= CT_FAST_PLAY_1 &&
		 key <= CT_FAST_PLAY_0 )
	{
		int index = key - CT_FAST_PLAY_1;
		getFastPlayBar()->playButton( index );
		return;
	}

	switch ( key )
	{
	case CT_SHOW_EQUIP_PANEL:
		equipPanel->show( true );
		break;
	case CT_SHOW_ITEM_BAG_PANEL:
		itemBagPanel->show( true );
		break;
	case CT_SHOW_SKILL_PANEL:
		skillPanel->show( true );
		break;
	}
}

CBuffBarUI::CBuffBarUI( Vec2i const& pos , CHero* player , Thinkable* thinkable ) 
	:BaseClass( CToolTipUI::DefultTemplate , pos , Size , NULL )
	,m_player( player )
{
	m_progessBar.reset( new CBloodBar( thinkable , mSprite , 100 , Vec3D(1,1,1) , Vec2i( Size.x - 2 , Size.y - 2 ) , Color3f( 1 ,0 ,0 ) ) );

	UG_ConnectEvent(  EVT_SKILL_BUFF_START , m_player->getRefID() , EvtCallBack( this , &CBuffBarUI::onStartBuff ) );
	UG_ConnectEvent(  EVT_SKILL_CANCEL , m_player->getRefID() , EvtCallBack( this , &CBuffBarUI::onCanelSKill ) );
	UG_ConnectEvent(  EVT_SKILL_END , m_player->getRefID() , EvtCallBack( this , &CBuffBarUI::onCanelSKill ) );
}

void CBuffBarUI::onUpdateUI()
{
	if ( isShow() )
	{
		float val = 100 * m_player->getCurBufTime() / bufTime;
		m_progessBar->setLife( val , false );
	}
}

void CBuffBarUI::onStartBuff( TEvent const& event )
{
	TSkill* skill = (TSkill*) event.data;
	bufTime = skill->getInfo().bufTime;

	if ( bufTime != 0 )
	{
		m_progessBar->setLife( 0 , false );
		show( true );
	}
}

void CBuffBarUI::onCanelSKill( TEvent const& event )
{
	show( false );
}

CTalkWidget::CTalkWidget( Vec2i const pos )
	:BaseClass( CToolTipUI::DefultTemplate ,  pos , Size , NULL )
{
	UG_ConnectEvent( EVT_TALK_CONCENT , EVENT_ANY_ID , EvtCallBack( this , &CTalkWidget::receiveTalkStr ) );
	UG_ConnectEvent( EVT_TALK_SECTION_END , EVENT_ANY_ID , EvtCallBack( this , &CTalkWidget::onEvent ) );
	UG_ConnectEvent( EVT_TALK_START , EVENT_ANY_ID , EvtCallBack( this , &CTalkWidget::onEvent ) );
	UG_ConnectEvent( EVT_TALK_END   , EVENT_ANY_ID , EvtCallBack( this , &CTalkWidget::onEvent ) );

	sTextFont.setColor( Color4ub( 255 , 255 , 255 ) );

	m_numLine = 0;
}

void CTalkWidget::onRender()
{
	Vec2i pos = getWorldPos();

	int x = pos.x + 10 + ActorPicSize;
	int y = pos.y + getSize().y - 10 - sTextFont.height;
	int endPos = 0;

	sTextFont.begin();

	int i = ( m_numLine <= MaxLineNum ) ? 0 : m_numLine - MaxLineNum;
	for( ; i < m_numLine ; ++i )
	{
		sTextFont.write( x , y , m_showTalkStr[i] );
		y -= LineOffset;
	}
	sTextFont.end();
}

void CTalkWidget::receiveTalkStr( TEvent const& event )
{
	char const* ptr = (char const*) event.data;

	m_numLine = 1;
	char* cpyPtr = m_showTalkStr[ 0 ];
	*cpyPtr = '\0';
	int num = 0;

	while ( *ptr != '\0' )
	{
		if ( *ptr == '\n' || num >= LineCharNum )
		{
			if ( *ptr == '\n') ++ptr;

			*cpyPtr = '\0';
			cpyPtr = m_showTalkStr[ m_numLine ];
			*cpyPtr = '\0';
			++m_numLine;
			num = 0;
		}
		else if ( *ptr & 0x80 ) //double char
		{
			*cpyPtr++ = *ptr++ ;
			*cpyPtr++ = *ptr++ ;
			++num;
		}
		else
		{
			*cpyPtr++ = *ptr++ ;
			++num;
		}
	}

	*cpyPtr = '\0';

	if  ( m_showTalkStr[ m_numLine - 1 ][0] == '\0' )
		--m_numLine;
}

void CTalkWidget::onEvent( TEvent const& event )
{
	if ( event.type == EVT_TALK_END )
	{
		show( false );
	}
	else if ( event.type == EVT_TALK_START )
	{
		for( int i = 0 ; i < MaxLineNum * 3 ; ++i )
		{
			m_showTalkStr[ i ][ 0 ] = '\0';
		}
		m_numLine = 0;
		show( true );
	}
}