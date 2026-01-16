#pragma once
#ifndef ExpressionCompiler_H_536C0F45_91A2_4182_B5BF_1BBCB21BE035
#define ExpressionCompiler_H_536C0F45_91A2_4182_B5BF_1BBCB21BE035

#include "ExpressionParser.h"
#include "Core/IntegerType.h"
#include <variant>
#include <type_traits>
#include <tuple>

#define ENABLE_ASM_CODE 1
#define ENABLE_BYTE_CODE 1
#define ENABLE_INTERP_CODE 1

#if ENABLE_ASM_CODE
#include "Backend/AsmCode.h"
#endif 
#if ENABLE_BYTE_CODE
#include "Backend/ExprByteCode.h"
#endif
#if ENABLE_INTERP_CODE
#include "ExpressionUtils.h"
#endif

class ExpressionCompiler;

// 定義執行類型
enum class ECodeExecType : uint8
{
	None = 0,
	Asm,
	ByteCode,
	Interp,
};

struct CodeDataEmpty {};

namespace ExprInternal
{
	template< typename RT, typename ...Args >
	FORCEINLINE RT CallAsm(void const* ptr, Args... args)
	{
#if TARGET_PLATFORM_64BITS
		if constexpr (std::is_same_v<RT, FloatVector>)
		{
			using FuncPtr = __m128(*)(Args...);
			return (RT)reinterpret_cast<FuncPtr>(const_cast<void*>(ptr))(args...);
		}
		else
#endif
		{
			if constexpr (std::is_floating_point_v<RT>)
			{
				// ASM backends (SSE scalar/FPU) typically return results in double precision 
				// or treat scalar floats as doubles in the return register.
				// Forcing double return and casting back to RT prevents bit-interpretation errors on x64.
				using FuncPtr = double(*)(Args...);
				return (RT)reinterpret_cast<FuncPtr>(const_cast<void*>(ptr))(args...);
			}
			else
			{
				using FuncPtr = RT(*)(Args...);
				return reinterpret_cast<FuncPtr>(const_cast<void*>(ptr))(args...);
			}
		}
	}
}

template< typename TData > struct TCodeExecutor;

template<>
struct TCodeExecutor<CodeDataEmpty>
{
	using DataType = CodeDataEmpty;
	TCodeExecutor(CodeDataEmpty const&) {}
	template<typename RT, typename ...Args>
	RT eval(Args...) const { return RT{}; }
};

#if ENABLE_ASM_CODE
template<>
struct TCodeExecutor<AsmCodeBuffer>
{
	using DataType = AsmCodeBuffer;
	AsmCodeBuffer const& mData;
	TCodeExecutor(AsmCodeBuffer const& data) : mData(data) {}

	template<typename RT, typename ...Args>
	FORCEINLINE RT eval(Args ...args) const
	{
		return ExprInternal::CallAsm<RT>(mData.getCode(), args...);
	}
};
#endif

#if ENABLE_BYTE_CODE
template<>
struct TCodeExecutor<ExprByteCodeExecData>
{
	using DataType = ExprByteCodeExecData;
	ExprByteCodeExecData const& mData;
	TCodeExecutor(ExprByteCodeExecData const& data) : mData(data) {}

	template<typename RT, typename ...Args>
	FORCEINLINE RT eval(Args ...args) const
	{
		TByteCodeExecutor<RT> executor;
		if constexpr (sizeof...(args) == 1)
		{
			using FirstArg = std::tuple_element_t<0, std::tuple<Args...>>;
			if constexpr (std::is_same_v<std::decay_t<FirstArg>, TArrayView<RT const>> ||
				          std::is_same_v<std::decay_t<FirstArg>, TArrayView<RT>> ||
				          std::is_same_v<std::decay_t<FirstArg>, TArrayView<RealType const>> ||
				          std::is_same_v<std::decay_t<FirstArg>, TArrayView<RealType>>)
			{
				return executor.execute(mData, args...);
			}
			else
			{
				return executor.execute(const_cast<ExprByteCodeExecData&>(mData), args...);
			}
		}
		else
		{
			return executor.execute(mData, args...);
		}
	}
};
#endif

