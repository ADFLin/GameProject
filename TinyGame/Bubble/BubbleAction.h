#ifndef BubbleAction_h__
#define BubbleAction_h__

#include "GameAction.h"
#include "GameNetPacket.h"
#include "BubbleScene.h"

int const gBubbleMaxPlayerNum = 5;

namespace Bubble
{
	class PlayerDataManager;
	struct BuFrameData : public KeyFrameData
	{
		int      mouseOffset;

		template< class OP >
		void serialize( OP& op )
		{
			op & port & keyActBit;
			if ( keyActBit & BIT( ACT_BB_MOUSE_ROTATE ) )
				op & mouseOffset;
		}
	};

	class CFrameActionTemplate : public KeyFrameActionTemplateT< BuFrameData >
	{
		typedef KeyFrameActionTemplateT< BuFrameData > BaseClass;
	public:
		CFrameActionTemplate( PlayerDataManager& manager );
		static unsigned const LastVersion = VERSION( 0,0,1 );

		void  listenAction( ActionParam& param );
		bool  checkAction( ActionParam& param );


		template< class OP >
		void serialize( OP& op )
		{
			op & mNumPlayer;
			for( int i = 0 ; i < mNumPlayer ; ++i )
			{
				BuFrameData& data = mPlayerFrame[i];
				data.serialize( op );
			}
		}

		void firePortAction( ActionTrigger& trigger );

		PlayerDataManager* mManager;
	};

	class CClientFrameGenerator : public CLKeyFrameGeneratorT< BuFrameData >
	{
		typedef CLKeyFrameGeneratorT< BuFrameData > BaseClass;
	public:
		virtual void  onFireAction( ActionParam& param );
		virtual void  generate( DataSerializer & serializer );
	};

	class CServerFrameGenerator : public SVKeyFrameGeneratorT< BuFrameData >
	{
		typedef SVKeyFrameGeneratorT< BuFrameData > BaseClass;
	public:
		CServerFrameGenerator();
		void recvClientData( unsigned pID , DataStreamBuffer& buffer );
	};

}// namespace Bubble
#endif // BubbleAction_h__
