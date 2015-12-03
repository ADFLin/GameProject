#ifndef ReplayStage_h__
#define ReplayStage_h__

#include "GameSingleStage.h"
#include "GameWidget.h"
#include "GameReplay.h"
#include "GameStage.h"


class ReplayEditStage;
typedef std::string String;

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
	void     deleteCurFile();
protected:
	bool     onChildEvent( int event , int id , GWidget* ui );

private:
	
	typedef std::vector< String > StringVec;
	String     mCurDir;
	StringVec  replayFileVec;

	GListCtrl* mFileListCtrl;
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
		 mIsVaildReplay = false;
	 }


protected:
	//StageBase
	virtual bool onInit();
	virtual void onUpdate( long time ){}
	virtual void onEnd(){}
	virtual bool onEvent( int event , int id , GWidget* ui );

	void viewReplay();

	void renderReplayInfo( GWidget* ui );

	bool             mIsVaildReplay;
	ReplayHeader     mReplayHeader;
	ReplayInfo       mGameInfo;
	std::string      mReplayFilePath;
	ReplayListPanel* mRLPanel;
	GButton*         mDelButton;
	GButton*         mViewButton;
	

};


class  GameReplayStage : public GameStage
{
	typedef GameStage BaseClass;
public:

	enum
	{
		UI_REPLAY_TOGGLE_PAUSE = BaseClass::NEXT_UI_ID ,
		UI_REPLAY_RESTART ,
		UI_REPLAY_FAST ,
		UI_REPLAY_SLOW ,

		NEXT_UI_ID ,
	};

	GameReplayStage();

	void setReplayPath( String const& path )
	{
		mReplayFilePath = path;
	}

	LocalPlayerManager* getPlayerManager();

protected:

	bool onInit();
	bool loadReplay( char const* path );
	void onEnd();
	void onUpdate( long time );
	bool onEvent( int event , int id , GWidget* ui );
	void onRestart( uint64& seed );

	TPtrHolder< IReplayInput >     mReplayInput;
	TPtrHolder< LocalPlayerManager >   mPlayerManager;

	std::string   mReplayFilePath;
	int           mIndexSpeed;
	int           mReplaySpeed;
	int           mReplayUpdateCount;
	GSlider*      mProgressSlider;
};

#endif // ReplayStage_h__
