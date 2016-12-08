#include "TinyGamePCH.h"
#include "GameSettingPanel.h"

#include "DataSteamBuffer.h"
#include "GameWidgetID.h"

#include "RenderUtility.h"

int const TitleLength = 130;

GameSettingPanel::GameSettingPanel( int id , Vec2i const& pos , Vec2i const& size , GWidget* parent ) 
	:GPanel( id , pos , size , parent )
{
	int borderX = 4;

	Vec2i uiPos( borderX + TitleLength , 5 );
	Vec2i uiSize( size.x - 2 * borderX - TitleLength  , 25 );
	mGameChoice = new GChoice( UI_GAME_CHOICE , uiPos , uiSize , this );

	mUISize = uiSize;
	mOffset = Vec2i( 0 , mUISize.y + 3 );
	mCurPos = uiPos + mOffset;

	setRenderCallback( RenderCallBack::create( this , &GameSettingPanel::renderTitle ) );

}

void   GameSettingPanel::removeGui( unsigned mask )
{
	for( SettingInfoVec::iterator iter = mSettingInfoVec.begin();
		iter != mSettingInfoVec.end() ;  )
	{
		if ( iter->mask & mask )
		{
			iter->ui->destroy();
			iter = mSettingInfoVec.erase( iter );
		}
		else
		{
			++iter;
		}
	}
}

void GameSettingPanel::adjustGuiLocation()
{
	int borderX = 4;

	Vec2i uiPos( borderX + TitleLength , 5 );
	Vec2i uiSize( getSize().x - 2 * borderX - TitleLength  , 25 );

	mUISize = uiSize;
	mOffset = Vec2i( 0 , mUISize.y + 3 );
	mCurPos = uiPos + mOffset;

	for( SettingInfoVec::iterator iter = mSettingInfoVec.begin();
		iter != mSettingInfoVec.end() ; ++iter )
	{
		iter->ui->setPos( mCurPos );
		iter->titlePos = mCurPos - Vec2i( TitleLength , 0 );
		mCurPos += Vec2i( 0 , mUISize.y + 3 );
	}
}

void GameSettingPanel::addWidgetInternal( GWidget* ui , char const* title , unsigned groupMask )
{
	//if ( !mSetInfoVec.empty() )
	//{
	//	SetInfo& info = mSetInfoVec.back();
	//	if ( GListCtrl* ctrl = dynamic_cast< GListCtrl* >( info.ui ) )
	//	{
	//		int sy = ctrl->getItemNum() * ctrl->getItemHeight();
	//		ctrl->setSize( TVec2D( mUISize.x , sy ) );
	//	}
	//}

	SettingInfo info;
	info.title = title;
	info.ui    = ui;
	info.mask  = groupMask;
	info.titlePos = mCurPos - Vec2i( TitleLength , 0 );

	mCurPos += Vec2i( 0 , mUISize.y + 3 );

	mSettingInfoVec.push_back( info );
}


void GameSettingPanel::setGame( char const* name )
{
	int idx = -1;
	if ( name )
		idx = mGameChoice->findItem( name );
	if ( idx == -1 )
		idx = 0;
	mGameChoice->modifySelection( idx );
}

void GameSettingPanel::addGame( IGameInstance* game )
{
	unsigned id = mGameChoice->appendItem( game->getName() );
	mGameChoice->setItemData( id , game );
}

bool GameSettingPanel::onChildEvent( int event , int id , GWidget* ui )
{
	if ( mCallback )
		return ( mCallback )( event , id , ui );

	return true;
}

void GameSettingPanel::renderTitle( GWidget* ui )
{
	int borderX = 4;
	Graphics2D& g = Global::getGraphics2D();

	RenderUtility::setFont( g , FONT_S10 );
	g.setTextColor(255 , 200 , 100 );

	Vec2i uiPos( borderX + 5 , 5 + 3 );
	g.drawText( getWorldPos() + uiPos , LAN("Game Name") );

	for( SettingInfoVec::iterator iter = mSettingInfoVec.begin();
		iter != mSettingInfoVec.end() ; ++iter )
	{
		Vec2i pos = getWorldPos() + iter->titlePos;
		g.drawText( pos + Vec2i(5,3) , iter->title.c_str() );
	}
}

GCheckBox* GameSettingPanel::addCheckBox(int id , char const* title , unsigned groupMask)
{
	Vec2i size( mUISize.y , mUISize.y );
	GCheckBox* ui = new GCheckBox( id , mCurPos + Vec2i( mUISize.x - size.x  , 0 ) , size , this );
	addWidgetInternal( ui , title , groupMask );
	return ui;
}

GSlider* GameSettingPanel::addSlider(int id , char const* title , unsigned groupMask)
{
	GSlider* ui = new GSlider( id , mCurPos + Vec2i( 5 , 5 ) , mUISize.x - 10 - 30 ,  true  , this );
	ui->showValue();
	addWidgetInternal( ui , title , groupMask );
	return ui;
}

GAME_API GChoice* GameSettingPanel::addChoice(int id , char const* title , unsigned groupMask)
{
	return addWidget< GChoice >( id , title , groupMask );
}

GAME_API GButton* GameSettingPanel::addButton(int id , char const* title , unsigned groupMask)
{
	return addWidget< GButton >( id , title , groupMask );
}
