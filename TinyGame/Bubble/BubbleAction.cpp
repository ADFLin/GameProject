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

	void CClientFrameGenerator::generate( DataSerializer & serializer )
	{
		DataSerializer::WriteOp op( serializer );
		mFrameData.serialize( op );
	}

	CServerFrameGenerator::CServerFrameGenerator() 
		:BaseClass( gBubbleMaxPlayerNum )
	{

	}

	void CServerFrameGenerator::recvClientData( unsigned pID , DataStreamBuffer& buffer )
	{
		BuFrameData data;
		DataSerializer serializer( buffer );
		DataSerializer::ReadOp op( serializer );
		data.serialize( op );

		unsigned dataPos = mPortDataMap[ data.port ];

		mFrameData[ dataPos ].keyActBit |= data.keyActBit;
		if ( data.keyActBit & BIT( ACT_BB_MOUSE_ROTATE ) )
		{
			mFrameData[ dataPos ].mouseOffset = data.mouseOffset;
		}
	}
}//namespace Bubble