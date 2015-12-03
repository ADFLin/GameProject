#ifndef TSkill_h__
#define TSkill_h__

#include "common.h"
#include "Singleton.h"
#include <vector>
#include <map>

#define SKILL_DATA_PATCH TSTR("Data/skill.dat")
#define SKILL_ICON_DIR   TSTR("Data/UI/Skills/")


enum SkillFlag
{
	SF_SELECT_EMPTY    = BIT(0),
	SF_SELECT_FRIEND   = BIT(1),
	SF_SELECT_POSITION = BIT(2),
	SF_SELECT_TOBJECT  = BIT(3),
	SF_NO_SELECT       = BIT(4),

	SF_SELECT_ENTITY  = SF_SELECT_EMPTY | SF_SELECT_FRIEND | SF_SELECT_TOBJECT,
};

class  ILevelObject;
class  CActor;

struct SkillInfo
{
	//SkillType type;
	int         level;
	float       bufTime;
	int         castMP;
	float       cdTime;
	float       attackRange;
	float       val;
	unsigned    flag;

	float       val2;
};

class TSkillLibrary;
//class FxEntity;
//class TFxData;

class TSkill
{
public:
	TSkill();
	virtual ~TSkill(){}

	char const*  getName(){ return name; }
	char const*  getFullName(){ return fullName; }

	void endBuff();
	virtual void startBuff();
	virtual void onStart( ILevelObject* select );
	virtual void onStart( Vec3D const& pos );
	virtual bool sustainCast(){ return false; }
	virtual void onCancel();
	virtual void setCaster( CActor* actor ){ caster = actor; }

	SkillInfo const& getInfo() const { return *info; }
	
	void  initInfo( int level )
	{
		info = &querySkillInfo( level );
	}
protected:
	virtual SkillInfo const& querySkillInfo( int level ) = 0;
	
	//TFxData* fxData;
	CActor* caster;

	char const* name;
	char const* fullName;
	SkillInfo const* info;
	friend TSkillLibrary;
};

class ISkillFactory
{
public:
	char const*  getName() const{ return m_name; }
	char const*  getIconName()
	{ 
		if ( m_iconName[0] == '\0')
			return NULL;
		return m_iconName.c_str(); 
	}

	char const* getFullName() const 
	{	 
		return reinterpret_cast< TSkill*>( m_skill )->getFullName();	
	}
	virtual TSkill*          create( int level ) = 0;
	virtual SkillInfo const& querySkillInfo(int level) = 0;
	void        setIconName( char const* name ){  m_iconName = name;  }
	
protected:
	static void clientLibrary(ISkillFactory* factory);
	String     m_iconName;
	char const* m_name;
	void*       m_skill;
};

template< class T >
class  CSkillFactory : public ISkillFactory
{
public:
	CSkillFactory( char const* name )
	{
		m_skill = NULL;
		m_name = name;
		ISkillFactory::clientLibrary( this );
	}

	~CSkillFactory()
	{
		delete ((T*)m_skill);
	}
	SkillInfo const& querySkillInfo(int level)
	{ 
		if ( !m_skill )
			m_skill = new T;
		return ((T*)m_skill)->querySkillInfo( level );
	}

	TSkill* create(int level)
	{ 
		TSkill* skill = new T;
		skill->initInfo( level );
		return skill;
	}


};

class TSkillLibrary : public SingletonT<TSkillLibrary>
{
public:
	TSkillLibrary()
	{
	}

	void           init();
	TSkill*        createSkill( char const* name , int level = 1);
	SkillInfo const* querySkill( char const* name , int level );
	char const*    getIconName( char const* name );
	char const*    getFullName( char const* name );
	bool           setIconName( char const* name , char const* iconName );
	bool           saveIconName( char const* patch );
	bool           loadIconName( char const* patch );
	int            getAllSkillName( char const** str , int maxNum );

	void           clear();

protected:
	ISkillFactory* getFactory( char const* name );
	void           initSkill( ISkillFactory* factory );
	typedef std::map< String , ISkillFactory* > SkillMap;
	SkillMap m_skillMap;
};


#define LINK_SKILL_LIBRARY( SkillClass , SkillName )\
	__declspec(dllexport) CSkillFactory< SkillClass > factory_##SkillClass( SkillName );

//#define LINK_SKILL_LIBRARY( SkillClass , SkillName )\
//	void linkFactory_##SkillClass()\
//    {\
//		static CSkillFactory< SkillClass > factory_##SkillClass( SkillName );\
//	}
//
//#define LINK_SKILL( SkillClass )\
//	void linkFactory_##SkillClass();\
//	linkFactory_##SkillClass()




#endif // TSkill_h__