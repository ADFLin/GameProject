#pragma once
#ifndef ReflectionCollect_H_990CAAB9_A6AF_48D5_B6A2_79DDB78AE3DD
#define ReflectionCollect_H_990CAAB9_A6AF_48D5_B6A2_79DDB78AE3DD

class ReflectionCollector
{
	template< class T >
	void beginClass(char const* name);

	template< typename T , typename Base >
	void addBaseClass();

	template< class T >
	void addProperty(T var, char const* name);

	void endClass();
};

#define REFLECT_OBJECT_BEGIN( CLASS )\
	template< class ReflectionCollector >\
	static void CollectReflection(ReflectionCollector& collector)\
	{\
		using ThisClass = CLASS;\
		collector.beginClass< ThisClass >( #CLASS );

#define REF_BASE_CLASS( BASE )\
		collector.addBaseClass< ThisClass , BASE >();

#define REF_PROPERTY( VAR )\
		collector.addProperty( &ThisClass::VAR , #VAR );

#define REFLECT_OBJECT_END()\
		collector.endClass();\
	}


#endif // ReflectionCollect_H_990CAAB9_A6AF_48D5_B6A2_79DDB78AE3DD

