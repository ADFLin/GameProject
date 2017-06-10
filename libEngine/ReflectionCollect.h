#pragma once
#ifndef ReflectionCollect_H_990CAAB9_A6AF_48D5_B6A2_79DDB78AE3DD
#define ReflectionCollect_H_990CAAB9_A6AF_48D5_B6A2_79DDB78AE3DD

class ReflectionCollector
{
	template< class T >
	void beginClass(char const* name);
	template< class T >
	void addProperty(T var, char const* name);
};

#define REF_OBJECT_COLLECTION_BEGIN( CLASS )\
	template< class ReflectionCollector >\
	static void CollectReflection(ReflectionCollector& collector)\
	{\
		typedef CLASS ThisClass;\
		collector.beginClass< ThisClass >( #CLASS );

//#define REF_OBJECT_COLLECTION_WITH_BASE_BEGIN( CLASS , BASE )

#define REF_PROPERTY( VAR )\
		collector.addProperty( &ThisClass::VAR , #VAR );

#define REF_OBJECT_COLLECTION_END()\
	}


#endif // ReflectionCollect_H_990CAAB9_A6AF_48D5_B6A2_79DDB78AE3DD

