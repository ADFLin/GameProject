#pragma once
#ifndef ConsoleSystem_H_D2FC89F2_0A99_4F1A_89D4_DB18A8247D7C
#define ConsoleSystem_H_D2FC89F2_0A99_4F1A_89D4_DB18A8247D7C

#include "CoreShare.h"

#include "FunctionTraits.h"
#include "InlineString.h"
#include "HashString.h"
#include "TypeMemoryOp.h"
#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"
#include "Template/ArrayView.h"
#include "Core/StringConv.h"
#include "Meta/EnableIf.h"
#include "Misc/StringPtrWrapper.h"

#include <string>
#include <map>
#include <unordered_map>
#include <typeindex>
#include <functional>


enum EConsoleVariableFlag
{
	CVF_CONFIG        = 1 << 0,
	CVF_TOGGLEABLE    = 1 << 1,
	CVF_CAN_OMIT_ARGS = 1 << 2,
};

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
DEFINE_COM_ARG_TYPE_TRAITS(uint32, "%u", 1)
DEFINE_COM_ARG_TYPE_TRAITS(int32, "%d", 1)
DEFINE_COM_ARG_TYPE_TRAITS(unsigned char, "%u", 1)
DEFINE_COM_ARG_TYPE_TRAITS(char, "%d", 1)
DEFINE_COM_ARG_TYPE_TRAITS(char*, "%s", 1)
DEFINE_COM_ARG_TYPE_TRAITS(char const*, "%s", 1)
DEFINE_COM_ARG_TYPE_TRAITS(Math::Vector2, "%f%f", 2)
DEFINE_COM_ARG_TYPE_TRAITS(Math::Vector3, "%f%f%f", 3)
DEFINE_COM_ARG_TYPE_TRAITS(Math::Vector4, "%f%f%f%f", 4)


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
		static ConsoleArgTypeInfo const sArgs[] = { TCommandArgTypeTraits< typename Meta::TypeTraits<Args>::BaseType >::GetInfo()... };
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
		static ConsoleArgTypeInfo const sArgs[] = { TCommandArgTypeTraits< typename Meta::TypeTraits<Args>::BaseType >::GetInfo()... };
		return MakeView(sArgs);
	}
};

class VariableConsoleCommadBase;

class ConsoleCommandBase
{
public:
 	ConsoleCommandBase( char const* inName , TArrayView< ConsoleArgTypeInfo const > inArgs, uint32 flags = 0);
	virtual ~ConsoleCommandBase(){}

	std::string   mName;
	TArrayView< ConsoleArgTypeInfo const > mArgs;
	uint32 getFlags() { return mFlags; }
	uint32 mFlags;

	virtual void execute( void* argsData[] ) = 0;
	virtual void getValue( void* pDest ){}
	virtual bool isHoldObject(void* objectPtr) { return false; }
	virtual VariableConsoleCommadBase* asVariable() { return nullptr; }

};


template < class TFunc, class T = void >
struct TMemberFuncConsoleCommand : public ConsoleCommandBase
{
	TFunc  mFunc;
	T*     mObject;

	TMemberFuncConsoleCommand( char const* inName , TFunc inFunc, T* inObj = NULL, uint32 flags = 0)
		:ConsoleCommandBase(inName, TCommandFuncTraints<TFunc>::GetArgs() , flags)
		, mFunc(inFunc), mObject(inObj)
	{
	}

	virtual void execute(void* argsData[]) override
	{
		Meta::Invoke(mFunc, mObject, argsData );
	}

	virtual bool isHoldObject(void* objectPtr) override
	{
		return objectPtr == mObject;
	}
};

template < class TFunc >
struct TBaseFuncConsoleCommand : public ConsoleCommandBase
{
	TFunc mFunc;
	TBaseFuncConsoleCommand( char const* inName, TFunc inFunc, uint32 flags = 0)
		:ConsoleCommandBase(inName, TCommandFuncTraints<TFunc>::GetArgs() , flags)
		,mFunc(inFunc)
	{
	}

	virtual void execute(void* argsData[]) override
	{
		Meta::Invoke(mFunc, argsData);
	}
};

class VariableConsoleCommadBase : public ConsoleCommandBase
{
public:
	VariableConsoleCommadBase(char const* inName, TArrayView< ConsoleArgTypeInfo const > inArgs, uint32 flags)
		:ConsoleCommandBase(inName, inArgs, flags)
	{

	}
	virtual std::type_index getTypeIndex() const = 0;
	virtual VariableConsoleCommadBase* asVariable() { return this; }
	virtual std::string toString() const = 0;
	virtual bool setFromString(StringView const& str) = 0;
	virtual void setValue(void* pDest) {}

	virtual bool setFromInt(int inValue) { return false; }
	virtual int  getAsInt() const { return 0; }

	virtual bool  setFromFloat(float inValue) { return false; }
	virtual float getAsFloat() const { return 0; }
};

