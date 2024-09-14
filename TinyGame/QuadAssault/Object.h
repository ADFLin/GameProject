#ifndef Object_h__
#define Object_h__

#include "Base.h"
#include "DataStructure/IntrList.h"
#include "GameEdit.h"
#include "ClassReflection.h"

class RHIGraphics2D;

inline Vec2f GetDirection(float angle)
{
	Vec2f dir;
	Math::SinCos(angle, dir.y, dir.x);
	return dir;
}

class Object : public CRObject
{
public:
	Object();
	Object( Vec2f const& pos );
	virtual ~Object(){}

	Vec2f const&  getPos() const { return mPos; }
	void          setPos(Vec2f const& pos){ mPos = pos; }

protected:
	Vec2f mPos;
};

class Level;

enum ObjectType
{
	OT_OBJECT ,
	OT_ACTOR ,
	OT_BULLET ,
	OT_EXPLOSION ,
	OT_LIGHT ,
	OT_MOB ,
	OT_PLAYER ,
	OT_ITEM ,
	OT_PARTICLE ,
	OT_TRIGGER ,
	OT_PLAYER_START,
};

class IObjectRenderer;
struct Tile;
class ColBody;

class ObjectClass : public CRClass
{
public:
	ObjectClass( ObjectClass* inParent , char const* inName , ObjectType inType )
		:CRClass( inParent , inName ),type( inType )
	{

	}

	ObjectType  getType(){ return type; }

private:
	ObjectType   type;
};


enum DevDrawMode
{
	DDM_EDIT ,
	DDM_COLLISION ,
};

enum SpawnDestroyFlag
{
	SDF_SETUP_DEFAULT = BIT(0),
	SDF_CAST_EFFECT   = BIT(1),
	SDF_LOAD_LEVEL    = BIT(2),
	SDF_EDIT          = BIT(3),
};

class LevelObject : public Object
	              , public IEditable
{
	typedef Object BaseClass;
public:
	LevelObject();
	LevelObject( Vec2f const& pos );

	static  ObjectClass* StaticClass();
	ObjectType   getType(){ return getClass()->getType(); }
	virtual ObjectClass* getClass(){ return StaticClass(); }
	virtual void enumProp( IPropEditor& editor );

	virtual void init(){}
	virtual void onSpawn( unsigned flag )
	{

	}
	virtual void onDestroy( unsigned flag ){}
	virtual void tick(){}
	virtual void postTick(){}
	virtual void updateRender( float dt ){}
	
	virtual void renderDev(RHIGraphics2D& g, DevDrawMode mode ){}

	virtual void onTileCollision( ColBody& self , Tile& tile ){}
	virtual void onBodyCollision( ColBody& self , ColBody& other ){}

	virtual IObjectRenderer* getRenderer(){ return nullptr; }

	Level* getLevel(){ return mLevel; }
	void   destroy(){ mNeedDestroy = true; }

	template< class T >
	T* cast()
	{ 
		if ( getClass()->isChildOf( T::StaticClass() ) )
			return static_cast< T* >( this );
		return nullptr;
	}
	template< class T >
	T* castChecked()
	{ 
		CHECK(getClass()->isChildOf(T::StaticClass()));
		return static_cast< T* >( this );
	}

	Vec2f        getRenderPos() const { return mPos - mSize / 2; }
	Vec2f const& getSize() const { return mSize; }
	void         setSize(Vec2f const& size ){ mSize = size; }

	void         calcBoundBox( BoundBox& bBox );


	//Edit
	bool    isTransient() const { return mbTransient; }

protected:
	Vec2f    mSize;
	Level*   mLevel;
	bool     mNeedDestroy;
	bool     mbTransient = true;

private:
	friend class Level;
	LinkHook baseHook;
	LinkHook typeHook;

private:
	friend class RenderEngine;
	friend class IObjectRenderer;
	LevelObject* renderLink;

	REFLECT_STRUCT_BEGIN(LevelObject)
		REF_PROPERTY(mPos, "Pos")
	REFLECT_STRUCT_END()
};


#define DECLARE_OBJECT_CLASS( CLASS , BASE )\
private:\
	typedef CLASS ThisClass;\
	typedef BASE  BaseClass;\
public:\
	static ObjectClass* StaticClass();\
	virtual ObjectClass* getClass(){ return ThisClass::StaticClass(); }\
	virtual void enumProp( IPropEditor& editor )\
	{\
		ClassEditReigster reg( static_cast<ThisClass*>(this), editor );\
		REF_COLLECT_TYPE(ThisClass, reg );\
	}

#define IMPL_OBJECT_CLASS( CLASS , TYPE , NAME )\
	ObjectClass* CLASS::StaticClass()\
	{\
		static ObjectClass myClass( BaseClass::StaticClass() , NAME , TYPE );\
		return &myClass;\
	}


#endif // Object_h__

