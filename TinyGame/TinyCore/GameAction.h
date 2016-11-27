#ifndef GameAction_h__
#define GameAction_h__

#include "DataStreamBuffer.h"
#include "GameNetPacket.h"
#include "GameWorker.h"
#include "THolder.h"

class IFrameActionTemplate
{
public:
	virtual ~IFrameActionTemplate(){}
	virtual void  firePortAction( ActionTrigger& trigger ) = 0;
	virtual void  setupPlayer( IPlayerManager& playerManager ) = 0;
	//
	virtual void  prevListenAction() = 0;
	virtual void  listenAction( ActionParam& param ) = 0;
	//
	virtual void  prevCheckAction() = 0;
	virtual bool  checkAction( ActionParam& param ) = 0;

	void translateData( DataSerializer& serializer )
	{
		outputData( serializer );
	}
	void restoreData( DataSerializer& serializer )
	{
		inputData( serializer );
	}
protected:
	//IDataStreamPort
	virtual void  inputData( DataSerializer& serializer ) = 0;
	virtual void  outputData( DataSerializer& serializer ) = 0;
};


template< class T >
class FrameActionHelper : public IFrameActionTemplate
{
	T* _this(){ return static_cast<T*>( this ); }
protected:
	//IDataStreamPort
	void  inputData( DataSerializer& serializer )
	{
		DataSerializer::ReadOp op( serializer );
		_this()->serialize( op );
	}
	void  outputData( DataSerializer& serializer )
	{
		DataSerializer::WriteOp op( serializer );
		_this()->serialize( op );
	}
	template< class OP >
	void serialize( OP & op ){}
};


class INetFrameGenerator : public ActionListener
	                     , public ComListener
{
public:
	//ActionListener
	virtual void  onScanActionStart(){}
	virtual void  onFireAction( ActionParam& param ) = 0;
	virtual void  onScanActionEnd(){}

	virtual void  generate( DataSerializer & serializer ) = 0;

	//for server
	virtual void  prevProcCommand(){}
	virtual void  recvClientData( unsigned pID , DataStreamBuffer& stream ){}
	virtual void  reflashPlayer( IPlayerManager& playerManager ){}
};

#define DEF_SERVER_FRAME_GENERATOR_FUN( ACTION_TEMP )\
	void  prevProcCommand(){  ACTION_TEMP::prevListenAction();  }\
	void  onFireAction( ActionParam& param ){  ACTION_TEMP::listenAction( param );  }\
	void  generate( DataSerializer& serializer ){  ACTION_TEMP::translateData( serializer );  }\
	void  firePortAction( ActionTrigger& trigger ){ assert(  0 && "No Need Call This Fun!");  }

template< class FrameActionTemplate >
class ServerFrameHelper : public  INetFrameGenerator
	                    , protected FrameActionTemplate
{
public:
	ServerFrameHelper(){}
	template< class T1 >
	ServerFrameHelper( T1 t1 ):FrameActionTemplate( t1 ){}
	template< class T1 , class T2 >
	ServerFrameHelper( T1 t1 , T2 t2 ):FrameActionTemplate( t1 , t2 ){}

	DEF_SERVER_FRAME_GENERATOR_FUN( FrameActionTemplate )
};


struct KeyFrameData
{
	unsigned  port;
	unsigned  keyActBit;
};


template< class FrameData >
class KeyFrameActionTemplateT : public FrameActionHelper< KeyFrameActionTemplateT< FrameData > >
{
public:
	KeyFrameActionTemplateT( size_t dataMaxSize )
		:mFrameData( new FrameData[ dataMaxSize ] )
		,mPortDataMap( new unsigned[ dataMaxSize ] )
		,mDataMaxSize( dataMaxSize )
	{

	}

	void setupPlayer( IPlayerManager& playerManager )
	{
		unsigned const ERROR_DATA_ID = unsigned(-1);

		for( size_t i = 0 ; i < mDataMaxSize ; ++i )
			mPortDataMap[i] = ERROR_DATA_ID;
		mNumPort = 0;

		for( IPlayerManager::Iterator iter = playerManager.getIterator(); 
			 iter.haveMore() ; iter.goNext() )
		{
			GamePlayer* player = iter.getElement();
			unsigned port = player->getInfo().actionPort;
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
		for( size_t i = 0 ; i < mNumPort ; ++i )
		{
			mFrameData[i].keyActBit = 0;
		}
	}
	void  listenAction( ActionParam& param )
	{
		unsigned dataPos = mPortDataMap[ param.port ];
		mFrameData[ dataPos ].keyActBit |= BIT( param.act );
	}

	void prevCheckAction()
	{
		for( size_t i = 0 ; i < mNumPort ; ++i )
		{
			mPortDataMap[ mFrameData[i].port ] = (unsigned)i;
		}
	}

	bool checkAction( ActionParam& param )
	{
		unsigned dataPos = mPortDataMap[ param.port ];
		if (  mFrameData[ dataPos ].keyActBit & BIT( param.act ) )
			return true;

		return false;
	}

	template< class T >
	void serialize( T& op )
	{
		op & mNumPort;
		for( size_t i = 0 ; i < mNumPort ; ++i )
		{
			op & mFrameData[i];
		}
	}

	virtual void firePortAction( ActionTrigger& trigger ) = 0;

protected:
	size_t  mDataMaxSize;
	size_t  mNumPort;
	TArrayHolder<FrameData> mFrameData;
	TArrayHolder<unsigned > mPortDataMap;
};


template< class FrameData >
class SVKeyFrameGeneratorT : public ServerFrameHelper< KeyFrameActionTemplateT<FrameData > >
{
public:
	SVKeyFrameGeneratorT( size_t dataMaxSize )
		:ServerFrameHelper< KeyFrameActionTemplateT<FrameData > >( dataMaxSize ){}

	void recvClientData( unsigned pID , DataStreamBuffer& buffer )
	{
		DataSerializer serializer( buffer ); 
		KeyFrameData fd;
		serializer.read( fd );

		unsigned dataPos = mPortDataMap[ fd.port ];
		mFrameData[ dataPos ].keyActBit |= fd.keyActBit;
	}
	void  reflashPlayer( IPlayerManager& playerManager )
	{
		KeyFrameActionTemplateT< FrameData >::setupPlayer( playerManager );
	}
};

template< class FrameData >
class CLKeyFrameGeneratorT : public INetFrameGenerator
{
public:
	void onScanActionStart()
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
	void generate( DataSerializer& serializer )
	{
		if ( mFrameData.port == ERROR_ACTION_PORT )
			return;
		serializer.write( mFrameData );
	}
	FrameData  mFrameData;
};


typedef KeyFrameActionTemplateT< KeyFrameData > KeyFrameActionTemplate;
typedef SVKeyFrameGeneratorT< KeyFrameData >    SVKeyFrameGenerator;
typedef CLKeyFrameGeneratorT< KeyFrameData >    CLKeyFrameGenerator;

#endif // GameAction_h__
