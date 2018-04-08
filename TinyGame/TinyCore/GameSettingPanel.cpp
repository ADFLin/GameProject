#include "TinyGamePCH.h"
#include "GameSettingPanel.h"

#include "DataSteamBuffer.h"
#include "GameWidgetID.h"

#include "RenderUtility.h"

int const TitleLength = 130;

BaseSettingPanel::BaseSettingPanel( int id , Vec2i const& pos , Vec2i const& size , GWidget* parent ) 
	:BaseClass( id , pos , size , parent )
{
	int borderX = 4;

	Vec2i uiPos(borderX + TitleLength, 5);
	Vec2i uiSize(size.x - 2 * borderX - TitleLength, 25);

	mUISize = uiSize;
	mOffset = Vec2i(0, 3);
	mCurPos = uiPos + mOffset;
	setRenderCallback( RenderCallBack::Create( this , &BaseSettingPanel::renderTitle ) );
}

void  BaseSettingPanel::removeChildWithMask( unsigned mask )
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

void BaseSettingPanel::adjustChildLayout()
{
	std::sort(mSettingInfoVec.begin(), mSettingInfoVec.end(),
		[](SettingInfo const& lhs, SettingInfo const& rhs) ->bool
		{
			return lhs.sortOrder < rhs.sortOrder;
		});
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

void BaseSettingPanel::addWidgetInternal( GWidget* ui , char const* title , unsigned groupMask , int sortorder )
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
	info.sortOrder = sortorder;
	mCurPos += Vec2i( 0 , mUISize.y + 3 );

	mSettingInfoVec.push_back( info );
}

bool BaseSettingPanel::onChildEvent( int event , int id , GWidget* ui )
{
	if ( mCallback )
		return ( mCallback )( event , id , ui );

	return true;
}

void BaseSettingPanel::renderTitle( GWidget* ui )
{
	int borderX = 4;
	IGraphics2D& g = Global::getIGraphics2D();

	RenderUtility::SetFont( g , FONT_S10 );
	g.setTextColor(255 , 200 , 100 );

	for( SettingInfoVec::iterator iter = mSettingInfoVec.begin();
		iter != mSettingInfoVec.end() ; ++iter )
	{
		Vec2i pos = getWorldPos() + iter->titlePos;
		g.drawText( pos + Vec2i(5,3) , iter->title.c_str() );
	}
}

GCheckBox* BaseSettingPanel::addCheckBox(int id , char const* title , unsigned groupMask , int sortOrder)
{
	Vec2i size( mUISize.y , mUISize.y );
	GCheckBox* ui = new GCheckBox( id , mCurPos + Vec2i( mUISize.x - size.x  , 0 ) , size , this );
	addWidgetInternal( ui , title , groupMask , sortOrder );
	return ui;
}

GSlider* BaseSettingPanel::addSlider(int id , char const* title , unsigned groupMask, int sortOrder)
{
	GSlider* ui = new GSlider( id , mCurPos + Vec2i( 5 , 5 ) , mUISize.x - 10 - 30 ,  true  , this );
	ui->showValue();
	addWidgetInternal( ui , title , groupMask , sortOrder);
	return ui;
}

GTextCtrl* BaseSettingPanel::addTextCtrl(int id, char const* title, unsigned groupMask, int sortOrder)
{
	GTextCtrl* ui = new GTextCtrl(id, mCurPos, mUISize.x, this);
	addWidgetInternal(ui, title, groupMask, sortOrder);
	return ui;
}

GChoice* BaseSettingPanel::addChoice(int id , char const* title , unsigned groupMask, int sortOrder)
{
	return addWidget< GChoice >( id , title , groupMask , sortOrder);
}

GButton* BaseSettingPanel::addButton(int id , char const* title , unsigned groupMask, int sortOrder)
{
	return addWidget< GButton >( id , title , groupMask , sortOrder);
}

GameSettingPanel::GameSettingPanel(int id, Vec2i const& pos, Vec2i const& size, GWidget* parent)
	:BaseClass(id, pos, size, parent)
{
	int borderX = 4;

	Vec2i uiPos(borderX + TitleLength, 5);
	Vec2i uiSize(size.x - 2 * borderX - TitleLength, 25);
	mGameChoice = new GChoice(UI_GAME_CHOICE, uiPos, uiSize, this);

	mUISize = uiSize;
	mOffset = Vec2i(0, mUISize.y + 3);
	mCurPos = uiPos + mOffset;
}

void GameSettingPanel::setGame(char const* name)
{
	int idx = -1;
	if( name )
		idx = mGameChoice->findItem(name);
	if( idx == -1 )
		idx = 0;
	mGameChoice->modifySelection(idx);
}

void GameSettingPanel::renderTitle(GWidget* ui)
{
	int borderX = 4;
	Graphics2D& g = Global::getGraphics2D();

	BaseClass::renderTitle(ui);
	Vec2i uiPos(borderX + 5, 5 + 3);
	g.drawText(getWorldPos() + uiPos, LOCTEXT("Game Name"));
}

void GameSettingPanel::addGame(IGameModule* game)
{
	unsigned id = mGameChoice->addItem(game->getName());
	mGameChoice->setItemData(id, game);
}
