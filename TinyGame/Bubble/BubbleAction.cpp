#include "BubblePCH.h"
#include "BubbleAction.h"

#include "BubbleMode.h"

namespace Bubble
{
	CFrameActionTemplate::CFrameActionTemplate( PlayerDataManager& manager ) 
		:BaseClass( gBubbleMaxPlayerNum )
		,mManager( &manager )
	{

	}


	void CFrameActionTemplate::listenAction( ActionParam& param )
	{
		BaseClass::listenAction( param );
		unsigned pos = mPortDataMap[ param.port ];
		if ( param.act == ACT_BB_MOUSE_ROTATE )
		{
			mFrameData[ pos ].mouseOffset = param.getResult< int >();
		}
	}

	bool CFrameActionTemplate::checkAction( ActionParam& param )
	{
		unsigned pos = mPortDataMap[ param.port ];
		if ( BaseClass::checkAction( param ) )
		{
			if ( param.act == ACT_BB_MOUSE_ROTATE )
			{
				param.getResult< int >() = mFrameData[ pos ].mouseOffset;
			}
			return true;
		}
		return false;
	}

	void CFrameActionTemplate::firePortAction( ActionTrigger& trigger )
	{
		mManager->firePlayerAction( trigger );
	}

	void CClientFrameGenerator::onFireAction( ActionParam& param )
	{
		BaseClass::onFireAction( param );
		if ( param.act == ACT_BB_MOUSE_ROTATE )
		{
			mFrameData.mouseOffset = param.getResult< int >();
		}
	}

	void CClientFrameGenerator::collectFrameData( IStreamSerializer& serializer )
	{
		IStreamSerializer::WriteOp op( serializer );
		mFrameData.serialize( op );
	}

	bool CClientFrameGenerator::haveFrameData(int32 frame)
	{
		if( !BaseClass::haveFrameData(frame) )
			return false;
		return mFrameData.mouseOffset != 0;
	}

	CServerFrameCollector::CServerFrameCollector()
		:BaseClass( gBubbleMaxPlayerNum )
	{

	}

	void CServerFrameCollector::processClientFrameData( unsigned pID , DataSteamBuffer& buffer )
	{
		if( buffer.getAvailableSize() )
		{
			BuFrameData data;
			auto serializer = MakeBufferSerializer(buffer);
			IStreamSerializer::ReadOp op(serializer);
			data.serialize(op);
			if( data.port == ERROR_ACTION_PORT )
			{


			}

			unsigned dataPos = mPortDataMap[data.port];
			mActivePortMask |= BIT(data.port);
			mFrameData[dataPos].keyActBit |= data.keyActBit;
			if( data.keyActBit & BIT(ACT_BB_MOUSE_ROTATE) )
			{
				mFrameData[dataPos].mouseOffset = data.mouseOffset;
			}
		}
	}
}//namespace Bubble