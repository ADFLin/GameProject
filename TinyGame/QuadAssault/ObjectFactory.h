#ifndef ObjectFactory_h__
#define ObjectFactory_h__

#include "Base.h"
#include "Misc/CStringWrapper.h"

#include <unordered_map>

template< class T >
class IFactoryT
{
public:
	virtual ~IFactoryT(){}
	virtual T* create() const = 0;

	template < class Q >
	class CFactory : public IFactoryT< T >
	{
	public:
		Q* create() const
		{ 
			Q* obj = new Q;
			return obj;
		}
	};

	template < class Q >
	static CFactory< Q >  Make(){ return CFactory< Q >(); }
	template < class Q >
	static CFactory< Q >* Create(){ return new CFactory< Q >(); }
};

template < class BASE >
class TObjectCreator
{
	typedef BASE BaseType;
public:
	typedef IFactoryT< BaseType > FactoryType;
	
	~TObjectCreator()
	{
		cleanup();
	}

	template< class T >
	class CFactory : public FactoryType
	{
	public:
		CFactory( char const* name ):name( name ){}
		T* create() const
		{ 
			T* obj = new T;
			return obj;
		}
		String name;
	};

	BaseType* createObject( char const* name )
	{
		FactoryMap::iterator iter = mNameMap.find( name );
		if ( iter == mNameMap.end() )
			return NULL;
		BaseType* obj = iter->second->create();
		return obj;
	}

	template< class T >
	bool registerClass(){  return registerClass< T >( T::StaticClass()->getName() );  }

	template< class T >
	bool registerClass( char const* name )
	{
		CFactory< T >* factory = new CFactory< T >( name );
		if ( !mNameMap.insert( std::make_pair( factory->name.c_str() , factory ) ).second  )
		{
			delete factory;
			return false;
		}
		return true;
	}

	using FactoryMap = std::unordered_map< TCStringWrapper< char, true >, FactoryType*>;
	FactoryMap& getFactoryMap(){ return mNameMap; }

	void cleanup()
	{
		for (auto const& pair : mNameMap)
		{
			delete pair.second;
		}
		mNameMap.clear();
	}
protected:
	FactoryMap mNameMap;
};

class LevelObject;
class Action;

typedef TObjectCreator< LevelObject > ObjectCreator;
typedef ObjectCreator::FactoryMap     ObjectFactoryMap;

typedef TObjectCreator< Action >      ActionCreator;
typedef ActionCreator::FactoryMap     ActionFactoryMap;

#endif // ObjectFactory_h__
