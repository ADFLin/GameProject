#ifndef GameInterface_h__
#define GameInterface_h__

#include <exception>

#include "MineDefines.h"

namespace Mine
{

	class ControlException : public std::exception
	{
	public:
		ControlException(char* msg) :std::exception(msg) {}
	};


	enum EGameMode
	{
		GM_BEGINNER,
		GM_INTERMEDITE,
		GM_EXPERT,
	};


	class IMineQuery
	{
	public:
		virtual int  lookCell(int cx, int cy, bool bWaitResult) = 0;
		virtual int  getSizeX() = 0;
		virtual int  getSizeY() = 0;
		virtual int  getBombNum() = 0;

		//helper function
		bool isValidPos(int cx, int cy)
		{
			return 0 <= cx && cx < getSizeX() &&
				   0 <= cy && cy < getSizeY();
		}
	};

	class IMineControl : public IMineQuery
	{
	public:
		virtual int  openCell(int cx, int cy) = 0;
		virtual bool markCell(int cx, int cy) = 0;
		virtual bool unmarkCell(int cx, int cy) = 0;
	};


	enum
	{

	};
	class ISolveStrategy
	{
	public:
		virtual void loadMap(IMineControl& map) = 0;
		virtual void restart(IMineControl& map) = 0;
		virtual bool solveStep() = 0;

		//virtual int  getSettings( int id[] , int value[] ) = 0;
		//virtual bool setSetting( int id , int value ) = 0;
	};


	class IMineControlClient : public IMineControl
	{
	public:
		virtual bool hookGame() = 0;
		virtual void openNeighberCell(int cx, int cy) = 0;
		virtual bool getCellSize(int& sx, int& sy)
		{
			sx = getSizeX();
			sy = getSizeY();
			return true;
		}

		virtual bool setupMode(EGameMode mode) = 0;
		virtual bool setupCustomMode(int sx, int sy, int numbomb) = 0;

		virtual void restart() = 0;
		virtual EGameState checkState() = 0;
	};


}



#endif // GameInterface_h__
