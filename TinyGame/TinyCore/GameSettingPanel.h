#ifndef GameSettingPanel_h__
#define GameSettingPanel_h__

#include "GameWidget.h"
#include "GameModule.h"

class DataSteamBuffer;

class  BaseSettingPanel : public GPanel
{
	typedef GPanel BaseClass;
public:
	TINY_API BaseSettingPanel(int id, Vec2i const& pos, Vec2i const& size, GWidget* parent);

	virtual bool  onChildEvent(int event, int id, GWidget* ui);

	TINY_API void      removeGui(unsigned mask);
	TINY_API void      adjustGuiLocation();
	TINY_API GChoice*   addChoice(int id, char const* title, unsigned groupMask = -1);
	TINY_API GButton*   addButton(int id, char const* title, unsigned groupMask = -1);
	TINY_API GCheckBox* addCheckBox(int id, char const* title, unsigned groupMask = -1);
	TINY_API GSlider*   addSlider(int id, char const* title, unsigned groupMask = -1);

	void      setEventCallback(EvtCallBack callback) { mCallback = callback; }

protected:

	virtual void renderTitle(GWidget* ui);
	//GListCtrl* addListCtrl( int id , char const* title ){ return addWidget< GListCtrl >( id , title ); }
	template< class Widget >
	Widget* addWidget(int id, char const* title, unsigned groupMask)
	{
		Widget* ui = new Widget(id, mCurPos, mUISize, this);
		addWidgetInternal(ui, title, groupMask);
		return ui;
	}
	TINY_API void   addWidgetInternal(GWidget* ui, char const* title, unsigned groupMask);
	struct SettingInfo
	{
		std::string   title;
		Vec2i         titlePos;
		EvtCallBack   callback;
		unsigned      mask;
		GWidget*      ui;
	};

	EvtCallBack  mCallback;
	typedef std::vector< SettingInfo > SettingInfoVec;
	SettingInfoVec mSettingInfoVec;
	Vec2i   mOffset;
	Vec2i   mUISize;
	Vec2i   mCurPos;
};

class  GameSettingPanel : public BaseSettingPanel
{
	typedef BaseSettingPanel BaseClass;
public:
	TINY_API GameSettingPanel( int id , Vec2i const& pos , Vec2i const& size , GWidget* parent );

	TINY_API void      addGame( IGameModule* game );
	TINY_API void      setGame(char const* name);

	void renderTitle(GWidget* ui);
private:

	GChoice* mGameChoice;
};


#endif // GameSettingPanel_h__