template < class Type >
struct TVariableConsoleCommad : public VariableConsoleCommadBase
{
	Type* mPtr;

	TVariableConsoleCommad( char const* inName, Type* inPtr , uint32 flags = 0)
		:VariableConsoleCommadBase(inName, GetArg() , flags)
		,mPtr(inPtr)
	{

	}

	virtual std::type_index getTypeIndex() const override
	{
		return typeid(Meta::RemoveCVRef<Type>::Type);
	}
	virtual std::string toString() const override
	{
		return FStringConv::From(*mPtr);
	}

	virtual bool setFromString(StringView const& str) override
	{
		return FStringConv::ToCheck(str.data(), str.length(), *mPtr);
	}

	static TArrayView< ConsoleArgTypeInfo const > GetArg()
	{
		static ConsoleArgTypeInfo const sArg = TCommandArgTypeTraits<Type>::GetInfo();
		return TArrayView< ConsoleArgTypeInfo const >(&sArg , 1);
	}
	virtual void execute(void* argsData[]) override
	{
		FTypeMemoryOp::Assign(mPtr, *(Type*)argsData[0]);
	}

	virtual void getValue(void* pDest) override
	{
		FTypeMemoryOp::Assign((Type*)pDest, *mPtr);
	}

	virtual void setValue(void* pDest) 
	{
		FTypeMemoryOp::Assign(mPtr, *(Type*)pDest);
	}

	virtual bool setFromInt(int inValue) { *mPtr = inValue; return true; }
	virtual int  getAsInt() const { return *mPtr; }

	virtual bool  setFromFloat(float inValue) { *mPtr = inValue;  return true; }
	virtual float getAsFloat() const { return *mPtr; }
};


template < class Type >
struct TVariableConsoleDelegateCommad : public VariableConsoleCommadBase
{
	using GetValueDelegate = std::function< Type(void) >;
	using SetValueDelegate = std::function< void(Type const& type) >;

	GetValueDelegate mGetValue;
	SetValueDelegate mSetValue;

	TVariableConsoleDelegateCommad(char const* inName, GetValueDelegate inGetValue , SetValueDelegate inSetValue , uint32 flags = 0)
		:VariableConsoleCommadBase(inName, GetArg(), flags)
		, mGetValue(inGetValue)
		, mSetValue(inSetValue)
	{

	}

	virtual std::type_index getTypeIndex() const override
	{
		return typeid(Meta::RemoveCVRef<Type>::Type);
	}
	virtual std::string toString() const override
	{
		return FStringConv::From(mGetValue());
	}

	virtual bool setFromString(StringView const& str) override
	{
		return formStringImpl(str, typename Meta::IsSameType< Type, char const* >::Type() );
	}

	bool formStringImpl(StringView const& str , Meta::FalseType )
	{
		Type value;
		if (!FStringConv::ToCheck(str.data(), str.length(), value))
			return false;
		mSetValue(value);
		return true;
	}

	bool formStringImpl(StringView const& str, Meta::TureType )
	{
		mSetValue(str.toCString());
		return true;
	}

	static TArrayView< ConsoleArgTypeInfo const > GetArg()
	{
		static ConsoleArgTypeInfo const sArg = TCommandArgTypeTraits<Type>::GetInfo();
		return TArrayView< ConsoleArgTypeInfo const >(&sArg, 1);
	}

	virtual void execute(void* argsData[]) override
	{
		mSetValue(*(Type*)argsData[0]);
	}

	virtual void getValue(void* pDest) override
	{
		FTypeMemoryOp::Assign((Type*)pDest, mGetValue());
	}

