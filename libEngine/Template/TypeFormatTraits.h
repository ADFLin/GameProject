#pragma once
#ifndef TypeFormatTraits_H_6B3580E8_D568_4EC7_9A6F_1FDE130FF8B5
#define TypeFormatTraits_H_6B3580E8_D568_4EC7_9A6F_1FDE130FF8B5

template <typename T>
struct TIsValidFormatType
{
private:
	static uint32 Tester(uint32);
	static uint32 Tester(uint16);
	static uint32 Tester(uint8);
	static uint32 Tester(int32);
	static uint32 Tester(uint64);
	static uint32 Tester(int64);
	static uint32 Tester(short);
	static uint32 Tester(double);
	static uint32 Tester(float);
	static uint32 Tester(long);
	static uint32 Tester(unsigned long);
	static uint32 Tester(char);
	static uint32 Tester(bool);
	static uint32 Tester(const void*);
	static uint8  Tester(...);

	static T DeclValT();

public:
	enum { Value = sizeof(Tester(DeclValT())) == sizeof(uint32) };
};

template< class T >
struct TTypeFormatTraits
{
	static char const* GetString();
};

#define DEFINE_TYPE_FORMAT( TYPE , STR )\
template<>\
struct TTypeFormatTraits< TYPE >\
{\
	enum { TypeSize = sizeof(TYPE) };\
	static char const* GetString() { return STR; }\
};

template< class CharT = char >
struct TTypeFormat
{
	static CharT const* GetString(unsigned) { return "%u"; }
	static CharT const* GetString(long unsigned) { return "%lu"; }
	static CharT const* GetString(uint8);
	static CharT const* GetString(int32);
	static CharT const* GetString(uint64);
	static CharT const* GetString(int64);
	static CharT const* GetString(short);
	static CharT const* GetString(double);
	static CharT const* GetString(float);
	static CharT const* GetString(long);
	static CharT const* GetString(char);
	static CharT const* GetString(bool);
	static CharT const* GetString(const void*);
};


DEFINE_TYPE_FORMAT(bool, "%d")
DEFINE_TYPE_FORMAT(float, "%g")
DEFINE_TYPE_FORMAT(double, "%lg")
DEFINE_TYPE_FORMAT(unsigned, "%u")
DEFINE_TYPE_FORMAT(long unsigned, "%lu")
DEFINE_TYPE_FORMAT(long long unsigned, "%llu")
DEFINE_TYPE_FORMAT(int, "%d")
DEFINE_TYPE_FORMAT(long int, "%ld")
DEFINE_TYPE_FORMAT(long long int, "%lld")
DEFINE_TYPE_FORMAT(unsigned char, "%u")
DEFINE_TYPE_FORMAT(char, "%d")
DEFINE_TYPE_FORMAT(char*, "%s")
DEFINE_TYPE_FORMAT(char const*, "%s")

#undef DEFINE_TYPE_FORMAT


#endif // TypeFormatTraits_H_6B3580E8_D568_4EC7_9A6F_1FDE130FF8B5
