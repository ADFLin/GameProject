#ifndef GameReplay_h__
#define GameReplay_h__

#include "GameGlobal.h"
#include "GameControl.h"
#include "DataStream.h"
#include <vector>
#include "Flag.h"

#define  REPLAY_SUB_FILE_NAME ".rpf"

#include <vector>
#include <string>

#include "FixString.h"

class IFrameActionTemplate;

struct ReplayInfo
{
	ReplayInfo()
	{
		gameInfoData = NULL;
		dataSize = 0;
	}

	~ReplayInfo()
	{
		delete [] gameInfoData;
	}

	FixString< 8 > name;
	//char      name[8];
	unsigned  gameVersion;
	unsigned  templateVersion;
	char*     gameInfoData;
	size_t    dataSize;

	void     setGameData( size_t size )
	{
		delete [] gameInfoData;
		gameInfoData = new char[ size ];
		dataSize     = size;
	}
};

struct ReplayHeader
{
	uint32   version;
	uint64   seed;
	uint32   totalSize;
	uint32   totalFrame;
	uint32   dataOffset;
	void     clear( uint32 version );
};

class ReplayBase
{
public:

	ReplayHeader const& getHeader() const { return mHeader;  }
	ReplayHeader&       getHeader()       { return mHeader;  }
	ReplayInfo&         getInfo()         { return mInfo;    }

	static TINY_API bool loadReplayInfo( char const* path , ReplayHeader& header , ReplayInfo& info  );
protected:

	union
	{
		ReplayHeader  mHeader;
	};
	ReplayInfo    mInfo;
};

class IReplayRecorder : public IActionListener
{
public:
	virtual ~IReplayRecorder(){}
	virtual void        start( uint64 seed ) = 0;
	virtual void        stop() = 0;
	virtual bool        save( char const* path ) = 0;
	virtual ReplayBase& getReplay() = 0;
};

class IReplayInput : public IActionInput
{
public:
	virtual ~IReplayInput(){}
	virtual void          restart() = 0;
	virtual bool          is() = 0;
	virtual bool          isPlayEnd() = 0;
	virtual bool          load( char const* path ) = 0;
	virtual ReplayBase&   getReplay() = 0;

	long    getRecordFrame(){  return  getReplay().getHeader().totalFrame;  }
	uint64  getSeed()       {  return  getReplay().getHeader().seed;  }
};


class  Replay : public ReplayBase 
	          , public DataStream
{
public:
	static uint32 const LastVersion = MAKE_VERSION(0,1,0);

	bool save( char const* path );
	bool load( char const* path );

	void recordFrame( long frame , IFrameActionTemplate* temp  );
	bool advanceFrame( long frame );

	void read( void* ptr , size_t num );
	void write( void const* ptr , size_t num );
	void clear();

	void resetTrackPos();
	bool is();

protected:

	template< class T >
	void serialize( T& op );

	void setupHeader();
	struct FrameNode
	{
		int32  frame;
		uint32 pos;
	};
	std::vector< FrameNode > mFrameNodeVec;
	std::vector< char >      mData;
	size_t      mNextNodePos;
	size_t      mLoadPos;
};

class  ReplayRecorder : public IReplayRecorder
{
public:
	TINY_API ReplayRecorder( IFrameActionTemplate* actionTemp  , long& gameFrame );
	TINY_API ~ReplayRecorder();

	virtual void start( uint64 seed );
	virtual void stop();
	virtual bool save( char const* path );
	virtual Replay& getReplay(){ return mReplay;  }

protected:

	void onScanActionStart( bool bUpdateFrame );
	void onFireAction( ActionParam& param );
	void onScanActionEnd();

	IFrameActionTemplate* mTemplate;
	Replay      mReplay;
	long&       mGameFrame;

};

class  ReplayInput : public IReplayInput
{
public:
	TINY_API ReplayInput( IFrameActionTemplate* actionTemp , long& gameFrame );
	TINY_API ~ReplayInput();

	void    restart();
	bool    is();
	bool    isPlayEnd();
	bool    load( char const* path );
	bool    scanInput( bool beUpdateFrame );
	bool    checkAction( ActionParam& param );
	Replay& getReplay(){ return mReplay;  }
	
protected:
	IFrameActionTemplate* mTemplate;
	Replay mReplay;
	long&  mGameFrame;
};


class ReplayTemplate;

namespace OldVersion
{
	class ReplayInput;
	class ReplayRecorder;
	class Replay : public ReplayBase
	{
	public:
		static uint32 const LastVersion = MAKE_VERSION(0,0,2);
		TINY_API Replay();

		bool   is();
		bool   save( char const* path );
		bool   load( char const* path );

		void   registerNode( unsigned id , unsigned size ){   mNodeFixSize[id] = size;  }
		TINY_API void  addNode( unsigned frame , unsigned id , void* data );
		TINY_API void  addNode( unsigned frame , unsigned id , void* data , size_t size );
		void   clear();

	private:
		struct NodeHeader
		{
			uint32   id    : 8;
			int32    frame : 24;
		};

		void setupHeader();

		static int const MaxRegisterNodeNum = 8;
		int     mNodeFixSize[ MaxRegisterNodeNum ];

		typedef std::vector< NodeHeader > NodeHeaderVec;
		NodeHeaderVec mNodeHeaders;
		typedef std::vector< char > NodeData;
		NodeData    mNodeData;



		friend class ReplayInput;
		friend class ReplayRecorder;

		template< class T >
		void serialize( T& op );

	};

	class  ReplayRecorder : public IReplayRecorder
	{
	public:
		TINY_API ReplayRecorder( ReplayTemplate* temp , long& gameTime );
		TINY_API ~ReplayRecorder();
		Replay& getReplay(){ return mReplay; }

		void start( uint64 seed );
		void stop();
		bool save( char const* path );

		ReplayTemplate* getTemplate(){ return mTemplate; }


		//ActionLister
		virtual void onScanActionStart( bool bUpdateFrame );
		virtual void onFireAction( ActionParam& param );
		virtual void onScanActionEnd();

		long&           mGameFrame;
		ReplayTemplate* mTemplate;
		Replay          mReplay;
	};


	class  ReplayInput : public IReplayInput
	{
	public:
		TINY_API ReplayInput( ReplayTemplate* replayTemp , long& gameFrame );
		TINY_API ~ReplayInput();
		void     restart();
		bool     isPlayEnd();
		bool     load( char const* path );
		bool     is();

		Replay&   getReplay(){  return mReplay;  }
	private:

		virtual bool scanInput( bool beUpdateFrame );
		virtual bool checkAction( ActionParam& param );


		ReplayTemplate* mTemplate;
		unsigned   mNextIdx;
		size_t     mDataOffset;
		long&      mGameFrame;
		Replay     mReplay;
	};

}//namespace OldVersion


class ReplayTemplate
{
public:
	virtual TINY_API bool  getReplayInfo( ReplayInfo& info );

	virtual void   registerNode( OldVersion::Replay& replay ) = 0;
	virtual void   startFrame() = 0;
	virtual size_t inputNodeData( unsigned id , char* data ) = 0;
	virtual bool   checkAction( ActionParam& param ) = 0;
	virtual void   listenAction( ActionParam& param ) = 0;
	virtual void   recordFrame( OldVersion::Replay& replay , unsigned frame ) = 0;
};


#endif // GameReplay_h__