#if ENABLE_INTERP_CODE
template<>
struct TCodeExecutor<TArray<ExprParse::Unit>>
{
	using DataType = TArray<ExprParse::Unit>;
	TArray<ExprParse::Unit> const& mData;
	TCodeExecutor(TArray<ExprParse::Unit> const& data) : mData(data) {}

	template<typename RT, typename ...Args>
	FORCEINLINE RT eval(Args ...args) const
	{
		auto evalInterp = [&](auto... vals) -> RT
		{
			if constexpr (sizeof...(vals) == 1)
			{
				using FirstArg = std::tuple_element_t<0, std::tuple<decltype(vals)...>>;
				if constexpr (std::is_same_v<std::decay_t<FirstArg>, TArrayView<RT const>> ||
					          std::is_same_v<std::decay_t<FirstArg>, TArrayView<RT>> ||
					          std::is_same_v<std::decay_t<FirstArg>, TArrayView<RealType const>> ||
					          std::is_same_v<std::decay_t<FirstArg>, TArrayView<RealType>>)
				{
					return FExpressUtils::template EvalutePosfixCodes<RT>(mData, vals...);
				}
			}
			return FExpressUtils::template EvalutePosfixCodes<RT>(mData, vals...);
		};

		if constexpr (std::is_same_v<RT, FloatVector>)
		{
			RT result;
			for (int i = 0; i < RT::Size; ++i)
			{
				auto get_arg = [&](auto const& arg) -> RealType {
					using TArg = std::decay_t<decltype(arg)>;
					if constexpr (std::is_same_v<TArg, FloatVector>)
						return arg[i];
					else if constexpr (std::is_convertible_v<TArg, RealType>)
						return (RealType)arg;
					else
						return 0; // TArrayView case for FloatVector eval is not supported
				};
				result[i] = FExpressUtils::template EvalutePosfixCodes<RealType>(mData, get_arg(args)...);
			}
			return result;
		}
		else
		{
			return evalInterp(args...);
		}
	}
};
#endif

class ExecutableCode
{
public:
	using CodeVariant = std::variant<
		CodeDataEmpty
#if ENABLE_ASM_CODE
		, AsmCodeBuffer
#endif
#if ENABLE_BYTE_CODE
		, ExprByteCodeExecData
#endif
#if ENABLE_INTERP_CODE
		, TArray<ExprParse::Unit>
#endif
	>;

	explicit ExecutableCode(int size = 0);
	~ExecutableCode();

#if ENABLE_BYTE_CODE || ENABLE_ASM_CODE
	static bool constexpr IsSupportSIMD = true;
#else
	static bool constexpr IsSupportSIMD = false;
#endif

	bool isSupportSIMD() const { return mbIsSIMD; }
	void setSIMD(bool bSIMD) { mbIsSIMD = bSIMD; }

	ECodeExecType getActiveType() const
	{
		if (std::holds_alternative<CodeDataEmpty>(mData))
			return ECodeExecType::None;
#if ENABLE_ASM_CODE
		if (std::holds_alternative<AsmCodeBuffer>(mData))
			return ECodeExecType::Asm;
#endif
#if ENABLE_BYTE_CODE
		if (std::holds_alternative<ExprByteCodeExecData>(mData))
			return ECodeExecType::ByteCode;
#endif
#if ENABLE_INTERP_CODE
		if (std::holds_alternative<TArray<ExprParse::Unit>>(mData))
			return ECodeExecType::Interp;
#endif
		return ECodeExecType::None;
	}


#if ENABLE_ASM_CODE
	AsmCodeBuffer& initAsm()
	{
		mData.emplace<AsmCodeBuffer>();
		return std::get<AsmCodeBuffer>(mData);
	}
#endif

#if ENABLE_BYTE_CODE
	ExprByteCodeExecData& initByteCode()
	{
		mData.emplace<ExprByteCodeExecData>();
		return std::get<ExprByteCodeExecData>(mData);
	}
#endif

#if ENABLE_INTERP_CODE
	TArray<ExprParse::Unit>& initInterp()
	{
		mData.emplace<TArray<ExprParse::Unit>>();
		return std::get<TArray<ExprParse::Unit>>(mData);
	}
#endif

