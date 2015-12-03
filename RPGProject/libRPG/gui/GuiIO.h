#ifndef GuiIO_h__
#define GuiIO_h__


#define GUI_INLINE __forceinline

#include <boost/type_traits/is_enum.hpp>
#include <boost/type_traits/is_array.hpp>

namespace GuiIO {

	typedef ::boost::true_type  true_type;
	typedef ::boost::false_type false_type;

	template < class T , class P , class Q >
	struct Selector
	{
		typedef P result_type;
	};
	template < class P , class Q >
	struct Selector< false_type , P , Q >
	{
		typedef Q result_type;
	};

	template < class P , class T1 , class T2 ,class T3 >
	struct Selector< false_type , P , Selector< T1 , T2 , T3 > >
	{
		typedef typename Selector<T1,T2,T3>::result_type result_type;
	};

	template <  class Platform  >
	class OutputEval : public Platform
	{
	public:
		typedef true_type  is_output_type;
		typedef false_type is_input_type;

		template < class Stream >
		GUI_INLINE OutputEval&  operator << ( Stream& stream )
		{
			return (*this) & stream;
		}

		template < class Stream >
		GUI_INLINE OutputEval&  operator >> ( Stream& stream )
		{
			return *this;
		}

		template < class GuiStream >
		GUI_INLINE OutputEval&  operator & ( GuiStream&  stream  )
		{
			stream.exec( *this );
			return *this;
		}

	};

	template <  class Platform  >
	class InputEval : public Platform
	{
	public:
		typedef true_type  is_input_type;
		typedef false_type is_output_type;

		template < class GuiStream >
		GUI_INLINE InputEval&  operator << ( GuiStream& stream )
		{
			return *this;
		}

		template < class GuiStream >
		GUI_INLINE InputEval&  operator >> ( GuiStream& stream )
		{
			return (*this) & stream;
		}

		template < class GuiStream >
		GUI_INLINE InputEval&  operator & ( GuiStream&  stream  )
		{
			stream.exec( *this );
			return *this;
		}
	};



	struct NormalInvoke
	{
		template < class Platform , class Gui , class Type >
		GUI_INLINE NormalInvoke( OutputEval<Platform>& eval , Gui& gui , Type&  val )
		{
			eval.output( gui , val );
		}

		template < class Platform , class Gui , class Type >
		GUI_INLINE NormalInvoke( InputEval<Platform>& eval , Gui& gui , Type&  val )
		{
			eval.input( gui , val );
		}
	};

	struct EnumInvoke
	{
		template < class Platform , class Gui , class Type >
		GUI_INLINE EnumInvoke( OutputEval<Platform>& eval , Gui& gui , Type&  val )
		{
			eval.output( gui , (int)val );
		}

		template < class Platform , class Gui , class Type >
		GUI_INLINE EnumInvoke( InputEval<Platform>& eval , Gui& gui , Type&  val )
		{
			int v;
			eval.input( gui , v );
			val = (Type) v;
		}
	};


	struct ArrayInvoke
	{
		template < class Platform , class Gui , class Type >
		GUI_INLINE ArrayInvoke( OutputEval<Platform>& eval , Gui& gui , Type&  val )
		{
			eval.output( gui , val );
		}

		template < class Platform , class Gui , class Type >
		GUI_INLINE ArrayInvoke( InputEval<Platform>& eval , Gui& gui , Type&  val )
		{
			eval.input( gui , val );
		}
	};

	template < class EvalType , class Gui , class Type >
	GUI_INLINE void evalInvoke( EvalType& eval , Gui& gui , Type& val )
	{
		Selector< 
			boost::is_enum< Type >::type , EnumInvoke ,
			NormalInvoke
		>::result_type invoke( eval, gui , val );
	}


	template < class Gui , class  T > 
	class Stream
	{
	public:
		Stream( Gui& w , T& v ):gui(w),val(v){}

		template < class Eval >
		GUI_INLINE void exec( Eval& eval ){ evalInvoke( eval , gui , val ); }
		T&   val;
		Gui& gui;
	};

	template < class T > 
	class BaseFun
	{
	public:
		typedef T (*Fun)();
		BaseFun( Fun f ):fun(f){}
		GUI_INLINE T  call(){  return (*fun)();  }

		template < class Eval >
		GUI_INLINE void exec( Eval& ){ call(); }
		Fun  fun;
	};

	template <  class Obj , class T > 
	class MemberFun
	{
	public:
		typedef T (Obj::*Fun)();
		MemberFun( Fun f, Obj* o ):fun(f),obj(o){}
		GUI_INLINE T  call(){  return (obj->*fun)();  }

		template < class Eval >
		GUI_INLINE void exec( Eval& ){ call(); }
		Fun  fun;
		Obj* obj;
	};

	template <  class Obj > 
	class MemberFun< Obj , void >
	{
	public:
		typedef void (Obj::*Fun)();

		MemberFun( Fun f, Obj* o ):fun(f),obj(o){}
		GUI_INLINE void call(){  return (obj->*fun)();  }

		template < class Eval >
		GUI_INLINE void exec( Eval& ){ call(); }
		Fun  fun;
		Obj* obj;
	};

	template < class Gui , class Obj , class T > 
	class StreamInputFun : public MemberFun< Obj , T >
	{
	public:
		StreamInputFun( Gui& g , Fun f, Obj* o )
			:MemberFun( f ,o ),gui(g){}
		Gui& gui;
		template< class Platform >
		GUI_INLINE void exec( OutputEval< Platform >& eval ){  evalInvoke( eval , gui , call() ); }
		template< class Platform >
		GUI_INLINE void exec( InputEval< Platform >& ){ }
	};

	template < class Gui , class Obj , class T , class RT > 
	class StreamOutputFun
	{
	public:
		typedef RT (Obj::*Fun)( T );
		StreamOutputFun( Gui& g , Fun f, Obj* o )
			:gui(g),fun(f),obj(o){}
		Gui& gui;
		Fun  fun;
		Obj* obj;
		template< class Platform >
		GUI_INLINE void exec( OutputEval< Platform >& ){}
		template< class Platform >
		GUI_INLINE void exec( InputEval< Platform >& eval )
		{
			::boost::remove_reference<
				::boost::remove_const< T >::type
			>::type val;
			evalInvoke( eval , gui , val );
			(obj->*fun)( val );
		}
	};

	template < class Gui , class  T > 
	GUI_INLINE Stream< Gui , T > 
	GuiStream( Gui& gui , T& val )
	{
		return Stream< Gui , T >( gui , val );
	}

	template < class Gui , class Obj , class T > 
	GUI_INLINE StreamInputFun< Gui , Obj , T > 
	GuiStream( Gui& gui , Obj* obj , T (Obj::*fun )() )
	{
		return StreamInputFun< Gui , Obj , T >( gui , fun , obj  );
	}

	template < class Gui , class Obj , class T , class RT  > 
	GUI_INLINE StreamOutputFun< Gui , Obj , T , RT > 
	GuiStream( Gui& gui , Obj* obj , RT (Obj::*fun)( T ) )
	{
		return StreamOutputFun< Gui , Obj , T , RT >( gui , fun , obj );
	}

	template < class Obj , class T > 
	GUI_INLINE MemberFun< Obj , T > 
	GuiStream( Obj* obj , T (Obj::*fun )() )
	{
		return MemberFun< Obj , T >( fun , obj );
	}




} //namespace GuiIO 

#endif // GuiIO_h__
