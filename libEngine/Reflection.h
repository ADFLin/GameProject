#ifndef Reflection_h__
#define Reflection_h__

namespace Reflection
{

	class Object
	{




	};

	class IClass
	{
	public:
		IClass* super;
		virtual Object* create(){}
	};


	template< class T >
	class ReflectSupportT
	{
	public:
		static IClass* getClass(){}
		void    registerClass()
		{
	
		}




	};


	class IProporty
	{
	public:
		IProporty* next;







	};
#define REF_PROPERTY_VAR( NAME , VAR )


	template< class T >
	class NumericProporty : public IProporty
	{

	};




}

#endif // Reflection_h__
