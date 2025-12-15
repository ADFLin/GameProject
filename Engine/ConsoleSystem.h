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
#include "ReflectionCollect.h"

#include <string>
#include <functional>
#include <typeindex>
#include <tuple>

enum EConsoleVariableFlag
{
	CVF_CONFIG = 1 << 0,
	CVF_TOGGLEABLE = 1 << 1,
	CVF_ALLOW_IGNORE_ARGS = 1 << 2,
	CVF_SECTION_GROUP = 1 << 3,
};

struct ConsoleArgTypeInfo
{
	int size;
	char const* format;
	int numElements;
};


template< class T, class Enable = void >
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

template< class T >
struct TCommandArgTypeTraits< T, typename std::enable_if< std::is_enum<T>::value >::type >
{
	static ConsoleArgTypeInfo GetInfo()
	{
		return { sizeof(T) , "%s" , 1 };
	}
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

namespace ConsoleDetail
{
	// -- Trait to determine storage type for an argument --
	// Most types store as themselves
	template< typename T >
	struct ArgHolder { using Type = T; };

	// Store 'char const*' as 'std::string' to manage lifetime of $Vars
	template<> struct ArgHolder<char const*> { using Type = std::string; };
	template<> struct ArgHolder<char*> { using Type = std::string; }; // Wrapper for unsafe char*

	// -- Trait to retrieve value to pass to function --
	template< typename T >
	struct ArgPasser
	{
		static T Get(typename ArgHolder<T>::Type& val) { return val; }
	};

	// Pass 'std::string' storage as 'char const*'
	template<> struct ArgPasser<char const*>
	{
		static char const* Get(std::string& val) { return val.c_str(); }
	};

	template<> struct ArgPasser<char*>
	{
		static char* Get(std::string& val) { return const_cast<char*>(val.data()); }
	};


	struct ArgHelper
	{
		// Base Scalar Resolve
		template <typename T, typename TStorage>
		static bool Resolve(TArrayView<StringView> args, int& index, bool bAllowIgnoreArgs, TStorage& outValue)
		{
			if (index >= args.size())
			{
				if (bAllowIgnoreArgs)
				{
					outValue = TStorage();
					return true;
				}
				return false;
			}

			StringView str = args[index];
			++index;

			// Variable Substitution ($Var)
			if (str.length() > 1 && str[0] == '$')
			{
				StringView varName = str.substr(1);
				std::string nameStr = varName.toStdString();

				auto* command = IConsoleSystem::Get().findCommand(nameStr.c_str());
				if (command)
				{
					auto* var = command->asVariable();
					if (var)
					{
						std::string valStr = var->toString();
						return FStringConv::ToCheck(valStr.c_str(), valStr.length(), outValue);
					}
				}
				// Var not found
				return false;
			}

			// Literal Value
			return FStringConv::ToCheck(str.data(), str.length(), outValue);
		}

		// Specialization for String storage (char const* target)
		template <typename T>
		static bool ResolveString(TArrayView<StringView> args, int& index, bool bAllowIgnoreArgs, std::string& outValue)
		{
			if (index >= args.size())
			{
				if (bAllowIgnoreArgs) { outValue.clear(); return true; }
				return false;
			}

			StringView str = args[index];
			++index;

			if (str.length() > 1 && str[0] == '$')
			{
				// Resolve var to string
				StringView varName = str.substr(1);
				std::string nameStr = varName.toStdString();
				auto* command = IConsoleSystem::Get().findCommand(nameStr.c_str());
				if (command && command->asVariable())
				{
					outValue = command->asVariable()->toString();
					return true;
				}
				return false;
			}

			outValue = str.toStdString();
			return true;
		}

		// Dispatcher for char const* inputs, using std::string storage
		template <>
		static bool Resolve<char const*, std::string>(TArrayView<StringView> args, int& index, bool bAllowIgnoreArgs, std::string& outValue)
		{
			return ResolveString<char const*>(args, index, bAllowIgnoreArgs, outValue);
		}

