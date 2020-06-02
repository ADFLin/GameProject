#ifndef ConsoleSystem_h__
#define ConsoleSystem_h__


#include "CoreShare.h"


#include "FunctionTraits.h"
#include <cstring>
#include "Singleton.h"
#include "FixString.h"
#include "HashString.h"
#include "TypeConstruct.h"
#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"
#include "Template/ArrayView.h"

#include <string>
#include <map>


struct ConsoleArgTypeInfo
{
	int size;
	char const* format;
	int numElements;
};


template< class T>
struct TCommandArgTypeTraits {};

#define DEFINE_COM_ARG_TYPE_TRAITS( TYPE , FORMAT , NUM_ELEMENTS ) \
	template<>\
	struct TCommandArgTypeTraits< TYPE >\
	{ \
		static ConsoleArgTypeInfo GetInfo()\
		{\
			return { sizeof(TYPE) , FORMAT , NUM_ELEMENTS };\
		}\
	};


DEFINE_COM_ARG_TYPE_TRAITS(bool, "%d", 1)
DEFINE_COM_ARG_TYPE_TRAITS(float, "%f", 1)
DEFINE_COM_ARG_TYPE_TRAITS(double, "%lf", 1)
DEFINE_COM_ARG_TYPE_TRAITS(unsigned, "%u", 1)
DEFINE_COM_ARG_TYPE_TRAITS(int, "%d", 1)
DEFINE_COM_ARG_TYPE_TRAITS(unsigned char, "%u", 1)
DEFINE_COM_ARG_TYPE_TRAITS(char, "%d", 1)
DEFINE_COM_ARG_TYPE_TRAITS(char*, "%s", 1)
DEFINE_COM_ARG_TYPE_TRAITS(char const*, "%s", 1)
DEFINE_COM_ARG_TYPE_TRAITS(Math::Vector2, "%f%f", 1)
DEFINE_COM_ARG_TYPE_TRAITS(Math::Vector3, "%f%f%f", 2)
DEFINE_COM_ARG_TYPE_TRAITS(Math::Vector4, "%f%f%f%f", 3)


template< class FuncSig >
struct TCommandFuncTraints {};

template < class RT >
struct TCommandFuncTraints< RT(*)() >
{
	static TArrayView< ConsoleArgTypeInfo const > GetArgs()
	{
		return TArrayView< ConsoleArgTypeInfo const >();
	}
};

template < class RT, class ...Args  >
struct TCommandFuncTraints< RT(*)(Args...) >
{
	static TArrayView< ConsoleArgTypeInfo const > GetArgs()
	{
		static ConsoleArgTypeInfo const sArgs[] = { TCommandArgTypeTraits< Meta::TypeTraits<Args>::BaseType >::GetInfo()... };
		return MakeView( sArgs );
	}
};

template < class RT, class T >
struct TCommandFuncTraints< RT(T::*)() >
{
	static TArrayView< ConsoleArgTypeInfo const > GetArgs()
	{
		return TArrayView< ConsoleArgTypeInfo const >();
	}
};

template < class RT, class T, class ...Args >
struct TCommandFuncTraints< RT(T::*)(Args...) >
{
	static TArrayView< ConsoleArgTypeInfo const > GetArgs()
	{
		static ConsoleArgTypeInfo const sArgs[] = { TCommandArgTypeTraits< Meta::TypeTraits<Args>::BaseType >::GetInfo()... };
		return MakeView(sArgs);
	}
};



class ConsoleCommandBase
{
public:
 	ConsoleCommandBase( char const* inName , TArrayView< ConsoleArgTypeInfo const > inArgs );
	virtual ~ConsoleCommandBase(){}

	std::string   name;
	TArrayView< ConsoleArgTypeInfo const > args;

	virtual void execute( void* argsData[] ) = 0;
	virtual void getValue( void* pDest ){}

};


template < class TFunc, class T = void >
struct TMemberFuncConsoleCom : public ConsoleCommandBase
{
	TFunc  mFunc;
	T*     mObject;

	TMemberFuncConsoleCom( char const* inName , TFunc inFunc, T* inObj = NULL )
		:ConsoleCommandBase(inName, TCommandFuncTraints<TFunc>::GetArgs() )
		, mFunc(inFunc), mObject(inObj)
	{
	}

	virtual void execute(void* argsData[]) override
	{
		Meta::Invoke(mFunc, mObject, argsData );
	}
};

template < class TFunc >
struct BaseFuncConsoleCom : public ConsoleCommandBase
{
	TFunc mFunc;
	BaseFuncConsoleCom( char const* inName, TFunc inFunc)
		:ConsoleCommandBase(inName, TCommandFuncTraints<TFunc>::GetArgs() )
		,mFunc(inFunc)
	{
	}

	virtual void execute(void* argsData[]) override
	{
		Meta::Invoke(mFunc, argsData);
	}
};


