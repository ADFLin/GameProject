#ifndef TDataMap_h__
#define TDataMap_h__

class dataFileIO;
class dataFieldBase
{
public:
	virtual void save( dataFileIO& file , void* ptr ) = 0;
	virtual void load( dataFileIO& file , void* ptr ) = 0;
};

struct false_type{};
struct true_type{};

template < class T >
struct HaveDataMap
{
	typedef false_type result_type;
};


struct dataMap_t
{
	dataFieldBase**   dataFieldArray;
	size_t            numDataField;
	dataMap_t*        baseDataMap;

	~dataMap_t()
	{
		for( size_t i = 0 ; i < numDataField ; ++ i )
			delete dataFieldArray[i];
	}
};


template < class T > 
void loadDataMap(dataFileIO& file , T* ptr )
{
	dataMap_t& dataMap = T::getDataMap();
	loadDataMap( file , dataMap , ptr );
}

template < class T > 
void saveDataMap(dataFileIO& file , T* ptr )
{
	dataMap_t& dataMap = T::getDataMap();
	saveDataMap( file , dataMap , ptr );
}


void saveDataMap( dataFileIO& file , dataMap_t& dataMap , void* ptr );
void loadDataMap( dataFileIO& file , dataMap_t& dataMap , void* ptr );

template < class T  >
class dataField_t : public dataFieldBase
{
public:
	dataField_t( size_t offset )
		:m_offset( offset )
	{

	}
	
	void save( dataFileIO& file , void* ptr )
	{ 
		save( file , ptr , HaveDataMap<T>::result_type() );
	}

	void load( dataFileIO& file , void* ptr )
	{
		load( file , ptr , HaveDataMap<T>::result_type() );
	}

	void save( dataFileIO& file , void* ptr , false_type )
	{
		file.write( (T*)((char*)ptr + m_offset ) );
	}
	void save( dataFileIO& file , void* ptr , true_type )
	{
		saveDataMap( file , (T*)((char*)ptr + m_offset ) );
	}

	void load( dataFileIO& file , void* ptr , false_type )
	{
		file.read( (T*)((char*)ptr + m_offset ) );
	}

	void load( dataFileIO& file , void* ptr , true_type )
	{
		loadDataMap( file , (T*)((char*)ptr + m_offset ) );
	}
	
	size_t m_offset;
};


template < class T  >
class dataFieldArray_t : public dataField_t<T>
{
public:
	dataFieldArray_t( size_t offset , size_t num )
		: dataField_t( offset )
		, m_num( num )
	{

	}

	void save( dataFileIO& file , void* ptr )
	{ 
		save( file , ptr  , HaveDataMap<T>::result_type() );
	}

	void load( dataFileIO& file , void* ptr )
	{
		load( file , ptr , HaveDataMap<T>::result_type() );
	}

	void save( dataFileIO& file , void* ptr , false_type )
	{
		file.write( (T*)((char*)ptr + m_offset )  , m_num );
	}
	void save( dataFileIO& file , void* ptr , true_type )
	{
		T* dest = (T*)((char*)ptr + m_offset );
		for (size_t i = 0; i < m_num ; ++i )
			saveDataMap( file , dest + i );
	}

	void load( dataFileIO& file , void* ptr , false_type )
	{
		file.read( (T*)((char*)ptr + m_offset )  , m_num );
	}

	void load( dataFileIO& file , void* ptr , true_type )
	{
		T* dest = (T*)((char*)ptr + m_offset );
		for (size_t i = 0; i < m_num ; ++i )
			loadDataMap( file , dest + i );
	}

	size_t m_num;
};

template < class T >
class dataFieldFun : public dataFieldBase
{
	typedef void (*FunType)( dataFileIO& , T* ptr );
	dataFieldFun( FunType saveFun  , FunType loadFun )
		: m_saveFun( saveFun )
		, m_loadFun( loadFun )
	{

	}

	void save( dataFileIO& file , void* ptr )
	{ 
		(*m_saveFun)( file , (T*) ptr );
	}

	void load( dataFileIO& file , void* ptr )
	{
		(*m_loadFun)( file , (T*) ptr );
	}

	FunType m_loadFun;
	FunType m_saveFun;
};


#define MEMBEROFFSET(s,m)	(size_t)&(((s *)0)->m)

template <class T ,class Type >
static dataFieldBase* makeDataField( size_t offset , Type (T::*) )
{
	return new dataField_t<Type>( offset );
}

template <class T ,class Type , size_t Num >
static dataFieldBase* makeDataField( size_t offset , Type (T::*)[Num] )
{
	return new dataFieldArray_t<Type>( offset , Num );
}

template <class T >
static dataFieldBase* makeDataFieldFun( void (*save)( dataFileIO& , T* ) ,
									    void (*load)( dataFileIO& , T* ) )
{
	return new dataFieldFun< T >( save , load );

}

template< class T >
struct IntiDataMapHelper
{
	IntiDataMapHelper()
	{
		dataMap_t& dataMap = T::getDataMap();
		dataMap.numDataField = sizeof( T::m_dataFieldArray ) / sizeof( void* );
	}
};

#define  DECLARE_FIELD_DATA_MAP()\
public:\
	static dataMap_t& getDataMap();\
protected:\
	static dataMap_t m_dataMap;\
	static dataFieldBase* m_dataFieldArray[];\
	template< class T > friend struct IntiDataMapHelper;

#define  BEGIN_FIELD_DATA_MAP_NOBASE( CLASS )\
	dataMap_t& CLASS::getDataMap(){ return m_dataMap; }\
	dataMap_t  CLASS::m_dataMap = { m_dataFieldArray  , 0 , NULL };\
	INTI_DATA_MAP( CLASS )\
	dataFieldBase*  CLASS::m_dataFieldArray[] = {

#define  BEGIN_FIELD_DATA_MAP( CLASS , BASE )\
	dataMap_t& CLASS::getDataMap(){ return m_dataMap; }\
	dataMap_t  CLASS::m_dataMap = { m_dataFieldArray  , 0 , &BASE::m_dataMap };\
	INTI_DATA_MAP( CLASS )\
	dataFieldBase*  CLASS::m_dataFieldArray[] = {

#define  DEFINE_FIELD_TYPE( CLASS , MEMBER , TYPE )\
	new dataField_t<TYPE>( MEMBEROFFSET( CLASS , MEMBER ) ) ,

#define  DEFINE_FIELD( CLASS , MEMBER )\
	makeDataField( MEMBEROFFSET( CLASS , MEMBER )  , &CLASS::MEMBER ) ,

#define  DEFINE_FIELD_FUN( CLASS , SAVEFUN , LOADFUN )\
	makeDataFieldFun< CLASS >( SAVEFUN , LOADFUN ) ,


#define INTI_DATA_MAP( CLASS )\
	namespace IntiDataMapNamespace {\
		static IntiDataMapHelper<CLASS> g_IntiDataMap_##CLASS; }\
    template <> struct HaveDataMap<CLASS>{ typedef true_type result_type; };

#define END_FIELD_DATA_MAP( ) };\






#endif // TDataMap_h__