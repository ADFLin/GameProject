#pragma once
#ifndef ShaderPermutation_H_33C8D6A9_E78B_49EC_A728_200A5CC9B2CE
#define ShaderPermutation_H_33C8D6A9_E78B_49EC_A728_200A5CC9B2CE

#include "ShaderCore.h"
#include "Template/ArrayView.h"

namespace Render
{

	struct ShaderPermutationBool
	{
		using Type = bool;
		constexpr static uint32 GetPermutationCount() { return 2; }
		static Type   ToValue(uint32 id) { return bool(id); }
		static uint32 ToDimensionId(Type value) { return value ? 1 : 0; }
	};

	struct ShaderPermutationInt
	{
		using Type = int32;
	};

	template< int32 ValueMin , int32 ValueMax >
	struct TShaderPermutationInt : ShaderPermutationInt
	{
		constexpr static uint32 GetPermutationCount() { return ( ValueMax - ValueMin ) + 1; }
		static Type   ToValue(uint32 id) { return ValueMin + id; }
		static uint32 ToDimensionId(Type value)
		{
			value = Math::Clamp(value, ValueMin, ValueMax);
			return value - ValueMin; 
		}
	};

	template< int32 ...Values >
	struct TShaderPermutationIntSelections : ShaderPermutationInt
	{
		constexpr static uint32 GetPermutationCount() { return sizeof...(Values); }
		static TArrayView< int const > GetSelections()
		{
			static int StaticValues[] = { Values... };
			return TArrayView< int const >( StaticValues , ARRAY_SIZE(StaticValues));
		}
		static Type   ToValue(uint32 id) { return GetSelections()[id]; }
		static uint32 ToDimensionId(Type value)
		{
			auto selections = GetSelections();
			for (int index = 0; index < selections.size(); ++index)
			{
				if (value == selections[index])
					return index;
			}
			return 0;
		}
	};

#define SHADER_PERMUTATION_TYPE_BOOL( TypeName , Name )\
	struct TypeName : Render::ShaderPermutationBool\
	{\
		static constexpr char* DefineName =  Name;\
		Type value;\
	};

#define SHADER_PERMUTATION_TYPE_INT( TypeName , Name , Min , Max )\
	struct TypeName : Render::TShaderPermutationInt< Min , Max >\
	{\
		static constexpr char* DefineName =  Name;\
		Type value;\
	};

#define SHADER_PERMUTATION_TYPE_INT_COUNT( TypeName , Name , Count )\
	struct TypeName : Render::TShaderPermutationInt< 0 , (Count) - 1 >\
	{\
		static constexpr char* DefineName =  Name;\
		Type value;\
	};

#define SHADER_PERMUTATION_TYPE_INT_SELECTIONS( TypeName , Name , ... )\
	struct TypeName : Render::TShaderPermutationIntSelections< __VA_ARGS__ >\
	{\
		static constexpr char* DefineName =  Name;\
		Type value;\
	};

	template< typename ...Ts >
	class TShaderPermutationDomain;


	struct FShaderPermutation
	{
		template<typename T>
		static void SetupOption(T const& domain, ShaderCompileOption& option)
		{
			option.addDefine(T::DefineName, static_cast<T const&>(domain).value);
		}

		struct CDomainConvertable
		{
			template< typename ...Ts >
			static bool Test(TShaderPermutationDomain< Ts... >& value);

			template< typename T >
			static auto Requires(T& value) -> decltype
			(
				Test(value)
			);
		};


		template< typename TDomain, typename T, typename ...Ps >
		static uint32 GetPermutationCount()
		{
			return T::GetPermutationCount() * GetPermutationCount< TDomain , Ps... >();
		}
		template< typename TDomain>
		static uint32 GetPermutationCount(){  return 1;  }

		template< typename TDomain, typename T, typename ...Ps >
		static uint32 GetPermutationId(TDomain const& domain)
		{
			uint32 id;
			if constexpr (TCheckConcept< CDomainConvertable, T >::Value )
			{
				id = static_cast<T const&>(domain).getPermutationId();
			}
			else
			{
				id = T::ToDimensionId(static_cast<T const&>(domain).value);
			}

			return id + T::GetPermutationCount() * GetPermutationId<TDomain, Ps... >(domain);
		}
		template< typename TDomain>
		static uint32 GetPermutationId(TDomain const& domain){  return 0;  }

