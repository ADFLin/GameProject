#ifndef ReplayStage_h__
#define ReplayStage_h__

#include "GameStageMode.h"
#include "SingleStageMode.h"
#include "GameWidget.h"
#include "GameReplay.h"


class ReplayEditStage;




class ReplayListPanel : public GPanel
{
	typedef GPanel BaseClass;
	enum
	{

	};
public:
	ReplayListPanel( int id , Vec2i const& pos , Vec2i const& size , GWidget* parent );

	String   getFilePath();
	void     loadDir( char const* dir );
	void     deleteSelectedFile();
protected:
	bool     onChildEvent( int event , int id , GWidget* ui );

private:
	
	typedef TArray< String > StringVec;
	String     mCurDir;
	GFileListCtrl* mFileListCtrl;
};


class  ReplayEditStage : public StageBase
{
	typedef StageBase BaseClass;
public:
	enum
	{
		UI_REPLAY_LIST  = BaseClass::NEXT_UI_ID ,
		UI_REPLAYLIST_PANEL ,
		UI_DELETE_REPLAY ,
		UI_VIEW_REPLAY ,
		NEXT_UI_ID ,
	};

	 ReplayEditStage()
	 {
		 mbReplayValid = false;
	 }


protected:
	//StageBase
	virtual bool onInit();
	virtual void onUpdate( long time ){}
	virtual void onEnd(){}
	virtual bool onWidgetEvent( int event , int id , GWidget* ui );

	void viewReplay();

	void renderReplayInfo( GWidget* ui );

	bool             mbReplayValid;
	ReplayHeader     mReplayHeader;
	ReplayInfo       mGameInfo;
	std::string      mReplayFilePath;
	ReplayListPanel* mRLPanel;
	GButton*         mDelButton;
	GButton*         mViewButton;
};

class  ReplayStageMode : public LevelStageMode
	                   , public IGameReplayFeature
{
	typedef LevelStageMode BaseClass;
public:

	enum
	{
		UI_REPLAY_TOGGLE_PAUSE = BaseClass::NEXT_UI_ID,
		UI_REPLAY_RESTART,
		UI_REPLAY_FAST,
		UI_REPLAY_SLOW,

		NEXT_UI_ID,
	};

	ReplayStageMode();

	void setReplayPath(String const& path)
	{
		mReplayFilePath = path;
	}

	LocalPlayerManager* getPlayerManager();
	virtual ReplayStageMode* getReplayMode() override { return this; }
	
protected:
	bool prevStageInit();
	bool postStageInit();
	bool loadReplay(char const* path);
	void onEnd();
	void updateTime(long time);
	bool onWidgetEvent(int event, int id, GWidget* ui);
	void onRestart(uint64& seed);

	TPtrHolder< IReplayInput >         mReplayInput;
	TPtrHolder< LocalPlayerManager >   mPlayerManager;
	IFrameActionTemplate* actionTemplate;

	std::string   mReplayFilePath;
	int           mIndexSpeed;
	int           mReplaySpeed;
	int           mReplayUpdateCount;
	GSlider*      mProgressSlider;
};

#endif // ReplayStage_h__
