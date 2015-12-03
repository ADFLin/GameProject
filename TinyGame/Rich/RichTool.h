#ifndef RichTool_h__
#define RichTool_h__

#include "RichBase.h"

namespace Rich
{
	class PlayerTurn;

	enum ToolType
	{
		TOOL_MISSILE ,
		TOOL_NUCLEAR_MISSILE ,
		TOOL_MINE ,
		TOOL_TIME_BOMB ,
		TOOL_ROMATE_DICE ,
		TOOL_CONVEYOR , 
		TOOL_SCOOTER ,
		TOOL_CAR ,
		TOOL_ROADBLOCK ,
		TOOL_MECHINE_DOLL ,
		TOOL_MECHINE_ROBOT ,
		TOOL_TRUCK ,
		TOOL_TIME_MECHINE ,


		TOOL_NUM ,
	};


	class Tool
	{
	public:
		virtual void use( PlayerTurn& turn ) = 0;
		

		static Tool* FromType( ToolType type );
		static void  init();
		static void  release();
	};



}//namespace Rich

#endif // RichTool_h__
