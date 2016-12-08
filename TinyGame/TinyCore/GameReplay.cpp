#include "TinyGamePCH.h"
#include "GameReplay.h"
#include "GameInstance.h"
#include "GameAction.h"

#include <fstream>
#include <vector>


struct ReplayInfoV0_0_1
{
	ReplayInfoV0_0_1()
	{
		gameInfoData = NULL;
		dataSize = 0;
	}

	~ReplayInfoV0_0_1()
	{
		delete [] gameInfoData;
	}
	char      name[8];
	unsigned  gameVersion;
	char*     gameInfoData;
	unsigned  dataSize;

	void     setGameData( size_t size )
	{
		delete [] gameInfoData;
		gameInfoData = new char[ size ];
		dataSize     = size;
	}
};

void ReplayHeader::clear( uint32 version )
{
	version    = version;
	seed       = 0;
	totalSize  = 0;
	totalFrame = 0;
	dataOffset = 0;
}

class IStreamOpteraion
{
public:
	IStreamOpteraion( std::ifstream& fs ):mFS(fs){}
	enum { isInput = 1 , isOutput = 0 };
	template < class T >
	IStreamOpteraion& operator & ( T const& val )
	{
		mFS.read( (char*) &val , sizeof( val ) );
		return *this;
	}

	template < class T >
	IStreamOpteraion& operator & ( std::vector< T >& val )
	{
		size_t num;
		mFS.read( (char*) &num , sizeof( num ) );
		if ( num )
		{
			val.resize( num );
			mFS.read( (char*) &val[0] , std::streamsize( sizeof( T ) * num ) );
		}
		return *this;
	}

	IStreamOpteraion& operator & ( ReplayInfo& info )
	{
		mFS.read( (char*) info.name , sizeof( info.name ) );

		(*this) & info.gameVersion & info.templateVersion;
		mFS.read( (char*) &info.dataSize    , sizeof( info.dataSize ) );
		info.setGameData( info.dataSize );
		mFS.read( (char*) info.gameInfoData    , info.dataSize );
		return *this;
	}

	IStreamOpteraion& operator & ( ReplayInfoV0_0_1& info )
	{
		mFS.read( (char*) info.name , sizeof( info.name ) );
		mFS.read( (char*) &info.gameVersion , sizeof( info.gameVersion ) );
		mFS.read( (char*) &info.dataSize    , sizeof( info.dataSize ) );
		info.setGameData( info.dataSize );
		mFS.read( (char*) info.gameInfoData    , info.dataSize );
		return *this;
	}
private:
	std::ifstream& mFS;
};

class OStreamOpteraion
{
public:
	OStreamOpteraion( std::ofstream& fs ):mFS(fs){}
	enum { isOutput = 1 , isInput = 0 };
	template < class T >
	OStreamOpteraion& operator & ( T const& val )
	{
		mFS.write( (char*) &val , sizeof( val ) );
		return *this;
	}
	template < class T >
	OStreamOpteraion& operator & ( std::vector< T > const& val )
	{
		size_t num = val.size();
		mFS.write( (char*) &num , sizeof( num ) );
		if ( num )
			mFS.write( (char*) &val[0] , std::streamsize( sizeof( T ) * num ) );

		return *this;
	}

	OStreamOpteraion& operator & ( ReplayInfo const& info )
	{
		mFS.write( (char*) info.name , sizeof( info.name ) );

		(*this) & info.gameVersion & info.templateVersion;

		mFS.write( (char*) &info.dataSize    , sizeof( info.dataSize ) );

		if ( info.dataSize )
			mFS.write( (char*) info.gameInfoData    , info.dataSize );

		return *this;
	}

	OStreamOpteraion& operator & ( ReplayInfoV0_0_1 const& info )
	{
		mFS.write( (char*) info.name , sizeof( info.name ) );
		mFS.write( (char*) &info.gameVersion , sizeof( info.gameVersion ) );
		mFS.write( (char*) &info.dataSize    , sizeof( info.dataSize ) );

		mFS.write( (char*) &info.dataSize    , sizeof( info.dataSize ) );

		if ( info.dataSize )
			mFS.write( (char*) info.gameInfoData    , info.dataSize );

		return *this;
	}
private:
	std::ofstream& mFS;
};


