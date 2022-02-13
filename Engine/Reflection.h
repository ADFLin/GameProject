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


	class IProporty
	{
	public:

	};


	template< class T >
	class NumericProporty : public IProporty
	{

	};

}


#endif // Reflection_h__
