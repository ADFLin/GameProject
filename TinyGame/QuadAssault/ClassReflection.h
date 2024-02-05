#ifndef ClassReflection_h__
#define ClassReflection_h__

#include "ReflectionCollect.h"
#include "DataStructure/ClassTree.h"

enum ClassTypeFlag
{




};

enum ClassPropFlag
{
	CPF_COLOR  = BIT(0) ,
	CPF_NET    = BIT(1) ,
};

enum PropType
{
	PROP_NONE ,
	PROP_INT ,
	PROP_UINT ,
	PROP_FLOAT ,
	PROP_DOUBLE ,
	PROP_BOOL  ,
	PROP_VEC3F ,
	PROP_VEC2I ,
	PROP_VEC2F ,
	PROP_COLOR ,
	PROP_STRING ,
	PROP_OBJECT ,
	PROP_CTRL   ,

	PROP_CLASSNAME,

	PROP_CARRAY  = 0x100,
	PROP_CVECTOR = 0x200,
	PROP_CLIST   = 0x300,
	
	PROP_CONTAINER_MASK = 0xf00 ,
};

class CRClass : public ClassTreeNode
{
public:
	CRClass( CRClass* inParent , char const* inName )
		:ClassTreeNode(GetTree(), inParent)
		,name( inName )
	{

	}

	char const* getName(){ return name; }
	char const*  name;

	static ClassTree& GetTree()
	{
		static ClassTree sTree;
		return sTree;
	}
};

class CRObject
{
public:
	virtual ~CRObject(){}
	virtual CRClass*   getClass(){ return NULL; }
};

class CRClassName
{
public:
	String  name;
};

#endif // ClassReflection_h__