bool ReplayBase::loadReplayInfo( char const* path , ReplayHeader& header , ReplayInfo& info )
{
	std::ifstream fs( path , std::ios::binary );
	if ( !fs )
		return false;
	IStreamOpteraion op( fs );
	op & header;
	if ( header.version == VERSION(0,0,1) )
	{
		ReplayInfoV0_0_1 oldInfo;

		op & oldInfo;
		strcpy_s( info.name , oldInfo.name );
		info.gameInfoData    = oldInfo.gameInfoData;
		info.dataSize        = oldInfo.dataSize;
		info.gameVersion     = oldInfo.gameVersion;
		info.templateVersion = VERSION( 0 , 0 , 1 );

		oldInfo.gameInfoData = NULL;
	}
	else
	{
		op & info;
	}
	return true;
}

bool ReplayTemplate::getReplayInfo( ReplayInfo& info )
{
	strcpy_s( info.name , "NO_GAME" );
	info.dataSize = 0;
	info.gameInfoData = 0;
	info.gameVersion = 0;
	info.templateVersion = 0;
	return false;
}

template< class T >
void Replay::serialize( T& op )
{
	op & mHeader;
	assert( mHeader.version >= LastVersion );
	op & mInfo;
	op & mFrameNodeVec & mData;
}


bool Replay::save( char const* path )
{
	std::ofstream fs( path , std::ios::binary );

	if ( !fs )
		return false;

	setupHeader();
	OStreamOpteraion op( fs );
	serialize( op );


	return true;
}

bool Replay::load( char const* path )
{
	clear();
	std::ifstream fs( path , std::ios::binary );
	if ( !fs )
		return false;
	IStreamOpteraion op( fs );
	serialize( op );
	return true;
}


void Replay::recordFrame( long frame , IFrameActionTemplate* temp )
{
	size_t oldPos = mData.size();
	temp->translateData( DataSerializer( *this ) );
	if ( oldPos != mData.size() )
	{
		FrameNode node;
		node.frame = frame;
		node.pos   = ( uint32 )oldPos;

		mFrameNodeVec.push_back( node );
	}
}

bool Replay::advanceFrame( long frame )
{
	while( mNextNodePos < mFrameNodeVec.size() )
	{
		FrameNode& node = mFrameNodeVec[ mNextNodePos ];
		if ( node.frame == frame )
		{
			++mNextNodePos;
			mLoadPos = node.pos;
			return true;
		}
		if ( mFrameNodeVec[ mLoadPos ].frame > frame )
			break;

		++mNextNodePos;
	}

	return false;
}

void Replay::read( void* ptr , size_t num )
{
	if ( mLoadPos + num > mData.size() )
		return;
	memcpy( ptr , &mData[ mLoadPos ], num );
	mLoadPos += num;
}

void Replay::write( void const* ptr , size_t num )
{
	char const* pData = ( char const* ) ptr ;
	mData.insert( mData.end() , pData , pData + num );
}

void Replay::clear()
{
	mData.clear();
	mFrameNodeVec.clear();
	mHeader.clear( LastVersion );
}

void Replay::setupHeader()
{
	mHeader.totalSize = 0 ;
	mHeader.totalSize += sizeof( mHeader );

	mHeader.totalSize += sizeof( mInfo ) - sizeof( void* );
	mHeader.totalSize += mInfo.dataSize;

	mHeader.totalSize += (uint32)mFrameNodeVec.size() * sizeof( FrameNode );
	mHeader.dataOffset = mHeader.totalSize;
	mHeader.totalSize += (uint32)mData.size();
}

void Replay::resetTrackPos()
{
	mLoadPos = 0;
	mNextNodePos = 0;
}

