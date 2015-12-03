#ifndef CFColor_h__
#define CFColor_h__

#include "IntegerType.h"

namespace CFly
{
	class Color
	{
	public:
		static uint32 toARGB( float r , float g , float b , float a )
		{
			return toARGB( uint8( 255*r ) , uint8( 255*g ) , uint8( 255*b ) , uint8( 255*a ) );
		}
		static uint32 toRGBA( float r , float g , float b , float a )
		{
			return toRGBA( uint8( 255*r ) , uint8( 255*g ) , uint8( 255*b ) , uint8( 255*a ) );
		}
		static uint32 toXRGB( float r , float g , float b )
		{
			return toXRGB( uint8( 255*r ) , uint8( 255*g ) , uint8( 255*b ) );
		}

		static uint32 toARGB( uint8 r , uint8 g , uint8 b , uint8 a )
		{
			return ( a << 24 ) | ( r << 16 ) | ( g << 8 ) | b;
		}
		static uint32 toXRGB( uint8 r , uint8 g , uint8 b )
		{
			return ( 0xff << 24 ) | ( r << 16 ) | ( g << 8 ) | b;
		}
		static uint32 toRGBA( uint8 r , uint8 g , uint8 b , uint8 a )
		{
			return ( r << 24 ) | ( g << 16 ) | ( b << 8 ) | a;
		}
	};


	template < class T >
	struct ColorTypeTraits
	{

	};

	template <>
	struct ColorTypeTraits< float >
	{
		static float min(){ return 0.0f; }
		static float max(){ return 1.0f; }
	};
	template <>
	struct ColorTypeTraits< uint8 >
	{
		static uint8 min(){ return 0x00; }
		static uint8 max(){ return 0xff; }
	};
	template< class T >
	class Color3T
	{
	public:
		Color3T(){}
		Color3T( T cr ,T cg ,T cb )
			:r(cr),g(cg),b(cb){}
		Color3T( T const* v )
			:r(v[0]),g(v[1]),b(v[2]){}

		operator T const*() const { return &r; }
		operator T*      ()       { return &r; }
		T  r,g,b;
	};

	template< class T >
	class Color4T : public Color3T< T >
	{
	public:
		Color4T(){}
		Color4T( T cr ,T cg ,T cb ,T ca = ColorTypeTraits< T >::max() )
			:Color3T<T>( cr,cg,cb),a(ca){}
		Color4T( Color3T<T> const& rh )
			:Color3T<T>( rh ),a( ColorTypeTraits<T>::max() ){}
		Color4T( T const* v )
			:Color3T<T>( v ),a(v[3]){}

		Color4T& operator = ( Color3T< T > const& rhs )
		{
			r = rhs.r;
			g = rhs.g;
			b = rhs.b;
			a = ColorTypeTraits< T>::max();
			return *this;
		}

		operator T const*() const { return &r; }
		operator T*      ()       { return &r; }
		uint32 toARGB() const { return Color::toARGB( r , g , b , a ); }
		uint32 toRGBA() const { return Color::toRGBA( r , g , b , a ); }
		T a;
	};

#if 0

	class Color3ub : public Color3T< uint8 >
	{
	public:
		Color3ub(){}
		Color3ub( uint8 cr ,uint8 cg ,uint8 cb )
			:Color3T< uint8 >( cr, cg , cb ){}

	};

	class Color3f : public Color3T< float >
	{
	public:
		Color3f(){}
		Color3f( float cr , float cg , float cb )
			:Color3T< float >( cr , cg , cb ){}
	};



	class Color4ub : public Color4T< uint8 >
	{
	public:
		Color4ub(){}
		Color4ub( uint8 cr ,uint8 cg , uint8 cb ,uint8 ca = 0xff )
			:Color4T< uint8 >( cr , cg , cb , ca ){}
	};

	class Color4f : public Color4T< float >
	{
	public:
		Color4f(){}
		Color4f( float cr , float cg  , float cb  , float ca = 1.0f )
			:Color4T< float >( cr , cg , cb , ca ){}
	};
#else
	typedef Color4T< uint8 > Color4ub;
	typedef Color3T< uint8 > Color3ub;
	typedef Color4T< float > Color4f;
	typedef Color3T< float > Color3f;
#endif



}//namespace CFly



#endif // CFColor_h__