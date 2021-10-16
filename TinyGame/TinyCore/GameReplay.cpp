#include "TinyGamePCH.h"
#include "GameReplay.h"
#include "GameModule.h"
#include "GameAction.h"

#include "Serialize/FileStream.h"
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

	template< class OP >
	void serialize(OP& op)
	{
		op & name & gameVersion &  dataSize;

		if( OP::IsLoading )
		{
			setGameData(dataSize);
		}
		if( dataSize )
		{
			op & IStreamSerializer::MakeSequence(gameInfoData, dataSize);
		}
	}
};


TYPE_SUPPORT_SERIALIZE_FUNC(ReplayInfoV0_0_1);

void ReplayHeader::clear( uint32 version )
{
	version    = version;
	seed       = 0;
	totalSize  = 0;
	totalFrame = 0;
	dataOffset = 0;
}

bool ReplayBase::LoadReplayInfo( char const* path , ReplayHeader& header , ReplayInfo& info )
{
	InputFileSerializer fs;
	if( !fs.open(path) )
		return false;

	IStreamSerializer::ReadOp op(fs);

	op & header;
	if ( header.version == MAKE_VERSION(0,0,1) )
	{
		ReplayInfoV0_0_1 oldInfo;

		op & oldInfo;
		info.name = oldInfo.name;
		info.gameInfoData    = oldInfo.gameInfoData;
		info.dataSize        = oldInfo.dataSize;
		info.gameVersion     = oldInfo.gameVersion;
		info.templateVersion = MAKE_VERSION( 0 , 0 , 1 );

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
	info.name = "NO_GAME";
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
	op & mInfo;
	op & mFrameNodeVec;
	op & mData;
}

bool Replay::save( char const* path )
{
	OutputFileSerializer fs;
	if ( !fs.open(path) )
		return false;

	setupHeader();

	serialize(IStreamSerializer::WriteOp(fs));
	return fs.isValid();
}

bool Replay::load( char const* path )
{
	clear();
	InputFileSerializer fs;
	if ( !fs.open( path ) )
		return false;

	serialize(IStreamSerializer::ReadOp(fs));
	return fs.isValid();
}


void Replay::recordFrame( long frame , IFrameActionTemplate* actionTemp)
{
	if ( actionTemp->haveFrameData(frame) )
	{
		size_t oldPos = mData.size();
		actionTemp->collectFrameData(*this);
		if( oldPos != mData.size() )
		{
			FrameNode node;
			node.frame = frame;
			node.pos = (uint32)oldPos;

			mFrameNodeVec.push_back(node);
		}
	}
}

bool Replay::advanceFrame( long frame )
{
	while( mNextNodePos < mFrameNodeVec.size() )
	{
		FrameNode& node = mFrameNodeVec[ mNextNodePos ];

		if( node.frame > frame )
			break;

		if ( node.frame == frame )
		{
			++mNextNodePos;
			mLoadPos = node.pos;
			return true;
		}
		else
		{
			LogWarning( 0, "Replay Error");
			++mNextNodePos;
		}
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
	mHeader.version = LastVersion;
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

bool Replay::isValid() const
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
	mPrevFrame = -1;
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
	mbUpdateFrame = bUpdateFrame;
	mTemplate->prevListenAction();
}

void ReplayRecorder::onFireAction( ActionParam& param )
{
	assert(mbUpdateFrame);
	mTemplate->listenAction( param );
}

void ReplayRecorder::onScanActionEnd()
{
	if( mbUpdateFrame )
	{
		if( mPrevFrame == -1 )
		{
			mPrevFrame = mGameFrame;
		}
		else
		{
			assert(mGameFrame - mPrevFrame == 1);
			mPrevFrame = mGameFrame;
		}
		mReplay.recordFrame(mGameFrame, mTemplate);
	}
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

	if( mPrevFrame == -1 )
	{
		mPrevFrame = mGameFrame;
	}
	else
	{
		assert(mGameFrame - mPrevFrame == 1);
		mPrevFrame = mGameFrame;
	}

	bool bHaveData = mReplay.advanceFrame(mGameFrame);
	
	mTemplate->restoreFrameData( mReplay , bHaveData );
	return bHaveData;
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

bool ReplayInput::isValid()
{
	return mReplay.isValid();
}

void ReplayInput::restart()
{
	mReplay.resetTrackPos();
	mPrevFrame = -1;
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
		if ( T::IsLoading && mHeader.version != LastVersion )
		{
			switch( mHeader.version )
			{
			case MAKE_VERSION(0,0,1):
				{
					ReplayInfoV0_0_1 oldInfo;
					op & oldInfo;

					mInfo.name = oldInfo.name;
					mInfo.gameInfoData    = oldInfo.gameInfoData;
					mInfo.dataSize        = oldInfo.dataSize;
					mInfo.gameVersion     = oldInfo.gameVersion;
					mInfo.templateVersion = MAKE_VERSION( 0 , 0 , 1 );

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
		setupHeader();
		OutputFileSerializer fs;
		if( !fs.open(path) )
			return false;

		serialize(IStreamSerializer::WriteOp(fs));
		return true;
	}

	bool Replay::load( char const* path )
	{
		clear();
		InputFileSerializer fs;
		if( !fs.open(path) )
			return false;

		serialize(IStreamSerializer::ReadOp(fs));
		return true;
	}

	void Replay::setupHeader()
	{
		mHeader.version = LastVersion;

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

	bool Replay::isValid()
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
		if ( !param.bUpdateFrame )
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
		if( !beUpdateFrame )
			return false;

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

	bool ReplayInput::isValid()
	{
		return mTemplate && mReplay.isValid();
	}

	bool ReplayInput::isPlayEnd()
	{
		long totalFrame = mReplay.getHeader().totalFrame;
		return totalFrame < mGameFrame;
	}

}//namespace OldVersion