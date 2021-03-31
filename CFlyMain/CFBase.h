#ifndef CFBase_h__
#define CFBase_h__

//#define  USE_PROFILE
#define CF_USE_D3D9   0
#define CF_USE_OPENGL 1
#define CF_USE_RENDERSYSTEM CF_USE_D3D9

#include "CompilerConfig.h"
#include "ProfileSystem.h"
#include "LogSystem.h"
#include "CppVersion.h"
#include "Core/IntegerType.h"
#include "HashString.h"

#define	CF_PROFILE( name ) PROFILE_ENTRY( name )
#define	CF_PROFILE2( name , flag ) PROFILE_ENTRY2( name , flag )

#include <cassert>
#include <algorithm>
#include <exception>
#include <string>
#include <vector>
#include <map>
#include <list>

#if ( CF_USE_RENDERSYSTEM == CF_USE_D3D9 )
#	define CF_RENDERSYSTEM_D3D9
#	define WIN32_LEAN_AND_MEAN
#	define NOMINMAX
#	include <Windows.h>
#	include <D3d9.h>
#	include <D3dx9core.h>
//#	include <d3dx9effect.h>
typedef IDirect3DDevice9            D3DDevice;
typedef IDirect3DVertexBuffer9      D3DVertexBuffer;
typedef IDirect3DIndexBuffer9       D3DIndexBuffer;
typedef IDirect3DTexture9           D3DTexture;
typedef IDirect3DVertexDeclaration9 D3DVertexDecl;
#elif ( CF_USE_RENDERSYSTEM == CF_USE_OPENGL )
#	define CF_RENDERSYSTEM_OPENGL
#	define WIN32_LEAN_AND_MEAN
#	define NOMINMAX
#	include <Windows.h>
#   inculde <GL/GL.h>
#   inculde <GL/GLU.h>
#else
#error
#endif //CF_USE_RENDERSYSTEM


#ifndef BIT
#	define BIT( n ) ( 1 << (n) )
#endif

#undef  ARRAYSIZE
#define ARRAYSIZE( ar ) ( sizeof(ar) / sizeof( ar[0] ) )

namespace CFly
{
	class RenderSystem;

	class World;
	class Viewport;
	class Material;
	class Texture;
	class Scene;
	class Object;
	class Sprite;
	class Camera;
	class Light;
	class Actor;

	typedef std::string String;

	template < class C >
	class VectorTypeIterWraper
	{
	public:
		typedef typename C::iterator   iterator;
		typedef typename C::value_type value_type;
		VectorTypeIterWraper( C& c )
			:cur( c.begin() ) , end( c.end() ){}

		value_type&  get(){ return *cur; }
		bool         haveMore(){ return cur != end; }
		void         next(){ ++cur; }

	private:
		iterator cur;
		iterator end;
	};

	class CFException : public std::exception
	{
	public:
		STD_EXCEPTION_CONSTRUCTOR_WITH_WHAT( CFException )
	};

}//namespace CFly

#define CF_FAIL_ID  -1

#include "RefCount.h"
#include "CFColor.h"
#include "CFEntity.h"
#include "CFMath.h"
#include "CFVector3.h"
#include "CFQuaternion.h"
#include "CFMatrix4.h"
#include "CFMatrix3.h"

#define  CF_SAFE_RELEASE( p ) { p->release(); p = NULL; }

#endif // CFBase_h__