		// Vector2 Specialization
		template <>
		static bool Resolve<Math::Vector2, Math::Vector2>(TArrayView<StringView> args, int& index, bool bAllowIgnoreArgs, Math::Vector2& outValue)
		{
			float x, y;
			if (!Resolve<float, float>(args, index, bAllowIgnoreArgs, x) ||
				!Resolve<float, float>(args, index, bAllowIgnoreArgs, y)) return false;
			outValue = Math::Vector2(x, y);
			return true;
		}

		// Vector3 Specialization
		template <>
		static bool Resolve<Math::Vector3, Math::Vector3>(TArrayView<StringView> args, int& index, bool bAllowIgnoreArgs, Math::Vector3& outValue)
		{
			float x, y, z;
			if (!Resolve<float, float>(args, index, bAllowIgnoreArgs, x) ||
				!Resolve<float, float>(args, index, bAllowIgnoreArgs, y) ||
				!Resolve<float, float>(args, index, bAllowIgnoreArgs, z)) return false;
			outValue = Math::Vector3(x, y, z);
			return true;
		}

		// Vector4 Specialization
		template <>
		static bool Resolve<Math::Vector4, Math::Vector4>(TArrayView<StringView> args, int& index, bool bAllowIgnoreArgs, Math::Vector4& outValue)
		{
			float x, y, z, w;
			if (!Resolve<float, float>(args, index, bAllowIgnoreArgs, x) ||
				!Resolve<float, float>(args, index, bAllowIgnoreArgs, y) ||
				!Resolve<float, float>(args, index, bAllowIgnoreArgs, z) ||
				!Resolve<float, float>(args, index, bAllowIgnoreArgs, w)) return false;
			outValue = Math::Vector4(x, y, z, w);
			return true;
		}

		// Helper to resolve enum by name using Reflection (if available)
		template< typename T >
		static auto ResolveEnumName(StringView str, T& outValue, int) -> decltype(::TReflectEnumValueTraits<T>::GetValues(), bool())
		{
			for (auto const& info : ::TReflectEnumValueTraits<T>::GetValues())
			{
				if (str.compare(info.text) == 0) // || str.compare(info.overridedName) == 0)
				{
					outValue = (T)info.value;
					return true;
				}
			}
			return false;
		}

		// Fallback if Reflection is not available
		template< typename T >
		static bool ResolveEnumName(StringView str, T& outValue, ...)
		{
			return false;
		}

		// Enum Resolution
		template <typename TEnum>
		static typename std::enable_if< std::is_enum<TEnum>::value, bool >::type
			Resolve(TArrayView<StringView> args, int& index, bool bAllowIgnoreArgs, TEnum& outValue)
		{
			if (index >= args.size())
			{
				if (bAllowIgnoreArgs)
				{
					outValue = TEnum(0);
					return true;
				}
				return false;
			}
			StringView str = args[index];
			++index;

			if (str.length() > 1 && str[0] == '$')
			{
				std::string resolvedStr;
				int tempIndex = index - 1;
				if (ResolveString<char const*>(args, tempIndex, false, resolvedStr))
				{
					// Try resolve variable content as Enum Name
					if (ResolveEnumName(StringView(resolvedStr), outValue, 0))
						return true;

					// Try resolve variable content as Integer
					int intVal;
					if (FStringConv::ToCheck(resolvedStr.c_str(), resolvedStr.length(), intVal))
					{
						outValue = (TEnum)intVal;
						return true;
					}
				}
				return false;
			}

			// Try resolve as Enum Name
			if (ResolveEnumName(str, outValue, 0))
				return true;

			// Try resolve as Integer
			int intVal;
			if (FStringConv::ToCheck(str.data(), str.length(), intVal))
			{
				outValue = (TEnum)intVal;
				return true;
			}

			return false;
		}
	};


	template< typename TArgList >
	struct CommandArgsConverter;