	virtual void setValue(void* pDest)
	{
		mSetValue(*(Type*)pDest);
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

	CORE_API bool        executeCommand(char const* inCmdText);
	CORE_API int         getAllCommandNames(char const* buffer[], int bufSize);
	CORE_API int         findCommandName( char const* includeStr, char const** findStr , int maxNum );
	CORE_API int         findCommandName2( char const* includeStr , char const** findStr , int maxNum );

	CORE_API void        unregisterCommand( ConsoleCommandBase* commond );
	CORE_API void        unregisterCommandByName( char const* name );
	CORE_API void        unregisterAllCommandsByObject(void* objectPtr);

	CORE_API ConsoleCommandBase* findCommand(char const* comName);


	CORE_API bool       insertCommand(ConsoleCommandBase* com);

	bool isInitialized() const { return mbInitialized; }

	template < class T >
	auto* registerVar( char const* name , T* obj )
	{
		auto* command = new TVariableConsoleCommad<T>( name , obj );
		if (!insertCommand(command))
			return decltype(command)(nullptr);
		return command;
	}

	template < class TFunc, class T >
	auto* registerCommand( char const* name , TFunc func , T* obj , uint32 flags = 0)
	{
		auto* command = new TMemberFuncConsoleCommand<TFunc, T >( name ,func , obj, flags);
		if (!insertCommand(command))
			return decltype(command)(nullptr);
		return command;
	}

	template < class TFunc >
	auto* registerCommand( char const* name , TFunc func, uint32 flags = 0)
	{
		auto* command = new TBaseFuncConsoleCommand<TFunc>( name ,func, flags);
		if (!insertCommand(command))
			return decltype(command)(nullptr);
		return command;
	}

	template < class TFunc >
	void  visitAllVariables(TFunc&& visitFunc)
	{
		for (auto const& pair : mNameMap)
		{
			auto variable = pair.second->asVariable();
			if (variable)
			{
				visitFunc(variable);
			}
		}
	}

protected:

	static int const NumMaxParams = 16;
	struct ExecuteContext
	{
		char buffer[512];
		ConsoleCommandBase* command;

		char const* text;
		char const* name;
		char const* args[NumMaxParams];
		int  numArgs;
		int  numArgUsed;
		bool parseText();

		std::string errorMsg;
	};

	bool fillArgumentData(ExecuteContext& context , ConsoleArgTypeInfo const& arg, uint8* outData, bool bCanOmitArgs);
	bool executeCommandImpl(ExecuteContext& context);
#if 0
	using CommandMap = std::map< TStringPtrWrapper<char, true> , ConsoleCommandBase* >;
#else
	using CommandMap = std::unordered_map< TStringPtrWrapper<char, true>, ConsoleCommandBase*, MemberFuncHasher>;
#endif
	CommandMap   mNameMap;
	bool         mbInitialized = false;

	friend class ConsoleCommandBase;
};

template< class T >
class TConsoleVariable
{
public:
	TConsoleVariable(T const& val , char const* name, uint32 flags = 0)
		:mValue(val)
	{
		auto command = new TVariableConsoleCommad<T>( name, &mValue, flags);
		if (ConsoleSystem::Get().insertCommand(command))
		{
			mCommand = command;
		}
	}
	~TConsoleVariable()
	{
		if (mCommand && ConsoleSystem::Get().isInitialized())
		{
			ConsoleSystem::Get().unregisterCommand(mCommand);
		}
	}

	T const& getValue() const { return mValue; }
	TConsoleVariable operator = ( T const& val ){   mValue = val;   }
	operator T(){ return mValue; }

	TVariableConsoleCommad<T>* mCommand = nullptr;
	T mValue;
};

template< class T >
class TConsoleVariableDelegate
{
public:
	TConsoleVariableDelegate(
		typename TVariableConsoleDelegateCommad<T>::GetValueDelegate inGetValue ,
		typename TVariableConsoleDelegateCommad<T>::SetValueDelegate inSetValue ,
		char const* name, uint32 flags = 0)
	{
		auto command = new TVariableConsoleDelegateCommad<T>(name, inGetValue, inSetValue, flags);
		if (ConsoleSystem::Get().insertCommand(command))
		{
			mCommand = command;
		}
	}
	~TConsoleVariableDelegate()
	{
		if (mCommand && ConsoleSystem::Get().isInitialized())
		{
			ConsoleSystem::Get().unregisterCommand(mCommand);
		}
	}

	TVariableConsoleDelegateCommad<T>* mCommand = nullptr;
};

template< class T >
class TConsoleVariableRef
{
public:
	TConsoleVariableRef(T& val, char const* name, uint32 flags = 0)
		:mValueRef(val)
	{
		auto command = new TVariableConsoleCommad<T>(name, &mValueRef, flags);
		if (ConsoleSystem::Get().insertCommand(command))
		{
			mCommand = command;
		}
	}
	~TConsoleVariableRef()
	{
		if (mCommand && ConsoleSystem::Get().isInitialized())
		{
			ConsoleSystem::Get().unregisterCommand(mCommand);
		}
	}

	T const& getValue() const { return mValueRef; }
	TConsoleVariableRef& operator = (T const& val) { mValueRef = val; return *this; }
	operator T() { return mValueRef; }

	TVariableConsoleCommad<T>* mCommand = nullptr;
	T& mValueRef;
};

class AutoConsoleCommand
{
public:
	template< class TFunc >
	AutoConsoleCommand(char const* name, TFunc func)
	{
		mCommand = ConsoleSystem::Get().registerCommand(name, func);
	}

	template< class TFunc , class T >
	AutoConsoleCommand(char const* name, TFunc func, T* object)
	{
		mCommand = ConsoleSystem::Get().registerCommand(name, func, object);
	}

	~AutoConsoleCommand()
	{
		if ( mCommand && ConsoleSystem::Get().isInitialized())
		{
			ConsoleSystem::Get().unregisterCommand(mCommand);
		}
	}

	ConsoleCommandBase* mCommand;
};

#endif // ConsoleSystem_H_D2FC89F2_0A99_4F1A_89D4_DB18A8247D7C