#ifndef ConsoleSystem_h__
#define ConsoleSystem_h__


#include "CoreShare.h"


#include "FunCallback.h"
#include <cstring>
#include "Singleton.h"
#include "FixString.h"
#include "HashString.h"
#include "TypeConstruct.h"

#include <string>
#include <map>

class ConsoleCommandBase
{
public:
 	ConsoleCommandBase( char const* inName , int inNumParam , char const** inPararmFormat );
	virtual ~ConsoleCommandBase(){}

	std::string  name;
	char const** paramFormat;
	int          numParam;

	virtual void execute( void* argsData[] ) = 0;
	virtual void getValue( void* pDest ){}

};


template < class FunSig , class T = void >
struct TMemberFunConsoleCommand : public ConsoleCommandBase
{
	FunSig fun;
	T*     mObject;

	TMemberFunConsoleCommand( char const* inName , FunSig inFun, T* inObj = NULL )
		:ConsoleCommandBase(inName,
			detail::FunTraits<FunSig>::NumParam ,
			detail::FunTraits<FunSig>::getParam() )
		,fun(inFun), mObject(inObj)
	{
	}

	virtual void execute(void* argsData[]) override
	{
		ExecuteCallbackFun(  fun , mObject, argsData );
	}
};

template < class FunSig >
struct BaseFunCom : public ConsoleCommandBase
{
	FunSig mFun;
	BaseFunCom( char const* inName, void* inFun)
		:ConsoleCommandBase(inName,
			detail::FunTraits<FunSig>::NumParam ,
			detail::FunTraits<FunSig>::getParam() )
		,mFun(inFun)
	{
	}

	virtual void execute(void* argsData[]) override
	{
		ExecuteCallbackFun( mFun , argsData);
	}
};


template < class Type >
struct TVariableConsoleCommad : public ConsoleCommandBase
{
	Type* mPtr;

	TVariableConsoleCommad( char const* inName, Type* inPtr )
		:ConsoleCommandBase(inName, 1 , detail::TypeToParam< Type >::getParam() )
		,mPtr(inPtr)
	{
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


	CORE_API void     insertCommand(ConsoleCommandBase* com);

	bool isInitialized() const { return mbInitialized; }

	char const* getErrorMsg() const { return mLastErrorMsg.c_str(); }

	template < class T >
	void registerVar( char const* name , T* obj )
	{
		auto* command = new TVariableConsoleCommad<T>( name , obj );
		insertCommand(command);
	}

	template < class FunSig , class T >
	 void registerCommand( char const* name , FunSig fun , T* obj )
	{
		auto* command = new TMemberFunConsoleCommand<FunSig , T >( name ,fun , obj );
		insertCommand(command);
	}

	template < class FunSig >
	void registerCommand( char const* name , FunSig fun )
	{
		auto* command = new BaseFunCom<FunSig>( name ,fun );
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
		int  numParam;
		int  numUsedParam;
		bool init(char const* inCommandString);
	};

	int  fillParameterData(ExecuteContext& context , uint8* data , char const* format );
	bool executeCommandImpl(char const* comStr);
	ConsoleCommandBase* findCommand( char const* str );


	struct StrCmp
	{
		bool operator()(const char* s1, const char* s2) const
		{
			return strcmp(s1, s2) < 0;
		}
	};

	typedef std::map< char const* , ConsoleCommandBase* ,StrCmp > CommandMap;

	CommandMap  mNameMap;
	bool        mbInitialized = false;
	ConsoleCommandBase* mRegisterdCommand = nullptr;
	std::string  mLastErrorMsg;

	friend struct ConsoleCommandBase;
};

template < class T >
inline void MakeConsoleCommand( char const* name , T* obj )
{
	TVariableConsoleCommad<T>* command = new TVariableConsoleCommad<T>( name , obj );
}

template < class FunSig , class T >
inline void MakeConsoleCommand( char const* name , FunSig fun , T* obj )
{
	TMemberFunConsoleCommand<FunSig , T >* command = new TMemberFunConsoleCommand<FunSig , T >( name ,fun , obj );
}

template < class FunSig >
inline void MakeConsoleCommand( char const* name , FunSig fun )
{
	BaseFunCom<FunSig>* command = new BaseFunCom<FunSig>( name ,fun );
}

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
		ConsoleSystem::Get().unregisterCommand( mCommand );
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
		ConsoleSystem::Get().unregisterCommand(mCommand);
	}

	T const& getValue() const { return mValueRef; }
	TConsoleVariable operator = (T const& val) { mValueRef = val; }
	operator T() { return mValueRef; }

	TVariableConsoleCommad<T>* mCommand;
	T& mValueRef;
};



#endif // ConsoleSystem_h__
