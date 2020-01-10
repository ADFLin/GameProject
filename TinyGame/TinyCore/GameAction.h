#ifndef GameAction_h__
#define GameAction_h__

#include "DataStreamBuffer.h"
#include "GameNetPacket.h"
#include "GameWorker.h"
#include "Holder.h"
#include "BitUtility.h"

#define DEBUG_SHOW_FRAME_DATA_TRANSITION 0

class IFrameActionTemplate
{
public:
	virtual ~IFrameActionTemplate(){}
	virtual void  firePortAction( ActionPort port, ActionTrigger& trigger ) = 0;
	virtual void  setupPlayer( IPlayerManager& playerManager ) = 0;
	//
	virtual void  prevListenAction() = 0;
	virtual void  listenAction( ActionParam& param ) = 0;
	//
	virtual void  prevRestoreData(bool bhaveData) = 0;
	virtual bool  checkAction( ActionParam& param ) = 0;
	virtual void  postRestoreData(bool bhaveData) = 0;
	virtual void  debugMessage(long frame){}

	virtual bool  haveFrameData(int32 frame) { return true; }

	void collectFrameData(IStreamSerializer& serializer )
	{
		serializeOutputData( serializer );
	}
	void restoreFrameData( IStreamSerializer& serializer , bool bhaveData)
	{
		prevRestoreData(bhaveData);
		if( bhaveData )
		{
			serializeInputData(serializer);
		}
		postRestoreData(bhaveData);
	}
protected:
	virtual void  serializeInputData(IStreamSerializer& serializer ) = 0;
	virtual void  serializeOutputData(IStreamSerializer& serializer ) = 0;
};


template< class T >
class FrameActionHelper : public IFrameActionTemplate
{
	T* _this(){ return static_cast<T*>( this ); }
protected:
	void  serializeInputData(IStreamSerializer& serializer )
	{
		IStreamSerializer::ReadOp op( serializer );
		_this()->serialize( op );
	}
	void  serializeOutputData(IStreamSerializer& serializer )
	{
		IStreamSerializer::WriteOp op( serializer );
		_this()->serialize( op );
	}
	template< class OP >
	void serialize( OP & op ){}
};


struct KeyFrameData
{
	uint32  port;
	uint32  keyActBit;

	bool haveData() const
	{
		return keyActBit != 0;
	}
};


template< class FrameData >
class TKeyFrameActionTemplate : public FrameActionHelper< TKeyFrameActionTemplate< FrameData > >
{
public:
	TKeyFrameActionTemplate( size_t dataMaxNum )
		:mFrameData( new FrameData[ dataMaxNum ] )
		,mPortDataMap( new unsigned[ dataMaxNum ] )
		,mDataMaxNum( dataMaxNum )
	{
		mNumPort = 0;
		mActivePortMask = 0;
	}

	void setupPlayer( IPlayerManager& playerManager )
	{
		unsigned const ERROR_DATA_ID = unsigned(-1);

		for( size_t i = 0 ; i < mDataMaxNum ; ++i )
			mPortDataMap[i] = ERROR_DATA_ID;
		mNumPort = 0;

		for( auto iter = playerManager.createIterator(); iter; ++iter )
		{
			GamePlayer* player = iter.getElement();
			unsigned port = player->getActionPort();
			if ( port != ERROR_ACTION_PORT )
			{
				if ( mPortDataMap[ port ] == ERROR_DATA_ID )
				{
					mPortDataMap[ port ] = (unsigned)mNumPort;
					mFrameData[ mNumPort ].port = port;
					++mNumPort;
				}
			}
		}
	}
	
	void prevListenAction()
	{
		mActivePortMask = 0;
		for(uint32 i = 0 ; i < mNumPort ; ++i )
		{
			mFrameData[i].keyActBit = 0;
		}
	}

	void  listenAction( ActionParam& param )
	{
		mActivePortMask |= BIT(param.port);
		unsigned dataPos = mPortDataMap[ param.port ];
		mFrameData[ dataPos ].keyActBit |= BIT( param.act );
	}

	bool checkAction( ActionParam& param )
	{
		if ( mActivePortMask & BIT(param.port) )
		{
			unsigned dataPos = mPortDataMap[param.port];
			if( mFrameData[dataPos].keyActBit & BIT(param.act) )
				return true;
		}

		return false;
	}

	virtual bool  haveFrameData(int32 frame) { return mActivePortMask != 0; }

	void prevRestoreData(bool bHaveData)
	{
		mNumPort = 0;
		mActivePortMask = 0;
	}

	void postRestoreData(bool bHaveData)
	{
		for(uint32 i = 0; i < mNumPort; ++i )
		{
			mPortDataMap[mFrameData[i].port] = (unsigned)i;
		}
	}

	template< class T >
	void serialize( T& op )
	{
		op & mActivePortMask;
		if( mActivePortMask )
		{
			if ( T::IsSaving )
			{
				for(uint32 i = 0; i < mNumPort; ++i )
				{
					if( mActivePortMask & BIT(mFrameData[i].port) )
					{
						op & mFrameData[i];
					}
				}
			}
			else
			{
				mNumPort = BitUtility::CountSet(mActivePortMask);
				for(uint32 i = 0; i < mNumPort; ++i )
				{
					op & mFrameData[i];
				}
			}
		}
	}


