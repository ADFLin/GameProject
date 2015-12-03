#ifndef WorldEdit_h__
#define WorldEdit_h__

#include "common.h"
#include "TGame.h"
#include "TObjectEditor.h"

enum
{
	MODE_EDIT ,
	MODE_RUN  ,
};

DECLARE_GAME( WorldEditor )

class TActor;
class TObjectEditor;

class WorldEditor : public TGame
{
public:
	WorldEditor()
	{
		m_player = NULL;
		m_Mode = MODE_EDIT;
	}

	bool OnInit( WORLDid wID );
	void update(int skip );
	void render( int skip );

	void openScene( char const* name );

	String const&  getMapName() const { return MapName; }

	void setMode( int mode ){ m_Mode = mode; }

protected:
	int      m_Mode;

	String  MapName;
	String  MapPath;

	TActor*  m_player;
	FnObject colShapeObj;

};



#endif // WorldEdit_h__