bool Replay::isVaild()
{
	return mFrameNodeVec.size() != 0;
}



ReplayRecorder::ReplayRecorder( IFrameActionTemplate* actionTemp , long& gameFrame ) 
	:mTemplate( actionTemp ) 
	,mGameFrame( gameFrame )
{

}


ReplayRecorder::~ReplayRecorder()
{

}

void ReplayRecorder::start( uint64 seed )
{
	mReplay.clear();
	mReplay.getHeader().seed = seed;
}

void ReplayRecorder::stop()
{
	mReplay.getHeader().totalFrame = mGameFrame;
}

bool ReplayRecorder::save( char const* path )
{
	return mReplay.save( path );
}

void ReplayRecorder::onScanActionStart(bool bUpdateFrame)
{
	mTemplate->prevListenAction();
}

void ReplayRecorder::onFireAction( ActionParam& param )
{
	mTemplate->listenAction( param );
}

void ReplayRecorder::onScanActionEnd()
{
	mReplay.recordFrame( mGameFrame , mTemplate );
}

ReplayInput::ReplayInput( IFrameActionTemplate* actionTemp , long& gameFrame ) 
	:mGameFrame( gameFrame )
	,mTemplate( actionTemp )
{

}

ReplayInput::~ReplayInput()
{

}

bool ReplayInput::scanInput( bool beUpdateFrame )
{
	if ( !beUpdateFrame )
		return false;

	mTemplate->prevCheckAction();
	if ( mReplay.advanceFrame( mGameFrame ) )
	{
		mTemplate->restoreData( DataSerializer( mReplay ) );
	}

	return true;
}

bool ReplayInput::checkAction( ActionParam& param )
{
	return mTemplate->checkAction( param );
}

bool ReplayInput::load( char const* path )
{
	if ( !mReplay.load( path ) )
		return false;
	return true;
}

bool ReplayInput::isPlayEnd()
{
	long totalFrame = mReplay.getHeader().totalFrame;
	return totalFrame < mGameFrame;
}

bool ReplayInput::isVaild()
{
	return mReplay.isVaild();
}

void ReplayInput::restart()
{
	mReplay.resetTrackPos();
}


namespace OldVersion
{
	Replay::Replay()
	{
		mHeader.totalSize = 0;
	}

	template< class T >
	void Replay::serialize( T& op )
	{
		op & mHeader;
		if ( T::isInput && mHeader.version != LastVersion )
		{
			switch( mHeader.version )
			{
			case VERSION(0,0,1):
				{
					ReplayInfoV0_0_1 oldInfo;
					op & oldInfo;

					strcpy_s( mInfo.name , oldInfo.name );
					mInfo.gameInfoData    = oldInfo.gameInfoData;
					mInfo.dataSize        = oldInfo.dataSize;
					mInfo.gameVersion     = oldInfo.gameVersion;
					mInfo.templateVersion = VERSION( 0 , 0 , 1 );

					oldInfo.gameInfoData = NULL;
				}
				break;
			}
		}
		else
		{
			op & mInfo;
		}

		op & mNodeHeaders & mNodeData;
	}

	bool Replay::save( char const* path )
	{
		std::ofstream fs( path , std::ios::binary );

		if ( !fs )
			return false;

		setupHeader();
		OStreamOpteraion op( fs );
		serialize( op );
		return true;
	}

	bool Replay::load( char const* path )
	{
		clear();
		std::ifstream fs( path , std::ios::binary );
		if ( !fs )
			return false;
		IStreamOpteraion op( fs );
		serialize( op );
		return true;
	}

	void Replay::setupHeader()
	{
		mHeader.totalSize = 0 ;
		mHeader.totalSize += sizeof( mHeader );

		mHeader.totalSize += sizeof( mInfo ) - sizeof( void* );
		mHeader.totalSize += mInfo.dataSize;

		mHeader.totalSize += (uint32)mNodeHeaders.size() * sizeof( NodeHeader );
		mHeader.dataOffset = mHeader.totalSize;
		mHeader.totalSize += (uint32)mNodeData.size();
	}

