#include "StageRegister.h"
#include "reflcpp/refl.hpp"

#include <iostream>


template < typename T, typename ...TArgs >
void packWarpper(T& packer, TArgs& ...args)
{
	packer.processStruct(args...);
}

#define  REFLECT_STRUCT_BEGIN(CLASS)\
	template < typename T >\
	void pack(T& packer){\
		packWarpper(packer, \

#define REF_PROPERTY(VAR) ,VAR

#define REFLECT_STRUCT_END()\
		);\
	}


struct Data
{
	int a;
	float b;

	REFLECT_STRUCT_BEGIN(Data)
		REF_PROPERTY(a)
		REF_PROPERTY(b)
	REFLECT_STRUCT_END()
};


struct Foo
{
	int pa;
	float pb;
};

class IField
{


};

REFL_AUTO(
	type(Foo),
	field(pa),
	field(pb)
)


template <typename T>
constexpr void PrintProperty() 
{
	// Get the type descriptor for T
	constexpr auto type = refl::reflect<T>();

	LogMsg("Class Name %s", get_display_name(type));
	// Get the compile-time refl::type_list<...> of member descriptors
	constexpr auto members = get_members(type);
	// Filter out the non-readable members (not field or getters marked with the property() attribute)
	constexpr auto readableMembers = filter(members, [](auto member) { return is_readable(member); });

	for_each(members, [&](auto member)
	{
		// get_display_name prefers the friendly_name of the property over the function name
		LogMsg("Member Name %s", get_display_name(member));
	});

}

void TypeReflectTest()
{
	PrintProperty<Foo>();
}

REGISTER_MISC_TEST_ENTRY("Type Reflect", TypeReflectTest);

/** An attribute for specifying a DB table's properties */
struct Table : refl::attr::usage::type
{
	const char* name;

	constexpr Table(const char* name) noexcept
		: name(name)
	{
	}
};

enum class DataType
{
	ID,
	TEXT,
};

/** An attribute for specifying a DB column's properties */
struct Column : refl::attr::usage::field
{
	const char* name;
	const DataType data_type;

	constexpr Column(const char* name, DataType dataType) noexcept
		: name(name), data_type(dataType)
	{ }
};

/** User will serve as the "model" for the "Users table" */
struct User
{
	std::uint32_t id;
	std::string email;
};

/** Define the reflection metadata for the model */
REFL_AUTO(
	type(User, Table{ "Users" }),
	field(id, Column{ "ID", DataType::ID }),
	field(email, Column{ "Email", DataType::TEXT })
);

template <typename Member>
constexpr auto make_sql_field_spec(Member)
{
	using namespace refl;

	constexpr auto col = descriptor::get_attribute<Column>(Member{});

	if constexpr (DataType::ID == col.data_type) {
		// convert the const char* data member to a refl::const_string<N>
		// (necessary to be able to do compile-time string concat)
		return REFL_MAKE_CONST_STRING(col.name) + " int PRIMARY KEY";
	}
	else if constexpr (DataType::TEXT == col.data_type) {
		return REFL_MAKE_CONST_STRING(col.name) + " TEXT";
	}
}

/**
 * Creates a CREATE TABLE SQL query.
 * Returns the query as a refl::const_string<N>.
 */
template <typename T>
constexpr auto make_sql_create_table()
{
	using namespace refl;
	using Td = type_descriptor<T>;

	// Verify that the type has the Table attribute.
	static_assert(descriptor::has_attribute<Table>(Td{}));

	// Concatenate all the members' column definitions together
	constexpr auto fields = accumulate(Td::members,
		[](auto acc, auto member) {
		return acc + ",\n\t" + make_sql_field_spec(member);
	}, make_const_string())
		.template substr<2>(); // remove the initial ",\n"

	constexpr auto tbl = descriptor::get_attribute<Table>(Td{});
	// convert the const char* data member to a refl::const_string<N>
	// (necessary to be able to do compile-time string concat)
	constexpr auto tbl_name = REFL_MAKE_CONST_STRING(tbl.name);

	// return the resulting compile-time string
	return "CREATE TABLE " + tbl_name + " (\n" + fields + "\n);";
}

void DAOTest()
{
	// sql is of type const_string<N> where N is the length of the
	// compile-time string.
	constexpr auto sql = make_sql_create_table<User>();
	std::cout << sql << "\n";
	std::cout << "Number of characters: " << sizeof(sql) - 1 << "\n";
}
REGISTER_MISC_TEST_ENTRY("DAO Reflect", DAOTest);
