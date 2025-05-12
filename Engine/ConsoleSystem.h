#pragma once
#ifndef ConsoleSystem_H_D2FC89F2_0A99_4F1A_89D4_DB18A8247D7C
#define ConsoleSystem_H_D2FC89F2_0A99_4F1A_89D4_DB18A8247D7C

#include "CoreShare.h"

#include "FunctionTraits.h"

#include "HashString.h"
#include "TypeMemoryOp.h"
#include "Template/ArrayView.h"
#include "Meta/EnableIf.h"

#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"

#include <string>
#include <functional>
#include <typeindex>

enum EConsoleVariableFlag
{
	CVF_CONFIG        = 1 << 0,
	CVF_TOGGLEABLE    = 1 << 1,
	CVF_ALLOW_IGNORE_ARGS = 1 << 2,
	CVF_SECTION_GROUP = 1 << 3,
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
DEFINE_COM_ARG_TYPE_TRAITS(uint8, "%hhu", 1)
//DEFINE_COM_ARG_TYPE_TRAITS(int8, "%hhd", 1)
DEFINE_COM_ARG_TYPE_TRAITS(uint16, "%hu", 1)
DEFINE_COM_ARG_TYPE_TRAITS(int16, "%hd", 1)
DEFINE_COM_ARG_TYPE_TRAITS(uint32, "%u", 1)
DEFINE_COM_ARG_TYPE_TRAITS(int32, "%d", 1)
//DEFINE_COM_ARG_TYPE_TRAITS(unsigned char, "%u", 1)
DEFINE_COM_ARG_TYPE_TRAITS(char, "%c", 1)
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
		static ConsoleArgTypeInfo const StaticArgs[] = { TCommandArgTypeTraits< typename Meta::TypeTraits<Args>::BaseType >::GetInfo()... };
		return MakeView(StaticArgs);
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
		static ConsoleArgTypeInfo const StaticArgs[] = { TCommandArgTypeTraits< typename Meta::TypeTraits<Args>::BaseType >::GetInfo()... };
		return MakeView(StaticArgs);
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
		return FStringConv::ToCheck(str.data(), (int)str.length(), *mPtr);
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

	virtual bool setFromInt(int inValue) { *mPtr = (Type)inValue; return true; }
	virtual int  getAsInt() const { return int(*mPtr); }

	virtual bool  setFromFloat(float inValue) { *mPtr = (Type)inValue;  return true; }
	virtual float getAsFloat() const { return float(*mPtr); }
};


template <>
struct TVariableConsoleCommad<std::string> : public VariableConsoleCommadBase
{
	std::string* mPtr;

	TVariableConsoleCommad(char const* inName, std::string* inPtr, uint32 flags = 0)
		:VariableConsoleCommadBase(inName, GetArg(), flags)
		, mPtr(inPtr)
	{

	}

	virtual std::type_index getTypeIndex() const override
	{
		return typeid(Meta::RemoveCVRef<std::string>::Type);
	}
	virtual std::string toString() const override
	{
		return *mPtr;
	}

	virtual bool setFromString(StringView const& str) override
	{
		*mPtr = str.toStdString();
		return true;
	}

	static TArrayView< ConsoleArgTypeInfo const > GetArg()
	{
		static ConsoleArgTypeInfo const sArg = TCommandArgTypeTraits<char*>::GetInfo();
		return TArrayView< ConsoleArgTypeInfo const >(&sArg, 1);
	}
	
	virtual void execute(void* argsData[]) override
	{
		*mPtr = *(char const**)argsData[0];
	}

	virtual void getValue(void* pDest) override
	{
		//FTypeMemoryOp::Assign((Type*)pDest, *mPtr);
	}

	virtual void setValue(void* pDest)
	{
		//FTypeMemoryOp::Assign(mPtr, *(Type*)pDest);
	}

	virtual bool setFromInt(int inValue) { return false; }
	virtual int  getAsInt() const { return 0; }