	template<typename TFunc>
	FORCEINLINE auto visit(TFunc&& func) const
	{
		return std::visit([&](auto&& data) -> decltype(auto)
		{
			using T = std::decay_t<decltype(data)>;
			return func(TCodeExecutor<T>(data));
		}, mData);
	}

	template<typename TExec>
	FORCEINLINE TExec getExecutor() const
	{
		return TExec(std::get<typename TExec::DataType>(mData));
	}

#if ENABLE_ASM_CODE
	FORCEINLINE AsmCodeBuffer& getAsmData() { return std::get<AsmCodeBuffer>(mData); }
	FORCEINLINE AsmCodeBuffer const& getAsmData() const { return std::get<AsmCodeBuffer>(mData); }
#endif

#if ENABLE_BYTE_CODE
	FORCEINLINE ExprByteCodeExecData& getByteCodeData() { return std::get<ExprByteCodeExecData>(mData); }
	FORCEINLINE ExprByteCodeExecData const& getByteCodeData() const { return std::get<ExprByteCodeExecData>(mData); }

#if EBC_USE_VALUE_BUFFER
	template<typename T>
	void initValueBuffer(T buffer[])
	{
		std::get<ExprByteCodeExecData>(mData).initValueBuffer(buffer);
	}
#endif
#endif

#if ENABLE_INTERP_CODE
	FORCEINLINE TArray<ExprParse::Unit>& getInterpData() { return std::get<TArray<ExprParse::Unit>>(mData); }
	FORCEINLINE TArray<ExprParse::Unit> const& getInterpData() const { return std::get<TArray<ExprParse::Unit>>(mData); }
#endif

	//-------------------------------------------------------------------------
	template<typename RT, typename ...TArgs>
	RT evalT(TArgs ...args) const
	{
		return visit([&](auto const& executor) -> RT {
			return executor.template eval<RT>(args...);
		});
	}

	void clearCode() { mData.emplace<CodeDataEmpty>(); }

	int getCodeLength() const
	{
		return std::visit([](auto&& data) -> int
		{
			using T = std::decay_t<decltype(data)>;
			if constexpr (std::is_same_v<T, CodeDataEmpty>) return 0;
#if ENABLE_ASM_CODE
			else if constexpr (std::is_same_v<T, AsmCodeBuffer>) return data.getCodeLength();
#endif
#if ENABLE_BYTE_CODE
			else if constexpr (std::is_same_v<T, ExprByteCodeExecData>) return (int)data.codes.size();
#endif
#if ENABLE_INTERP_CODE
			else if constexpr (std::is_same_v<T, TArray<ExprParse::Unit>>) return (int)data.size();
#endif
			else return 0;
		}, mData);
	}

#if ENABLE_ASM_CODE
	AsmCodeBuffer& getBuffer() { return initAsm(); }
#endif

private:
	friend class ExpressionCompiler;
	CodeVariant mData;
	bool        mbIsSIMD = false;
};

class ExpressionCompiler
{
public:
	ExpressionCompiler();
	
	bool compile(
		char const* expr, 
		SymbolTable const& table, 
		ParseResult& parseResult, 
		ExecutableCode& data, 
		int numInput = 0, 
		ValueLayout inputLayouts[] = nullptr
	);

	void enableOpimization(bool enable = true) { mOptimization = enable; }
	void enableSIMD(bool bSIMD) { mbGenerateSIMD = bSIMD; }
	void setTargetType(ECodeExecType type) { mTargetType = type; }
	
protected:
	bool  mOptimization;
	bool  mbGenerateSIMD = false;
	ECodeExecType mTargetType = ECodeExecType::Asm;
};

#endif // ExpressionCompiler_H_536C0F45_91A2_4182_B5BF_1BBCB21BE035