	virtual void firePortAction( ActionPort port, ActionTrigger& trigger ) = 0;


	virtual void debugMessage(long frame)
	{
		FixString< 512 > msg;

		msg.format("%ld =", frame);
		int count = 0;
		for(uint32 i = 0; i < mNumPort; ++i )
		{
			FrameData& data = mFrameData[i];
			if( data.keyActBit )
			{
				FixString< 512 > temp;
				temp.format("(%u %u)" , data.port , data.keyActBit );
				msg += temp;
				++count;
			}
		}
		if( count )
		{
			LogMsg( msg );
		}
	}

protected:
	uint32  mActivePortMask;
	size_t  mDataMaxNum;
	uint32  mNumPort;
	TArrayHolder<FrameData> mFrameData;
	TArrayHolder<unsigned > mPortDataMap;

	template< class FrameActionTemplate >
	friend class TServerKeyFrameCollector;
};


class INetFrameCollector : public ComListener
{
public:
	virtual void  setupAction(ActionProcessor& processer){}
	virtual bool  haveFrameData(int32 frame) { return true; }
	virtual void  collectFrameData(IStreamSerializer & serializer) = 0;

	//for server
	virtual bool  prevProcCommand() override { return true; }
	virtual void  processClientFrameData(unsigned pID, DataStreamBuffer& stream) {}
	virtual void  reflashPlayer(IPlayerManager& playerManager) {}
};


template< class FrameActionTemplate >
class TServerFrameCollector : public  INetFrameCollector
	                        , protected FrameActionTemplate
							, public IActionListener
{
public:
	template< class ...Args >
	TServerFrameCollector(Args ...args) : FrameActionTemplate(std::forward<Args>(args)...) {}

	bool  prevProcCommand() { FrameActionTemplate::prevListenAction();  return true; }
	virtual void  onFireAction(ActionParam& param) override { FrameActionTemplate::listenAction(param); }
	bool  haveFrameData(int32 frame){  return FrameActionTemplate::haveFrameData(frame);  }
	void  collectFrameData(IStreamSerializer& serializer) { FrameActionTemplate::collectFrameData(serializer); }
	virtual void  firePortAction(ActionPort port, ActionTrigger& trigger) override { NEVER_REACH("Don't call this function!"); }
	virtual void  setupAction(ActionProcessor& processer)
	{
		processer.addListener(*this);
	}
};


template< class FrameData >
class TServerKeyFrameCollector : public TServerFrameCollector< TKeyFrameActionTemplate<FrameData > >
{
public:
	TServerKeyFrameCollector( size_t dataMaxNum )
		:TServerFrameCollector< TKeyFrameActionTemplate<FrameData > >( dataMaxNum )
	{

	}

	void processClientFrameData( unsigned pID , DataStreamBuffer& buffer )
	{
		if ( buffer.getAvailableSize() )
		{
			auto serializer = MakeBufferSerializer(buffer);
			FrameData fd;
			serializer.read(fd);
#if DEBUG_SHOW_FRAME_DATA_TRANSITION
			LogDevMsg(0, "Process Client Frame Data : %d %d" , fd.port , fd.keyActBit );
#endif
			unsigned dataPos = mPortDataMap[fd.port];
			mActivePortMask |= BIT(fd.port);
			mFrameData[dataPos].keyActBit |= fd.keyActBit;
		}
	}

	void  reflashPlayer( IPlayerManager& playerManager )
	{
		TServerFrameCollector< TKeyFrameActionTemplate<FrameData > >::setupPlayer( playerManager );
	}
};

template< class FrameData >
class TClientKeyFrameCollector : public INetFrameCollector
	                           , public IActionListener
{
public:
	virtual void  setupAction(ActionProcessor& processer) 
	{
		processer.addListener(*this);
	}

	void onScanActionStart( bool bUpdateFrame )
	{
		mFrameData.port = ERROR_ACTION_PORT;
		mFrameData.keyActBit = 0;
	}
	void onFireAction( ActionParam& param )
	{
		if ( mFrameData.port == ERROR_ACTION_PORT )
			mFrameData.port = param.port;

		assert( mFrameData.port == param.port );
		mFrameData.keyActBit |= BIT( param.act );
	}
	void collectFrameData(IStreamSerializer& serializer )
	{
		if ( mFrameData.port == ERROR_ACTION_PORT )
			return;
		serializer.write( mFrameData );
	}
	FrameData  mFrameData;

	virtual bool haveFrameData(int32 frame) override
	{
		return mFrameData.port != ERROR_ACTION_PORT && mFrameData.keyActBit;
	}

};


typedef TKeyFrameActionTemplate< KeyFrameData >     KeyFrameActionTemplate;
typedef TServerKeyFrameCollector< KeyFrameData >    ServerKeyFrameCollector;
typedef TClientKeyFrameCollector< KeyFrameData >    ClientKeyFrameCollector;

#endif // GameAction_h__
