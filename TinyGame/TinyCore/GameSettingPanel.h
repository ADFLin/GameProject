#ifndef GameSettingPanel_h__
#define GameSettingPanel_h__

#include "GameWidget.h"
#include "GameInstance.h"

class DataSteamBuffer;

class  GameSettingPanel : public GPanel
{
public:
	GAME_API GameSettingPanel( int id , Vec2i const& pos , Vec2i const& size , GWidget* parent );

	virtual bool  onChildEvent( int event , int id , GWidget* ui );

	GAME_API void      addGame( IGameInstance* game );
	GAME_API void      removeGui( unsigned mask );
	GAME_API void      adjustGuiLocation();
	GAME_API GChoice*   addChoice( int id , char const* title , unsigned groupMask  );
	GAME_API GButton*   addButton( int id , char const* title , unsigned groupMask  );
	GAME_API GCheckBox* addCheckBox( int id , char const* title , unsigned groupMask  );
	GAME_API GSlider*   addSlider( int id , char const* title , unsigned groupMask  );

	void      setEventCallback( EvtCallBack callback ){ mCallback = callback; }
	GAME_API void      setGame( char const* name );
private:

	void renderTitle( GWidget* ui );
	//GListCtrl* addListCtrl( int id , char const* title ){ return addWidget< GListCtrl >( id , title ); }
	template< class Widget >
	Widget* addWidget( int id , char const* title , unsigned groupMask )
	{
		Widget* ui = new Widget( id , mCurPos , mUISize , this );
		addWidgetInternal( ui , title , groupMask );
		return ui;
	}
	GAME_API void   addWidgetInternal( GWidget* ui , char const* title , unsigned groupMask );
	struct SettingInfo
	{
		std::string  title;
		Vec2i        titlePos;
		unsigned     mask;
		GWidget*     ui;
	};

	EvtCallBack  mCallback;
	typedef std::vector< SettingInfo > SettingInfoVec;
	SettingInfoVec mSettingInfoVec;
	Vec2i   mOffset;
	Vec2i   mUISize;
	Vec2i   mCurPos;
	GChoice* mGameChoice;
};


#endif // GameSettingPanel_h__
