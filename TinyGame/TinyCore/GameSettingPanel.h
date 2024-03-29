#ifndef GameSettingPanel_h__
#define GameSettingPanel_h__

#include "GameWidget.h"
#include "GameModule.h"

class DataStreamBuffer;

class  BaseSettingPanel : public GPanel
{
	typedef GPanel BaseClass;
public:
	TINY_API BaseSettingPanel(int id, Vec2i const& pos, Vec2i const& size, GWidget* parent);

	TINY_API virtual bool  onChildEvent(int event, int id, GWidget* ui);

	TINY_API void       removeChildWithMask(unsigned mask);
	TINY_API void       adjustChildLayout();
	TINY_API GChoice*   addChoice(int id, char const* title, unsigned groupMask = -1 , int sortOrder = 0);
	TINY_API GButton*   addButton(int id, char const* title, unsigned groupMask = -1, int sortOrder = 0);
	TINY_API GCheckBox* addCheckBox(int id, char const* title, unsigned groupMask = -1, int sortOrder = 0);
	TINY_API GSlider*   addSlider(int id, char const* title, unsigned groupMask = -1, int sortOrder = 0);
	TINY_API GTextCtrl* addTextCtrl(int id, char const* title, unsigned groupMask = -1, int sortOrder = 0);
	template< class Widget >
	Widget* addWidget(int id, char const* title, unsigned groupMask, int sortOrder)
	{
		Widget* ui = new Widget(id, mCurPos, mUISize, this);
		addWidgetInternal(ui, title, groupMask, sortOrder);
		return ui;
	}

	void      setEventCallback(WidgetEventCallBack callback) { mCallback = callback; }

protected:

	TINY_API virtual void renderTitle(GWidget* ui);
	//GListCtrl* addListCtrl( int id , char const* title ){ return addWidget< GListCtrl >( id , title ); }

	TINY_API void   addWidgetInternal(GWidget* ui, char const* title, unsigned groupMask, int sortOrder);
	struct SettingInfo
	{
		std::string   title;
		Vec2i         titlePos;
		WidgetEventCallBack   callback;
		unsigned      mask;
		int           sortOrder;
		GWidget*      ui;
	};

	WidgetEventCallBack  mCallback;
	typedef TArray< SettingInfo > SettingInfoVec;
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