	template< typename ...Args >
	struct CommandArgsConverter< Meta::TypeList<Args...> >
	{
		// Member Function Invoker
		template< size_t ...Is, class T, class TFunc >
		static bool Invoke(TFunc func, T* obj, TArrayView<StringView> args, bool bAllowIgnoreArgs, TIndexList<Is...>)
		{
			// Tuple holds STORAGE types (std::string instead of char*)
			std::tuple< typename ArgHolder<typename Meta::TypeTraits<Args>::BaseType>::Type... > convertedArgs;

			int currentIndex = 0;
			bool bValid = true;

			using Expand = int[];

			// Resolve into storage
			(void)Expand
			{
				0, (bValid = bValid && ArgHelper::Resolve<
					typename Meta::TypeTraits<Args>::BaseType,
					typename ArgHolder<typename Meta::TypeTraits<Args>::BaseType>::Type
				>(args, currentIndex, bAllowIgnoreArgs, std::get<Is>(convertedArgs)), 0)...
			};

			if (!bValid)
				return false;

			// Pass to function using ArgPasser to unwrap (std::string -> char*)
			(obj->*func)(ArgPasser<typename Meta::TypeTraits<Args>::BaseType>::Get(std::get<Is>(convertedArgs))...);
			return true;
		}

		// Static Function Invoker
		template< size_t ...Is >
		static bool Invoke(void(*func)(Args...), TArrayView<StringView> args, bool bAllowIgnoreArgs, TIndexList<Is...>)
		{
			std::tuple< typename ArgHolder<typename Meta::TypeTraits<Args>::BaseType>::Type... > convertedArgs;

			int currentIndex = 0;
			bool bValid = true;

			using Expand = int[];

			(void)Expand
			{
				0, (bValid = bValid && ArgHelper::Resolve<
					typename Meta::TypeTraits<Args>::BaseType,
					typename ArgHolder<typename Meta::TypeTraits<Args>::BaseType>::Type
				>(args, currentIndex, bAllowIgnoreArgs, std::get<Is>(convertedArgs)), 0)...
			};

			if (!bValid)
				return false;

			func(ArgPasser<typename Meta::TypeTraits<Args>::BaseType>::Get(std::get<Is>(convertedArgs))...);
			return true;
		}
	};
}



class ConsoleCommandBase
{
public:
	ConsoleCommandBase(char const* inName, TArrayView< ConsoleArgTypeInfo const > inArgs, uint32 flags = 0);
	virtual ~ConsoleCommandBase() {}

	std::string   mName;
	TArrayView< ConsoleArgTypeInfo const > mArgs;
	uint32 getFlags() { return mFlags; }
	uint32 mFlags;

	virtual bool execute(TArrayView<StringView> args) = 0;
	virtual void getValue(void* pDest) {}
	virtual bool isHoldObject(void* objectPtr) { return false; }
	virtual VariableConsoleCommadBase* asVariable() { return nullptr; }

};


template < class TFunc, class T = void >
struct TMemberFuncConsoleCommand : public ConsoleCommandBase
{
	TFunc  mFunc;
	T*     mObject;

	TMemberFuncConsoleCommand(char const* inName, TFunc inFunc, T* inObj = NULL, uint32 flags = 0)
		:ConsoleCommandBase(inName, TCommandFuncTraints<TFunc>::GetArgs(), flags)
		, mFunc(inFunc), mObject(inObj)
	{
	}

