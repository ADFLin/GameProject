#include "RichPCH.h"
#include "RichTool.h"

#include "RichPlayerTurn.h"

namespace Rich
{
	class RoadBlockActor : public ActorComp
	{



	};

	class RomateDice : public Tool
	{
	public:
		virtual void use( PlayerTurn& turn )
		{
			ActionReqestData data;
			data.numDice = 1;
			turn.queryAction( REQ_ROMATE_DICE_VALUE , data );
		}

	};

	static Tool* gToolMap[ TOOL_NUM ];

	void Tool::Initialize()
	{
		for ( int i = 0 ; i < TOOL_NUM ; ++i )
			gToolMap[i] = nullptr;

		gToolMap[ TOOL_ROADBLOCK ] = new RomateDice;
	}

	void Tool::Release()
	{
		for ( int i = 0 ; i < TOOL_NUM ; ++i )
		{
			delete gToolMap[i];
		}
	}

	Tool* Tool::FromType( ToolType type )
	{
		return gToolMap[ type ];
	}

}//namespace Rich