		template<typename TDomain, typename T, typename ...Ps >
		static void SetPermutationId(TDomain& domain, uint32 id)
		{
			if constexpr (TCheckConcept< CDomainConvertable, T >::Value)
			{
				static_cast<T&>(domain).setPermutationId(id % T::GetPermutationCount());
			}
			else
			{
				static_cast<T&>(domain).value = T::ToValue(id % T::GetPermutationCount());
			}

			SetPermutationId<TDomain, Ps... >(domain, id / T::GetPermutationCount());
		}
		template<typename TDomain>
		static void SetPermutationId(TDomain& domain, uint32 id){}

		template<typename TDomain, typename T, typename ...Ps >
		static void SetupShaderCompileOption(TDomain const& domain, ShaderCompileOption& option)
		{
			if constexpr (TCheckConcept< CDomainConvertable, T >::Value)
			{
				static_cast<T const&>(domain).setupShaderCompileOption(option);
			}
			else
			{
				option.addDefine(T::DefineName, static_cast<T const&>(domain).value);
			}

			SetupShaderCompileOption<TDomain, Ps... >(domain, option);
		}
		template<typename TDomain>
		static void SetupShaderCompileOption(TDomain const& domain, ShaderCompileOption& option){}

		template< typename TDomain, typename PFind, typename T, typename ...Ps >
		static PFind& GetPermutation(TDomain& domain)
		{
			if constexpr (Meta::IsSameType<T, PFind >::Value)
				return static_cast<PFind&>(domain);

			if constexpr (TCheckConcept< CDomainConvertable, T >::Value)
			{
				if constexpr (T::ContainPermutation<PFind>())
				{
					return static_cast<T&>(domain).getPermutation<PFind>();
				}
			}
			return GetPermutation<TDomain, PFind, Ps... >(domain);
		}
		template< typename TDomain, typename PFind>
		static PFind& GetPermutation(TDomain const& domain) { return *static_cast<PFind*>( nullptr ); }


		template< typename TDomain, typename PFind, typename T, typename ...Ps >
		constexpr static bool ContainPermutation()
		{
			if constexpr (Meta::IsSameType<T, PFind >::Value)
				return true;

			if constexpr (TCheckConcept< CDomainConvertable, T >::Value)
			{
				if constexpr (T::ContainPermutation<PFind>())
					return true;
			}
			return ContainPermutation<TDomain, PFind, Ps... >();
		}
		template< typename TDomain, typename PFind>
		constexpr static bool ContainPermutation() { return false; }
	};

	template< typename ...Ts >
	class TShaderPermutationDomain : public Ts...
	{
	public:
		using Type = TShaderPermutationDomain< Ts... >;

		template< typename P >
		void set(typename P::Type value)
		{
			static_assert(ContainPermutation<P>());
			FShaderPermutation::GetPermutation<Type, P, Ts...>(*this).value = value;
		}
		template< typename P >
		auto get() const
		{
			return FShaderPermutation::GetPermutation<Type, P, Ts...>(const_cast<TShaderPermutationDomain&>(*this)).value;
		}

		template< typename P >
		P& getPermutation()
		{
			static_assert(ContainPermutation<P>());
			return FShaderPermutation::GetPermutation<Type, P, Ts...>(*this);
		}

		template< typename P >
		constexpr static bool ContainPermutation()
		{
			return FShaderPermutation::ContainPermutation<Type, P, Ts... >();
		}
		constexpr static uint32 GetPermutationCount()
		{
			return FShaderPermutation::GetPermutationCount<Type, Ts... >();
		}

		uint32 getPermutationId() const
		{
			return FShaderPermutation::GetPermutationId<Type, Ts... >(*this);
		}

		void setPermutationId(uint32 id)
		{
			FShaderPermutation::SetPermutationId<Type, Ts... >(*this, id);
		}

		void setupShaderCompileOption(ShaderCompileOption& option) const
		{
			FShaderPermutation::SetupShaderCompileOption<Type, Ts... >(*this, option);
		}

	};

}//namespace Render
#endif // ShaderPermutation_H_33C8D6A9_E78B_49EC_A728_200A5CC9B2CE
