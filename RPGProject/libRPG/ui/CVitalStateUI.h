#ifndef CVitalStateUI_h__
#define CVitalStateUI_h__

#include "CUICommon.h"

class CBloodBar;
class AbilityProp;
class Thinkable;

class CVitalStateUI : public CWidget
{
	typedef CWidget BaseClass;
public:
	static int const BloodBarLength = 164;

	CVitalStateUI( Vec2i const& pos , Thinkable* thinkable , AbilityProp* comp );
	void onUpdateUI();
	void onRender();

	CFly::ShuFa*  shufa;
	AbilityProp*  mPropComp;
	CFActor*      m_actor;
	CBloodBar*    m_HPBar;
	CBloodBar*    m_MPBar;
};


#endif // CVitalStateUI_h__