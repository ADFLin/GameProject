#pragma once
#ifndef Concept_H_DE28E8DF_9819_454A_A691_58F960CEC90E
#define Concept_H_DE28E8DF_9819_454A_A691_58F960CEC90E


#if 0
struct TConcept
{	
	template< typename T , typename ...Args >
	static auto Requires(T& t) -> decltype ();
}
#endif

template< typename TConcept , typename ...Args >
struct TCheckConcept
{
	struct Yes { char a[2]; };

	template< typename ...Ts >
	static Yes  Tester(decltype(&TConcept::template Requires<Ts...>)*);
	template< typename ...Ts >
	static char Tester(...);

	static constexpr bool Value = sizeof(Tester<Args...>(0)) == sizeof(Yes);
};

template <typename Concept, typename... Args>
auto Refines() -> int(&)[!!TCheckConcept<Concept, Args...>::Value * 2 - 1];

#endif // Concept_H_DE28E8DF_9819_454A_A691_58F960CEC90E
