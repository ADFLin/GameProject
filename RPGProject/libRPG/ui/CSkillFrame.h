#ifndef TScalePanel_h__
#define TScalePanel_h__

#include "WidgetCommon.h"
#include "CPlayButton.h"


class SkillState;
class TSkillBook;

class CSkillFrame;

class CSkillButton : public CPlayButton
{
public:
	static int const FPButtonSize = 15;
	CSkillButton( Vec2i const& pos , Vec2i const& size , CSkillFrame* parent );

	CActor* getActor();
	//void render( TFont& font , int i );

};


class CSkillFrame : public CSlotFrameUI
{
	typedef CSlotFrameUI BaseClass;
public:
	static int const CellSize  = 60;
	static int const BoardSize = 40;
	static int const ItemSize  = 36;

	static Vec2i const Size;
	static int const FPButtonNum = 3;

	CSkillFrame( CActor* actor , Vec2i const& pos );
	CActor* getActor(){ return m_actor; }

	void  onRender();
	TSkillBook*   m_book;
	CActor*       m_actor;

	void setButtonFun( int index , CActor* actor , char const* skillName )
	{
		getButton(index)->setFunction( actor , skillName );
	}
	void playButton( int index );
	CSkillButton* getButton( int index ) { return skButtonVec[index]; };

	std::vector< CSkillButton* > skButtonVec;

};
#endif // TSkillPanel_h__