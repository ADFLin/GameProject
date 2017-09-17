#ifndef Visitor_h__
#define Visitor_h__

namespace BT
{
	class BaseVisitor
	{
	public:
		virtual ~BaseVisitor(){}
	};
	
	template< class T , class R = void >
	class Visitor
	{
	public:
		virtual R visit( T& ) = 0;
	};
	
	template< class Ret >
	class BaseVisitable
	{
	public:
		typedef Ret ReturnType;
		virtual ReturnType accept( BaseVisitor& visitor ){ return Ret(); }
	
	protected:
		template< class T >
		static ReturnType acceptImpl( T& visited , BaseVisitor& visitor )
		{
			typedef Visitor< T , ReturnType > VisitorType;
			if ( VisitorType* p = dynamic_cast< VisitorType* >( &visitor ) )
			{
				return p->visit( visited );
			}
		}
	
	#define DEFNE_VISITABLE()\
		ReturnType accept( BaseVisitor& visitor )\
		{\
			return acceptImpl( *this , visitor );\
		}
	};

}//namespace BT

#endif // Visitor_h__