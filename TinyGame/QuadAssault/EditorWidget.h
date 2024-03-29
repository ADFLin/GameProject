#ifndef EditorWidget_h__
#define EditorWidget_h__

#include "Base.h"

#include "GameEdit.h"
#include "GUISystem.h"
#include "Dependence.h"
#include "ObjectFactory.h"
#include "Trigger.h"

enum
{
	UI_PROP_FRAME = UI_EDIT_ID ,

	UI_CREATE_LIGHT ,  
	UI_CREATE_TRIGGER ,
	
	UI_NEW_MAP ,
	UI_SAVE_MAP  ,
	UI_SAVE_AS_MAP ,
	UI_OBJECT_EDIT ,
	UI_TILE_EDIT ,

	UI_TRY_PLAY,

	UI_TILE_SELECT ,
	UI_ACTION_SELECT ,

	UI_OBJECT_DESTROY ,

	UI_OBJECT_LISTCTRL ,
	UI_ACTION_LISTCTRL ,
};



class PorpTextCtrl : public QTextCtrl
{
public:
	PorpTextCtrl( int id , Vec2i const& pos , Vec2i const& size , QWidget* parent );
	~PorpTextCtrl();
	void      inputData();
	void      outputData();
	void      setData( PropData const& data ){ mPorpData = data; }
private:
	PropData mPorpData;
};

class StrPropChioce : public QChoice
{
	typedef QChoice BaseClass;
public:
	StrPropChioce(int id, Vec2i const& pos, Vec2i const& size, QWidget* parent);

	void     init(int numSet, char const* const strSet[]);
	void     inputData();
	void     outputData();
	void     setData(String& str) { mData = &str; }

	String* mData;
};


class IntPropChioce : public QChoice
{
	typedef QChoice BaseClass;
public:
	IntPropChioce( int id , Vec2i const& pos , Vec2i const& size , QWidget* parent );

	void     init( int numSet , int const valueSet[] , char const* const strSet[] );
	void     init(TArrayView< ReflectEnumValueInfo const > valueSet);
	void     inputData();
	void     outputData();
	void     setData( void* data , int dataSize ){ mData = data; mDataSize = dataSize; }

	void*    mData;
	int      mDataSize;
};

class PropFrame : public QFrame
	            , public IPropEditor
{
	typedef QFrame BaseClass;
public:

	PropFrame( int id , Vec2i const& pos , QWidget* parent );

	enum 
	{
		UI_PROP_TEXTCTRL = UI_WIDGET_ID ,
		UI_INT_PROP_CHOICE,
		UI_STR_PROP_CHOICE,
	};

	void   changeEdit( IEditable& obj );
	void   removeEdit();

	void     inputData();
	void     outputData();

	using IPropEditor::addProp;
	virtual void addPropData( char const* name , PropData const& data , unsigned flag );
	virtual void addProp( char const* name , void* value , int sizeValue , int numSet , int const valueSet[] , char const* const strSet[] , unsigned flag );
	virtual void addProp(char const* name, void* value, int sizeValue, TArrayView< ReflectEnumValueInfo const > valueSet, unsigned flag);

	void addPorpWidget( char const* name , QWidget* widget );

	Vec2i getWidgetSize(){ return Vec2i( 130 , 20 ); }
	Vec2i calcWidgetPos();

	virtual bool onChildEvent( int event , int id , QWidget* ui );
	virtual void onRender();

	ObjectCreator* mObjectCreator;
private:
	void     cleanupAllPorp();

	struct PropInfo
	{
		IText*    name;
		QWidget*  widget;
	};
	typedef std::vector< PropInfo >  PropInfoVec;
	PropInfoVec mPorps;
	IEditable*  mEditObj;

};


//#TODO : IMPL
class TileButton : public QButtonBase
{
	typedef QButtonBase BaseClass;




};

class TileEditFrame : public QFrame
{
	typedef QFrame BaseClass;
public:
	TileEditFrame( int id , Vec2f const& pos , QWidget* parent );
	static int const ButtonLength = 40;
};

class ObjectEditFrame : public QFrame
{
	typedef QFrame BaseClass;
public:
	ObjectEditFrame( int id , Vec2f const& pos , QWidget* parent );
	static Vec2i ButtonSize(){ return Vec2i( 90 , 20 ); }
	void setupObjectList( ObjectCreator& creator );

	QListCtrl* mObjectListCtrl;
};


class ActionEditFrame : public QFrame
{
	typedef QFrame BaseClass;

public:

	ActionEditFrame( int id , Vec2i const& pos , QWidget* widget );

	void setTrigger( TriggerBase* trigger );

	static int const ListCtrlWidth = 100; 
	static Vec2i ButtonSize(){ return Vec2i( 90 , 20 ); }
	void setupActionList( ActionCreator& creator );
	void refreshList();


	TriggerBase*  mTrigger;
	QListCtrl*    mListCtrl;
};



#endif // EditorWidget_h__
