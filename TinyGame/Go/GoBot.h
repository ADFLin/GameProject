#pragma once
#ifndef GoBot_H_18894B79_718A_4F5F_8D24_C6AE9156A7D5
#define GoBot_H_18894B79_718A_4F5F_8D24_C6AE9156A7D5

#include "GoCore.h"

namespace Go
{
	using Vec2i = TVector2<int>;

	enum EBotExecResult
	{
		BOT_OK,
		BOT_FAIL,
		BOT_WAIT,
	};

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
			eBoardState ,
			eParam ,

			eExecResult,
		};
		int   id;

		using ParamIdType = uint32;
		union
		{
			int   color;
			struct
			{
				ParamIdType paramId;
				union 
				{
					int   intParam;
					float floatParam;
					char const* strParam;
					void* ptrParam;
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
			EBotExecResult result;
			int* pBoardData;
		};

		void setParam(ParamIdType inParamId , int param)
		{
			id = GameCommand::eParam;
			paramId = inParamId;
			intParam = param;
		}

		void setParam(ParamIdType inParamId, char const* param)
		{
			id = GameCommand::eParam;
			paramId = inParamId;
			strParam = param;
		}

		void setParam(ParamIdType inParamId, float param)
		{
			id = GameCommand::eParam;
			paramId = inParamId;
			floatParam = param;
		}

		void setParam(ParamIdType inParamId, void* param)
		{
			id = GameCommand::eParam;
			paramId = inParamId;
			ptrParam = param;
		}
	};

	class IGameCommandListener
	{
	public:
		virtual void notifyCommand(GameCommand const& com) = 0;
	};


	class IBot
	{
	public:
		virtual bool initialize(void* settingData) = 0;
		virtual void destroy() = 0;
		virtual bool setupGame(GameSetting const& setting) = 0;
		virtual bool restart(GameSetting const& setting) = 0;
		virtual EBotExecResult playStone(int x , int y , int color) = 0;
		virtual EBotExecResult playPass(int color) = 0;
		virtual EBotExecResult undo() = 0;
		virtual bool requestUndo() = 0;

		virtual bool thinkNextMove(int color) = 0;
		virtual bool isThinking() = 0;
		virtual void update(IGameCommandListener& listener) = 0;
		virtual bool getMetaData(int id , uint8* dataBuffer , int size) { return false; }
		virtual bool isGPUBased() const { return false; }

		
		virtual EBotExecResult readBoard(int* outState) { return BOT_FAIL; }


		template< class T >
		bool getMetaDataT(int id, T& data)
		{
			return getMetaData(id, (uint8*)&data, sizeof(T));
		}
	};

	class PorxyBot : public IBot
	{
	public:
		PorxyBot(IBot& inBot, bool inbUseInMatch)
			:mBot(&inBot), bUsedInMatch(inbUseInMatch)
		{
		}

		bool initialize(void* settingData) override { return true; }
		void destroy() override {}
		bool setupGame(GameSetting const& setting) override { if( bUsedInMatch ) return true; return mBot->setupGame(setting); }
		bool restart(GameSetting const& setting) override { if( bUsedInMatch ) return true; return mBot->restart(setting); }
		EBotExecResult playStone(int x, int y, int color) override { if( bUsedInMatch ) return BOT_OK; return mBot->playStone(x, y, color); }
		EBotExecResult playPass(int color) override { if( bUsedInMatch ) return BOT_OK; return mBot->playPass(color); }
		EBotExecResult undo() override { if( bUsedInMatch ) return BOT_OK; return mBot->undo(); }
		
		bool thinkNextMove(int color) override { return mBot->thinkNextMove(color);  }
		bool isThinking() override { return mBot->isThinking(); }
		void update(IGameCommandListener& listener) override { mBot->update(listener); }
		bool getMetaData(int id, uint8* dataBuffer, int size) override { return mBot->getMetaData(id , dataBuffer , size); }
		bool isGPUBased() const override { return mBot->isGPUBased(); }

		bool requestUndo() override { if (bUsedInMatch) return true; return mBot->requestUndo(); }

		EBotExecResult readBoard(int* outState) override { return mBot->readBoard(outState); }
	private:

		IBot* mBot;
		bool bUsedInMatch = false;

	};



}//namespace Go

#endif // GoBot_H_18894B79_718A_4F5F_8D24_C6AE9156A7D5
