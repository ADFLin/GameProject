#pragma once
#ifndef GoBot_H_18894B79_718A_4F5F_8D24_C6AE9156A7D5
#define GoBot_H_18894B79_718A_4F5F_8D24_C6AE9156A7D5

#include "GoCore.h"

namespace Go
{
	typedef TVector2<int> Vec2i;

	struct GameCommand
	{
		enum
		{
			eStart  ,
			ePass   ,
			eResign ,
			eUndo  ,
			ePlayStone ,
			eAddStone  ,
			eEnd   ,
			eParam ,
		};
		int   id;
		union
		{
			int   color;
			struct
			{
				uint8 paramId;
				union 
				{
					int   intParam;
					float floatParam;
					char const* strParam;
				};
			};
			struct  
			{
				uint8  winner;
				float  winNum;
			};

			struct
			{
				uint8 playColor;
				uint8 pos[2];
			};
		};

		void setParam(uint8 inParamId , int param)
		{
			id = GameCommand::eParam;
			paramId = inParamId;
			intParam = param;
		}

		void setParam(uint8 inParamId, char const* param)
		{
			id = GameCommand::eParam;
			paramId = inParamId;
			strParam = param;
		}

		void setParam(uint8 inParamId, float param)
		{
			id = GameCommand::eParam;
			paramId = inParamId;
			floatParam = param;
		}
	};

	class IGameCommandListener
	{
	public:
		virtual void notifyCommand(GameCommand const& com) = 0;
	};


	class IBotInterface
	{
	public:
		virtual bool initilize(void* settingData) = 0;
		virtual void destroy() = 0;
		virtual bool setupGame(GameSetting const& setting ) = 0;
		virtual bool restart() = 0;
		virtual bool playStone(int x , int y , int color) = 0;
		virtual bool playPass(int color) = 0;
		virtual bool undo() = 0;
		virtual bool thinkNextMove(int color) = 0;
		virtual bool isThinking() = 0;
		virtual void update(IGameCommandListener& listener) = 0;
		virtual bool getMetaData(int id , int dataBuffer[] , int size) { return false; }
	};



}//namespace Go

#endif // GoBot_H_18894B79_718A_4F5F_8D24_C6AE9156A7D5
