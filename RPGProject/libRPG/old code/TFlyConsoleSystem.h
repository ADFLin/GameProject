#ifndef TFlyConsoleSystem_h__
#define TFlyConsoleSystem_h__

#include "common.h"
#include "ConsoleSystem.h"

DEF_TYPE_PARAM( Vec3D , "%f%f%f" )
DEF_TYPE_PARAM( Quat ,  "%f%f%f%f" )

class TFlyConsoleSystem : public ConsoleSystem
{
public:
	TFlyConsoleSystem()
	{
		pevStrIndex = 0;
		findStrNum = 0;
	}

	void pushChar( char c );
	void popChar();

	void setFindCommand(int offset);
	void setSaveString(int offset);
	void doCommand();
	TString const& getInputStr() const { return inputStr; }

protected:

	TString inputStr;
	int findStrNum;
	int pevStrIndex;
	int saveStrIndex;
	int findStrIndex;
	TString saveStr[10];
	char const* findStr[10];
};
#endif // TFlyConsoleSystem_h__