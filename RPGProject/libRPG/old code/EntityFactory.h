#include <map>


class IEntityFactoryDictionary
{
public:
	virtual void InstallFactory( IEntityFactory *pFactory, const char *pClassName );
	virtual T* Create( const char *pClassName ) = 0;
	virtual void Destroy( const char *pClassName, T* pNetworkable ) = 0;
};

IEntityFactoryDictionary *EntityFactoryDictionary();

class IEntityFactory
{
public:
	virtual T* Create( const char *pClassName ) = 0;
	virtual void Destroy( T* pNetworkable ) = 0;
};

template <class T>
class CEntityFactory : public IEntityFactory
{
public:
	CEntityFactory( const char *pClassName )
	{
		EntityFactoryDictionary()->InstallFactory( this, pClassName );
	}

	T* Create( const char *pClassName )
	{
		return _CreateEntityTemplate<T>(pClassName);
	}

	void Destroy( T* pNetworkable )
	{
		delete pNetworkable;
	}
};

template< class T >
T* _CreateEntityTemplate( const char *className )
{
	T* newEnt = new T; // this is the only place 'new' should be used!
	//newEnt->PostConstructor( className );
	return newEnt;
}

#define LINK_ENTITY_TO_CLASS(mapClassName,DLLClassName) \
	static CEntityFactory<DLLClassName> mapClassName( #mapClassName );