	virtual bool  setFromFloat(float inValue) {  return false; }
	virtual float getAsFloat() const { return 0; }
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


class IConsoleSystem
{
public:

	static CORE_API IConsoleSystem& Get();

	virtual bool  initialize() = 0;
	virtual void  finalize() = 0;

	virtual bool  executeCommand(char const* inCmdText) = 0;
	virtual int   getAllCommandNames(char const* buffer[], int bufSize) = 0;
	virtual int   findCommandName(char const* includeStr, char const** findStr, int maxNum) = 0;
	virtual int   findCommandName2(char const* includeStr, char const** findStr, int maxNum) = 0;
	virtual void  unregisterCommand(ConsoleCommandBase* commond) = 0;
	virtual void  unregisterCommandByName(char const* name) = 0;
	virtual void  unregisterAllCommandsByObject(void* objectPtr) = 0;
	virtual ConsoleCommandBase* findCommand(char const* comName) = 0;
	virtual bool  insertCommand(ConsoleCommandBase* com) = 0;

	bool isInitialized() const { return mbInitialized; }


	virtual void  visitAllVariables(std::function<void(VariableConsoleCommadBase*)> func) = 0;


	template < class T >
	auto* registerVar(char const* name, T* obj)
	{
		auto* command = new TVariableConsoleCommad<T>(name, obj);
		if (!insertCommand(command))
			return decltype(command)(nullptr);
		return command;
	}

	template < class TFunc, class T >
	auto* registerCommand(char const* name, TFunc func, T* obj, uint32 flags = 0)
	{
		auto* command = new TMemberFuncConsoleCommand<TFunc, T >(name, func, obj, flags);
		if (!insertCommand(command))
			return decltype(command)(nullptr);
		return command;
	}

	template < class TFunc >
	auto* registerCommand(char const* name, TFunc func, uint32 flags = 0)
	{
		auto* command = new TBaseFuncConsoleCommand<TFunc>(name, func, flags);
		if (!insertCommand(command))
			return decltype(command)(nullptr);
		return command;
	}

protected:
	bool         mbInitialized = false;

};


template< class T >
class TConsoleVariable
{
public:
	TConsoleVariable(T const& val , char const* name, uint32 flags = 0)
		:mValue(val)
	{
		auto command = new TVariableConsoleCommad<T>( name, &mValue, flags);
		if (IConsoleSystem::Get().insertCommand(command))
		{
			mCommand = command;
		}
	}
	~TConsoleVariable()
	{
		if (mCommand && IConsoleSystem::Get().isInitialized())
		{
			IConsoleSystem::Get().unregisterCommand(mCommand);
		}
	}

	T const& getValue() const { return mValue; }
	TConsoleVariable& operator = (T const& val) { mValue = val;  return *this;  }
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
		if (IConsoleSystem::Get().insertCommand(command))
		{
			mCommand = command;
		}
	}
	~TConsoleVariableDelegate()
	{
		if (mCommand && IConsoleSystem::Get().isInitialized())
		{
			IConsoleSystem::Get().unregisterCommand(mCommand);
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
		if (IConsoleSystem::Get().insertCommand(command))
		{
			mCommand = command;
		}
	}
	~TConsoleVariableRef()
	{
		if (mCommand && IConsoleSystem::Get().isInitialized())
		{
			IConsoleSystem::Get().unregisterCommand(mCommand);
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
		mCommand = IConsoleSystem::Get().registerCommand(name, func);
	}

	template< class TFunc , class T >
	AutoConsoleCommand(char const* name, TFunc func, T* object)
	{
		mCommand = IConsoleSystem::Get().registerCommand(name, func, object);
	}

	~AutoConsoleCommand()
	{
		if ( mCommand && IConsoleSystem::Get().isInitialized())
		{
			IConsoleSystem::Get().unregisterCommand(mCommand);
		}
	}

	ConsoleCommandBase* mCommand;
};

#endif // ConsoleSystem_H_D2FC89F2_0A99_4F1A_89D4_DB18A8247D7C