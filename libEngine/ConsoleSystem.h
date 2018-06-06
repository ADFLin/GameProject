#ifndef ConsoleSystem_h__
#define ConsoleSystem_h__


#include "CoreShare.h"


#include "FunCallback.h"
#include <cstring>
#include "Singleton.h"

#include <string>
#include <map>

struct ComBase
{
 	CORE_API ComBase( char const* _name , void* _ptr ,
		     int _num , char const** _pararm );

	CORE_API virtual ~ComBase(){}

	std::string  name;
	void*        ptr;
	char const** Param;
	int          numParam;

	ComBase* next;
	virtual void exec(void** dataMap ) = 0;
	virtual void getVal(void* g){}

	static ComBase* s_unRegCom;

};


template < class FunSig , class T = void >
struct MemFunCom : public ComBase
{
	FunSig fun;

	MemFunCom( char const* _name , FunSig _fun , T* _obj = NULL )
		:ComBase( _name , _obj,
			detail::FunTraits<FunSig>::NumParam ,
			detail::FunTraits<FunSig>::getParam() )
		,fun(_fun)
	{
	}

	void exec(void** dataMap )
	{
		execCallbackFun(  fun , (T*) ptr , dataMap );
	}
};

template < class FunSig >
struct BaseFunCom : public ComBase
{
	BaseFunCom( char const* _name , void* _fun )
		:ComBase( _name ,_fun ,
			detail::FunTraits<FunSig>::NumParam ,
			detail::FunTraits<FunSig>::getParam() )
	{
	}

	void exec(void** dataMap )
	{
		execCallbackFun( ( FunSig )ptr , dataMap );
	}
};


template < class Type >
struct VarCom : public ComBase
{
	VarCom( char const* _name , void* _ptr )
		:ComBase( _name, _ptr , 1 , detail::TypeToParam< Type >::getParam() )
	{
	}

	void exec( void** dataMap )
	{
		*(Type*)ptr = *(Type*)dataMap[0];
		//memcpy( ptr , dataMap[0] , sizeof(Type) );
	}

	void getVal(void* g)
	{
		*(Type*)g   = *(Type*) ptr;
	}
};

class ConsoleSystem : public SingletonT< ConsoleSystem >
{
public:
	ConsoleSystem();
	~ConsoleSystem();

	CORE_API bool        init();
	CORE_API bool        executeCommand( char const* comStr );
	CORE_API int         findCommandName( char const* includeStr, char const** findStr , int maxNum );
	CORE_API int         findCommandName2( char const* includeStr , char const** findStr , int maxNum );

	CORE_API void        unregisterByName( char const* name );
	char const* getErrorMsg() const { return m_errorMsg.c_str(); }

	template < class T >
	static void registerVar( char const* name , T* obj )
	{
		VarCom<T>* command = new VarCom<T>( name , obj );
	}

	template < class FunSig , class T >
	static void registerCommand( char const* name , FunSig fun , T* obj )
	{
		MemFunCom<FunSig , T >* command = new MemFunCom<FunSig , T >( name ,fun , obj );
	}

	template < class FunSig >
	static void registerCommand( char const* name , FunSig fun )
	{
		BaseFunCom<FunSig>* command = new BaseFunCom<FunSig>( name ,fun );
	}

protected:
	void     insertCommand( ComBase* com );
	char*    fillVarData( char* data , char const* format );
	ComBase* findCommand( char const* str );


	struct StrCmp
	{
		bool operator()(const char* s1, const char* s2) const
		{
			return strcmp(s1, s2) < 0;
		}
	};

	typedef std::map< char const* , ComBase* ,StrCmp > CommandMap;

	CommandMap  mNameMap;
	ComBase*    m_curCom;

	static int const NumMaxParams = 16;

	int         m_numStr;
	char*       m_str[ NumMaxParams ];
	int         m_numParam;
	int         m_numUsedParam;
	int         m_fillNum[ NumMaxParams ];
	char*       m_paramStr[ NumMaxParams ];
	void*       m_dataMap[NumMaxParams];

	std::string m_errorMsg;

	char*       m_strBuffer;
	char*       m_dataBuffer;


	friend struct ComBase;
};

template < class T >
inline void ConVarCom( char const* name , T* obj )
{
	VarCom<T>* command = new VarCom<T>( name , obj );
}

template < class FunSig , class T >
inline void ConCommand( char const* name , FunSig fun , T* obj )
{
	MemFunCom<FunSig , T >* command = new MemFunCom<FunSig , T >( name ,fun , obj );
}

template < class FunSig >
inline void ConCommand( char const* name , FunSig fun )
{
	BaseFunCom<FunSig>* command = new BaseFunCom<FunSig>( name ,fun );
}

template< class T >
class ConVar
{
public:
	ConVar(T const& val , char const* name )
		:m_val(val)
	{
		data = new VarCom<T>( name , &m_val );
	}
	~ConVar()
	{
		extern ConsoleSystem* getConsole();
		if ( getConsole() )
			getConsole()->unregisterByName( data->name.c_str() );
	}

	T const& getValue() const { return m_val; }
	ConVar operator = ( T const& val ){   m_val = val;   }
	operator T(){ return m_val; }

	VarCom<T>* data;
	T m_val;
};


template< class T >
class ConVarRef
{
public:
	ConVarRef(T& val, char const* name)
		:m_val(val)
	{
		data = new VarCom<T>(name, &m_val);
	}
	~ConVarRef()
	{
		extern ConsoleSystem* getConsole();
		if( getConsole() )
			getConsole()->unregisterByName(data->name.c_str());
	}

	T const& getValue() const { return m_val; }
	ConVar operator = (T const& val) { m_val = val; }
	operator T() { return m_val; }

	VarCom<T>* data;
	T& m_val;
};

#define DECLARE_CON_VAR( type , name ) extern ConVar<type> name;
#define DEFINE_CON_VAR( type , name , val ) ConVar<type> name( val , #name );


#endif // ConsoleSystem_h__