template < class Type >
struct TVariableConsoleCommad : public ConsoleCommandBase
{
	Type* mPtr;

	TVariableConsoleCommad( char const* inName, Type* inPtr )
		:ConsoleCommandBase(inName, GetArg() )
		,mPtr(inPtr)
	{
	}

	static TArrayView< ConsoleArgTypeInfo const > GetArg()
	{
		static ConsoleArgTypeInfo const sArg = TCommandArgTypeTraits<Type>::GetInfo();
		return TArrayView< ConsoleArgTypeInfo const >(&sArg , 1);
	}
	virtual void execute(void* argsData[]) override
	{
		TypeDataHelper::Assign(mPtr, *(Type*)argsData[0]);
	}

	virtual void getValue(void* pDest) override
	{
		TypeDataHelper::Assign((Type*)pDest, *mPtr);
	}
};

class ConsoleSystem
{
public:
	ConsoleSystem();
	~ConsoleSystem();

	static CORE_API ConsoleSystem& Get();

	CORE_API bool        initialize();
	CORE_API void        finalize();

	
	CORE_API bool        executeCommand(char const* comStr);
	CORE_API int         findCommandName( char const* includeStr, char const** findStr , int maxNum );
	CORE_API int         findCommandName2( char const* includeStr , char const** findStr , int maxNum );

	CORE_API void        unregisterCommand( ConsoleCommandBase* commond );
	CORE_API void        unregisterCommandByName( char const* name );

	CORE_API ConsoleCommandBase* findCommand(char const* comName);


	CORE_API void     insertCommand(ConsoleCommandBase* com);

	bool isInitialized() const { return mbInitialized; }

	char const* getErrorMsg() const { return mLastErrorMsg.c_str(); }

	template < class T >
	void registerVar( char const* name , T* obj )
	{
		auto* command = new TVariableConsoleCommad<T>( name , obj );
		insertCommand(command);
	}

	template < class TFunc, class T >
	void registerCommand( char const* name , TFunc func , T* obj )
	{
		auto* command = new TMemberFuncConsoleCom<TFunc, T >( name ,func , obj );
		insertCommand(command);
	}

	template < class TFunc >
	void registerCommand( char const* name , TFunc func )
	{
		auto* command = new BaseFuncConsoleCom<TFunc>( name ,func );
		insertCommand(command);
	}

protected:


	static int const NumMaxParams = 16;
	struct ExecuteContext
	{
		FixString<512> buffer;
		ConsoleCommandBase*    command;
		char const* commandString;
		char const* paramStrings[NumMaxParams];
		int  numArgs;
		int  numUsedParam;
		bool init(char const* inCommandString);
	};

	bool fillParameterData(ExecuteContext& context , ConsoleArgTypeInfo const& arg, uint8* outData );
	bool executeCommandImpl(char const* comStr);


	struct StrCmp
	{
		bool operator()(const char* s1, const char* s2) const
		{
			return strcmp(s1, s2) < 0;
		}
	};
	typedef std::map< char const* , ConsoleCommandBase* , StrCmp > CommandMap;

	CommandMap  mNameMap;
	bool        mbInitialized = false;
	ConsoleCommandBase* mRegisterdCommand = nullptr;
	std::string  mLastErrorMsg;

	friend class ConsoleCommandBase;
};

template< class T >
class TConsoleVariable
{
public:
	TConsoleVariable(T const& val , char const* name )
		:mValue(val)
	{
		mCommand = new TVariableConsoleCommad<T>( name , &mValue );
		ConsoleSystem::Get().insertCommand(mCommand);
	}
	~TConsoleVariable()
	{
		if (ConsoleSystem::Get().isInitialized())
		{
			ConsoleSystem::Get().unregisterCommand(mCommand);
		}
	}

	T const& getValue() const { return mValue; }
	TConsoleVariable operator = ( T const& val ){   mValue = val;   }
	operator T(){ return mValue; }

	TVariableConsoleCommad<T>* mCommand;
	T mValue;
};


template< class T >
class TConsoleVariableRef
{
public:
	TConsoleVariableRef(T& val, char const* name)
		:mValueRef(val)
	{
		mCommand = new TVariableConsoleCommad<T>(name, &mValueRef);
		ConsoleSystem::Get().insertCommand(mCommand);
	}
	~TConsoleVariableRef()
	{
		if (ConsoleSystem::Get().isInitialized())
		{
			ConsoleSystem::Get().unregisterCommand(mCommand);
		}
	}

	T const& getValue() const { return mValueRef; }
	TConsoleVariableRef& operator = (T const& val) { mValueRef = val; return *this; }
	operator T() { return mValueRef; }

	TVariableConsoleCommad<T>* mCommand;
	T& mValueRef;
};



#endif // ConsoleSystem_h__
