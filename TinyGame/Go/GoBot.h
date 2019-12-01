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

		typedef uint32 ParamIdType;
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


	class IBotInterface
	{
	public:
		virtual bool initialize(void* settingData) = 0;
		virtual void destroy() = 0;
		virtual bool setupGame(GameSetting const& setting ) = 0;
		virtual bool restart() = 0;
		virtual bool playStone(int x , int y , int color) = 0;
		virtual bool playPass(int color) = 0;
		virtual bool undo() = 0;
		virtual bool requestUndo() = 0;
		virtual bool thinkNextMove(int color) = 0;
		virtual bool isThinking() = 0;
		virtual void update(IGameCommandListener& listener) = 0;
		virtual bool getMetaData(int id , uint8* dataBuffer , int size) { return false; }
		virtual bool isGPUBased() const { return false; }


		template< class T >
		bool getMetaDataT(int id, T& data)
		{
			return getMetaData(id, (uint8*)&data, sizeof(T));
		}
	};

	class PorxyBot : public IBotInterface
	{
	public:
		PorxyBot(IBotInterface& inBot, bool inbUseInMatch)
			:mBot(&inBot), bUsedInMatch(inbUseInMatch)
		{
		}

		virtual bool initialize(void* settingData) override { return true; }
		virtual void destroy() override {}
		virtual bool setupGame(GameSetting const& setting) override { if( bUsedInMatch ) return true; return mBot->setupGame(setting); }
		virtual bool restart() override { if( bUsedInMatch ) return true; return mBot->restart(); }
		virtual bool playStone(int x, int y, int color) override { if( bUsedInMatch ) return true; return mBot->playStone(x, y, color); }
		virtual bool playPass(int color) override { if( bUsedInMatch ) return true; return mBot->playPass(color); }
		virtual bool undo() override { if( bUsedInMatch ) return true; return mBot->undo(); }
		virtual bool requestUndo() override { if( bUsedInMatch ) return true; return mBot->requestUndo();  }
		virtual bool thinkNextMove(int color) override { return mBot->thinkNextMove(color);  }
		virtual bool isThinking() override { return mBot->isThinking(); }
		virtual void update(IGameCommandListener& listener) override { mBot->update(listener); }
		virtual bool getMetaData(int id, uint8* dataBuffer, int size) { return mBot->getMetaData(id , dataBuffer , size); }
		virtual bool isGPUBased() const { return mBot->isGPUBased(); }

	private:

		IBotInterface* mBot;
		bool bUsedInMatch = false;

	};



}//namespace Go

#endif // GoBot_H_18894B79_718A_4F5F_8D24_C6AE9156A7D5
