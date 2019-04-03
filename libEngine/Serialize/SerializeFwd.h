#pragma once
#ifndef SerializeFwd_H_A7297718_219D_4AB8_8139_11C3B1DDAC5F
#define SerializeFwd_H_A7297718_219D_4AB8_8139_11C3B1DDAC5F

#include "MetaBase.h"

SUPPORT_BINARY_OPERATOR(HaveSerializerOutput, operator<<, &, const&);
SUPPORT_BINARY_OPERATOR(HaveSerializerInput, operator >> , &, &);
SUPPORT_BINARY_OPERATOR(HaveBitDataOutput, operator<<, &, const &);
SUPPORT_BINARY_OPERATOR(HaveBitDataInput, operator >> , &, &);

template< class T >
struct TTypeSupportSerializeOPFunc
{ 
	enum { Value = 0, };
};

#define TYPE_SUPPORT_SERIALIZE_FUNC( TYPE )\
	template<>\
	struct ::TTypeSupportSerializeOPFunc<TYPE> { enum { Value = 1, }; };

template< class T >
struct TTypeSerializeAsRawData
{
	enum { Value = 0, };
};

#define TYPE_SERIALIZE_AS_RAW_DATA( TYPE )\
	template<>\
	struct ::TTypeSerializeAsRawData< TYPE > { enum { Value = 1, }; };

#define TYPE_SERIALIZE_AS_RAW_DATA_T( TYPE )\
	struct ::TTypeSerializeAsRawData< TYPE > { enum { Value = 1, }; };

#endif // SerializeFwd_H_A7297718_219D_4AB8_8139_11C3B1DDAC5F