	virtual bool execute(TArrayView<StringView> args) override
	{
		using FuncTraits = Meta::FuncTraits<TFunc>;
		bool bAllowIgnoreArgs = !!(mFlags & CVF_ALLOW_IGNORE_ARGS);

		bool result = ConsoleDetail::CommandArgsConverter<typename FuncTraits::ArgList>::Invoke(
			mFunc, mObject, args, bAllowIgnoreArgs, TIndexSequence<FuncTraits::NumArgs>{}
		);

		if (!result)
		{
			//LogWarning(0, "Command Execution Failed (Invalid Args)");
		}
		return result;
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
	TBaseFuncConsoleCommand(char const* inName, TFunc inFunc, uint32 flags = 0)
		:ConsoleCommandBase(inName, TCommandFuncTraints<TFunc>::GetArgs(), flags)
		, mFunc(inFunc)
	{
	}

	virtual bool execute(TArrayView<StringView> args) override
	{
		using FuncTraits = Meta::FuncTraits<TFunc>;
		bool bAllowIgnoreArgs = !!(mFlags & CVF_ALLOW_IGNORE_ARGS);

		bool result = ConsoleDetail::CommandArgsConverter<typename FuncTraits::ArgList>::Invoke(
			mFunc, args, bAllowIgnoreArgs, TIndexSequence<FuncTraits::NumArgs>{}
		);

		if (!result)
		{
			//LogWarning(0, "Command Execution Failed (Invalid Args)");
		}
		return result;
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

	TVariableConsoleCommad(char const* inName, Type* inPtr, uint32 flags = 0)
		:VariableConsoleCommadBase(inName, GetArg(), flags)
		, mPtr(inPtr)
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
		return TArrayView< ConsoleArgTypeInfo const >(&sArg, 1);
	}
	virtual bool execute(TArrayView<StringView> args) override
	{
		if (args.size() == 0)
			return false;
		return FStringConv::ToCheck(args[0].data(), (int)args[0].length(), *mPtr);
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

	virtual bool execute(TArrayView<StringView> args) override
	{
		if (args.size() == 0)
			return false;
		*mPtr = args[0].toStdString();
		return true;
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

	virtual bool  setFromFloat(float inValue) { return false; }
	virtual float getAsFloat() const { return 0; }
};


template < class Type >
struct TVariableConsoleDelegateCommad : public VariableConsoleCommadBase
{
	using GetValueDelegate = std::function< Type(void) >;
	using SetValueDelegate = std::function< void(Type const& type) >;

	GetValueDelegate mGetValue;
	SetValueDelegate mSetValue;

	TVariableConsoleDelegateCommad(char const* inName, GetValueDelegate inGetValue, SetValueDelegate inSetValue, uint32 flags = 0)
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
		return formStringImpl(str, typename Meta::IsSameType< Type, char const* >::Type());
	}

	bool formStringImpl(StringView const& str, Meta::FalseType)
	{
		Type value;
		if (!FStringConv::ToCheck(str.data(), str.length(), value))
			return false;
		mSetValue(value);
		return true;
	}

	bool formStringImpl(StringView const& str, Meta::TureType)
	{
		mSetValue(str.toCString());
		return true;
	}

	static TArrayView< ConsoleArgTypeInfo const > GetArg()
	{
		static ConsoleArgTypeInfo const sArg = TCommandArgTypeTraits<Type>::GetInfo();
		return TArrayView< ConsoleArgTypeInfo const >(&sArg, 1);
	}

	virtual bool execute(TArrayView<StringView> args) override
	{
		if (args.size() == 0)
			return false;

		return setFromString(args[0]);
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


	virtual bool  registerAlias(char const* aliasName, char const* commandName) = 0;

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
	TConsoleVariable(T const& val, char const* name, uint32 flags = 0)
		:mValue(val)
	{
		auto command = new TVariableConsoleCommad<T>(name, &mValue, flags);
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
	TConsoleVariable& operator = (T const& val) { mValue = val;  return *this; }
	operator T() { return mValue; }

	TVariableConsoleCommad<T>* mCommand = nullptr;
	T mValue;
};

template< class T >
class TConsoleVariableDelegate
{
public:
	TConsoleVariableDelegate(
		typename TVariableConsoleDelegateCommad<T>::GetValueDelegate inGetValue,
		typename TVariableConsoleDelegateCommad<T>::SetValueDelegate inSetValue,
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

	template< class TFunc, class T >
	AutoConsoleCommand(char const* name, TFunc func, T* object)
	{
		mCommand = IConsoleSystem::Get().registerCommand(name, func, object);
	}

	~AutoConsoleCommand()
	{
		if (mCommand && IConsoleSystem::Get().isInitialized())
		{
			IConsoleSystem::Get().unregisterCommand(mCommand);
		}
	}

	ConsoleCommandBase* mCommand = nullptr;
};



class FConsole
{
public:
	static CORE_API bool ExecuteScript(char const* path);
};

#endif // ConsoleSystem_H_D2FC89F2_0A99_4F1A_89D4_DB18A8247D7C