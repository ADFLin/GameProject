#ifndef CTextButton_h__
#define CTextButton_h__

#include "common.h"
#include "CUICommon.h"

struct TextButtonTemplateInfo
{
	char const* left;
	char const* middle;
	char const* right;
};

class CTextButton : public CButtonUI
{
	typedef CButtonUI BaseClass;
public:

	static int const Hight = 25;

	CTextButton( char const* text , int  width , Vec2i const& pos , CWidget* parent );

	virtual void onChangeState( ButtonState state );


	void onRender();
	
	String     m_text;
	unsigned   mIdBtnImage[3];

	static TextButtonTemplateInfo DefultTemplate[];
};



#endif // TTextButton_h__