	void Replay::addNode( unsigned frame , unsigned id , void* data )
	{
		NodeHeader node;
		node.id    = id;
		node.frame = frame;
		mNodeHeaders.push_back( node );

		char* pData = ( char*) data;
		mNodeData.insert( mNodeData.end() , pData , pData + mNodeFixSize[ id ] );
	}

	void Replay::addNode( unsigned frame , unsigned id , void* data , size_t size )
	{
		NodeHeader node;
		node.id    = id;
		node.frame = frame;
		mNodeHeaders.push_back( node );

		char* pData = ( char*) data;
		mNodeData.insert( mNodeData.end() , pData , pData + mNodeFixSize[ id ] + size );
	}


	void Replay::clear()
	{
		std::fill_n( mNodeFixSize , MaxRegisterNodeNum , -1 );
		mNodeData.clear();
		mNodeHeaders.clear();
		mHeader.clear( LastVersion );
	}

	bool Replay::isVaild()
	{
		return mNodeHeaders.size() != 0;
	}

	ReplayRecorder::ReplayRecorder( ReplayTemplate* temp , long& gameTime ) 
		:mGameFrame( gameTime )
	{
		mTemplate = temp;
	}

	ReplayRecorder::~ReplayRecorder()
	{
		delete mTemplate;
	}

	void ReplayRecorder::onScanActionStart(bool bUpdateFrame)
{
		mTemplate->startFrame();
	}

	void ReplayRecorder::onFireAction( ActionParam& param )
	{
		if ( !param.beUpdateFrame )
			return;
		mTemplate->listenAction( param );
	}

	void ReplayRecorder::onScanActionEnd()
	{
		mTemplate->recordFrame( mReplay , mGameFrame );
	}

	void ReplayRecorder::start( uint64 seed )
	{
		mReplay.clear();
		mReplay.getHeader().seed = seed;
		mTemplate->registerNode( mReplay );
	}

	void ReplayRecorder::stop()
	{
		mReplay.getHeader().totalFrame = mGameFrame;
	}

	bool ReplayRecorder::save( char const* path )
	{
		return mReplay.save( path );
	}

	ReplayInput::ReplayInput( ReplayTemplate* replayTemp , long& gameFrame ) 
		:mGameFrame( gameFrame )
	{
		assert( replayTemp );
		mTemplate = replayTemp;
	}

	ReplayInput::~ReplayInput()
	{
		delete mTemplate;
	}


	bool ReplayInput::scanInput( bool beUpdateFrame )
	{
		size_t size = mReplay.mNodeHeaders.size();

		if ( mNextIdx >= size )
			return false;
		if ( mReplay.mNodeHeaders[ mNextIdx ].frame > mGameFrame )
			return false;

		mTemplate->startFrame();

		while ( mNextIdx < size && 
			mGameFrame >= mReplay.mNodeHeaders[ mNextIdx ].frame )
		{
			Replay::NodeHeader& node = mReplay.mNodeHeaders[ mNextIdx ];

			mDataOffset += mTemplate->inputNodeData( node.id , &mReplay.mNodeData[ mDataOffset ] );
			mDataOffset += mReplay.mNodeFixSize[ node.id ];
			++mNextIdx;
		}
		return true;
	}

	bool ReplayInput::checkAction( ActionParam& param )
	{
		return mTemplate->checkAction( param );
	}

	void ReplayInput::restart()
	{
		mNextIdx    = 0;
		mDataOffset = 0;
	}

	bool ReplayInput::load( char const* path )
	{
		if ( !mReplay.load( path ) )
			return false;
		mTemplate->registerNode( mReplay );
		return true;
	}

	bool ReplayInput::isVaild()
	{
		return mTemplate && mReplay.isVaild();
	}

	bool ReplayInput::isPlayEnd()
	{
		long totalFrame = mReplay.getHeader().totalFrame;
		return totalFrame < mGameFrame;
	}

}//namespace OldVersion