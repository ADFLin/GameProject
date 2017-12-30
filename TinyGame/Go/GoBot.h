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
			eStart = -1,
			ePass = -2,
			eResign = -3,
			eUndo = -4 ,
		};
		union
		{
			int id;
			int pos[2];
		};
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
		virtual bool setupGame(GameSetting const& setting) = 0;
		virtual bool playStone(Vec2i const& pos, int color) = 0;
		virtual bool playPass(int color) = 0;
		virtual bool undo() = 0;
		virtual bool thinkNextMove(int color) = 0;
		virtual bool isThinking() = 0;
		virtual void update(IGameCommandListener& listener) = 0;

	};



}//namespace Go

#endif // GoBot_H_18894B79_718A_4F5F_8D24_C6AE9156A7D5
