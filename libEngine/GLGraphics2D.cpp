#include "GLGraphics2D.h"

#include "Math/Base.h"

#include "RHI/RHICommand.h"
#include "RHI/RHICommon.h"
#include "RHI/RHIGlobalResource.h"
#include "RHI/DrawUtility.h"



#include <cassert>
#include <algorithm>


#include "Math/Vector3.h"
#include "Math/Quaternion.h"
#include "Math/Matrix4.h"

using Math::Vector3;
using Math::Quaternion;
using Math::Matrix4;

static char const* vertexShader = CODE_STRING(
	varying vec2 vTexCoord;
	void main(void)
	{
		vTexCoord = gl_MultiTexCoord0.xy;
		gl_Position = ftransform();
	}
);

static char const* fragmentShader = CODE_STRING(
	sampler2D myTexture;
	varying vec2 vTexCoord;
	uniform vec3 colorKey;
	void main(void)
	{
		vec4 color = texture2D(myTexture, vTexCoord);
		if( color.rgb == colorKey )
			discard;
		gl_FragColor = color;
	}
);
#if 1
#define DRAW_LINE_IMPL( EXPR )\
	mBuffer.clear();\
	EXPR;\
	drawLineBuffer();

#define DRAW_IMPL( EXPR )\
	mBuffer.clear();\
	EXPR;\
	drawPolygonBuffer();
#else


#endif


class BatchedDrawer
{
public:
	void flushDrawCommond( Render::RHICommandList& commandList )
	{


	}


	void addPolygon( Vector2 v[] , int numV , Color4ub const& color )
	{
		int baseIndex = mBaseVertices.size();
		mBaseVertices.resize(baseIndex + numV);
		for( int i = 0; i < numV; ++i )
		{
			BaseVertex& vtx = mBaseVertices[baseIndex + i];
			vtx.pos = v[i];
			vtx.color = color;
		}

		for( int i = 0; i < numV ; ++i )
		{

		}
	}
	void addPolygonLine(Vector2 v[], int numV, Color4ub const& color)
	{



	}


	struct BaseVertex
	{
		Vector2    pos;
		Color4ub color;
	};

	std::vector< BaseVertex > mBaseVertices;
	std::vector< int32 >      mBaseIndices;

	struct TexVertex : BaseVertex
	{
		float uv[2];
	};
};

static inline int calcCircleSemgmentNum( int r )
{
	return std::max( 4 * ( r / 2 ) , 32 ) * 10;
}


GLGraphics2D::GLGraphics2D()
	:mWidth(1)
	,mHeight(1)
{
	mFont = nullptr;
	mWidthPen = 1;
	mDrawBrush = true;
	mDrawPen   = true;
	mAlpha     = 1.0;
	mColorKeyShader = 0;
	mColorBrush = Color3f( 0 , 0 , 0 );
	mColorPen = Color3f( 0 , 0 , 0 );
	mColorFont = Color3f(0, 0, 0);
}

void GLGraphics2D::init( int w , int h )
{
	mWidth = w;
	mHeight = h;

	char const* str = vertexShader;
	int i = 1;
}

void GLGraphics2D::beginXForm()
{

}

void GLGraphics2D::finishXForm()
{

}

void GLGraphics2D::pushXForm()
{
	mXFormStack.push();
}

void GLGraphics2D::popXForm()
{
	mXFormStack.pop();
}

void GLGraphics2D::identityXForm()
{
	mXFormStack.set(RenderTransform2D::Identity());
}

void GLGraphics2D::translateXForm(float ox, float oy)
{
	mXFormStack.translate(Vector2(ox, oy));
}

void GLGraphics2D::rotateXForm(float angle)
{
	mXFormStack.rotate(Math::Deg2Rad(angle));
}

void GLGraphics2D::scaleXForm(float sx, float sy)
{
	mXFormStack.scale(Vector2(sx, sy));
}

void GLGraphics2D::beginRender()
{
	//glPushAttrib(GL_ENABLE_BIT | GL_VIEWPORT_BIT);

	using namespace Render;
	RHICommandList& commandList = GetCommandList();
	
	RHISetViewport(commandList, 0, 0, mWidth, mHeight);

	RHISetShaderProgram(commandList, nullptr);
	RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());
	RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
	RHISetRasterizerState(commandList, TStaticRasterizerState< ECullMode::None >::GetRHI());

	RHISetupFixedPipelineState(commandList, AdjProjectionMatrixForRHI(OrthoMatrix(0 , mWidth, mHeight, 0 , -1, 1)));
	RHISetInputStream(commandList, &TStaticRenderRTInputLayout<RTVF_XY>::GetRHI() , nullptr , 0 );

	mXFormStack.clear();
	//glDisable(GL_TEXTURE_2D);
}

void GLGraphics2D::endRender()
{
	using namespace Render;

	RHICommandList& commandList = GetCommandList();
	RHIFlushCommand(commandList);
	//glPopAttrib();
}

void GLGraphics2D::emitPolygonVertex(Vector2 pos[] , int num)
{
	for( int i = 0 ; i < num ; ++i )
	{
		emitVertex( pos[i].x , pos[i].y );
	}
}

void GLGraphics2D::emitPolygonVertex(Vec2i pos[] , int num)
{
	for( int i = 0 ; i < num ; ++i )
	{
		emitVertex( pos[i].x , pos[i].y );
	}
}

void GLGraphics2D::emintRectVertex(Vector2 const& p1 , Vector2 const& p2)
{
	emitVertex( p1.x , p1.y );
	emitVertex( p2.x , p1.y );
	emitVertex( p2.x , p2.y );
	emitVertex( p1.x , p2.y );
}

void GLGraphics2D::emitCircleVertex(float cx, float cy, float r, int numSegment)
{
	float theta = 2 * Math::PI / float(numSegment); 
	float c, s;
	Math::SinCos(theta, s, c);

	float x = r;//we start at angle = 0 
	float y = 0;
	float v[2];
	for(int i = 0; i < numSegment; ++i) 
	{ 
		v[0] = cx + x;
		v[1] = cy + y;
		emitVertex( v );

		//apply the rotation matrix
		float temp = x;
		x = c * temp - s * y;
		y = s * temp + c * y;
	}
}

void GLGraphics2D::emitEllipseVertex(float cx, float cy, float r , float yFactor , int numSegment)
{
	float theta = 2 * Math::PI / float(numSegment);
	float c, s;
	Math::SinCos(theta, s, c);

	float x = r;//we start at angle = 0 
	float y = 0;
	float v[2];
	for(int i = 0; i < numSegment; ++i) 
	{ 
		v[0] = cx + x;
		v[1] = cy + yFactor * y;
		emitVertex( v );
		float temp = x;
		x = c * temp - s * y;
		y = s * temp + c * y;
	}
}

void GLGraphics2D::emitRoundRectVertex( Vector2 const& pos , Vector2 const& rectSize , Vector2 const& circleSize)
{
	int numSegment = calcCircleSemgmentNum( ( circleSize.x + circleSize.y ) / 2 );
	float yFactor = float( circleSize.y ) / circleSize.x;
	int num = numSegment / 4;

	double theta = 2 * Math::PI / float(numSegment);
	double c = cos(theta);//precalculate the sine and cosine
	double s = sin(theta);

	float cvn[4][2] =
	{
		float( pos.x + rectSize.x - circleSize.x ) , float( pos.y + rectSize.y - circleSize.y ) ,
		float( pos.x + circleSize.x )              , float( pos.y + rectSize.y - circleSize.y ) ,
		float( pos.x + circleSize.x )              , float( pos.y + circleSize.y ) ,
		float( pos.x + rectSize.x - circleSize.x ) , float( pos.y + circleSize.y ) ,
	};

	float v[2];
	double x , y;
	float cx = cvn[0][0];
	float cy = cvn[0][1];
	switch (0)
	{
	case 0: x = circleSize.x; y = 0; break;
	case 1: x = 0; y = circleSize.x; break;
	case 2: x = -circleSize.x; y = 0; break;
	case 3: x = 0; y = -circleSize.x; break;
	}

	for( int side = 0 ; side < 4 ; ++side )
	{
		v[0] = cx + x;
		v[1] = cy + yFactor * y;
		emitVertex(v);

		for(int i = 0; i < num ; ++i) 
		{ 
			if ( i == num / 2 )
			{
				float tc, ts;
				Math::SinCos(theta * i , ts, tc);
				float x0, y0;
				switch (side)
				{
				case 0: x0 = circleSize.x; y0 = 0; break;
				case 1: x0 = 0; y0 = circleSize.x; break;
				case 2: x0 = -circleSize.x; y0 = 0; break;
				case 3: x0 = 0; y0 = -circleSize.x; break;
				}

				x = tc * x0 - ts * y0;
				y = ts * x0 + tc * y0;
			}
			else
			{
				double x0 = x;
				x = c * x0 - s * y;
				y = s * x0 + c * y;
			}

			v[0] = cx + x;
			v[1] = cy + yFactor * y;
			emitVertex(v);
		}

		int next = ( side == 3 ) ? 0 : side + 1;
		switch (next)
		{
		case 0: x = circleSize.x; y = 0; break;
		case 1: x = 0; y = circleSize.x; break;
		case 2: x = -circleSize.x; y = 0; break;
		case 3: x = 0; y = -circleSize.x; break;
		}
		cx = cvn[next][0];
		cy = cvn[next][1];
		v[0] = cx + x;
		v[1] = cy + yFactor * y;
		emitVertex( v );
	}
}


void GLGraphics2D::emitLineVertex(Vector2 const &p1, Vector2 const &p2)
{
	emitVertex( p1.x , p1.y );
	emitVertex( p2.x , p2.y );
}

void GLGraphics2D::emitVertex(float x , float y)
{
	Vector2 v = mXFormStack.get().tranformPosition(Vector2(x, y));
	mBuffer.push_back(v.x);
	mBuffer.push_back(v.y);
}

void GLGraphics2D::emitVertex( float v[] )
{
	Vector2 temp = mXFormStack.get().tranformPosition(Vector2(v[0],v[1]));
	mBuffer.push_back(temp.x);
	mBuffer.push_back(temp.y);
}

void GLGraphics2D::drawPixel(Vector2 const& p , Color3ub const& color)
{
	using namespace Render;
	struct Vertex_XY_CA
	{
		Math::Vector2 pos;
		Color4f c;
	} v = { p , Color4f( color , mAlpha ) };
	TRenderRT<RTVF_XY_CA>::Draw( GetCommandList() , EPrimitive::Points, &v, 1);
}

void GLGraphics2D::drawRect(int left , int top , int right , int bottom)
{
	Vector2 p1( left , top );
	Vector2 p2( right , bottom );
	DRAW_IMPL( emintRectVertex( p1 , p2 ) );
}

void GLGraphics2D::drawRect(Vector2 const& pos , Vector2 const& size)
{
	Vector2 p2 = pos + size;
	DRAW_IMPL( emintRectVertex( pos , p2 ) );
}

void GLGraphics2D::drawCircle(Vector2 const& center , float r)
{
	int numSeg = std::max( 2 * r * r , 4.0f );
	DRAW_IMPL( emitCircleVertex( center.x , center.y , r , calcCircleSemgmentNum( r ) ) );
}

void GLGraphics2D::drawEllipse(Vector2 const& center , Vector2 const& size)
{
	float yFactor = float( size.y ) / size.x;
	DRAW_IMPL( emitEllipseVertex( center.x , center.y , size.x , yFactor , calcCircleSemgmentNum( ( size.x + size.y ) / 2 ) ) );
}

void GLGraphics2D::drawPolygon(Vector2 pos[] , int num)
{
	DRAW_IMPL( emitPolygonVertex( pos , num ) );
}

void GLGraphics2D::drawPolygon(Vec2i pos[] , int num)
{
	DRAW_IMPL( emitPolygonVertex( pos , num ) );
}

void GLGraphics2D::drawLine(Vector2 const& p1 , Vector2 const& p2)
{
	DRAW_LINE_IMPL( emitLineVertex(p1, p2) );
}

void GLGraphics2D::drawRoundRect(Vector2 const& pos , Vector2 const& rectSize , Vector2 const& circleSize)
{
	DRAW_IMPL( emitRoundRectVertex( pos , rectSize , circleSize / 2 ) );
}

void GLGraphics2D::fillGradientRect(Vector2 const& posLT, Color3ub const& colorLT, Vector2 const& posRB, Color3ub const& colorRB, bool bHGrad)
{

}

void GLGraphics2D::setTextColor(Color3ub const& color)
{
	mColorFont = color;
}

void GLGraphics2D::drawText(Vector2 const& pos , char const* str)
{
	if ( !mFont || !mFont->isValid() )
		return;
	int fontSize = mFont->getSize();
	float ox = pos.x;
	float oy = pos.y;
	drawTextImpl(ox, oy, str);
}

void GLGraphics2D::drawText(Vector2 const& pos , Vector2 const& size , char const* str , bool beClip /*= false */)
{
#if	IGNORE_NSIGHT_UNSUPPORT_CODE
	return;
#endif

	if ( !mFont || !mFont->isValid() )
		return;
	if( beClip )
		beginClip(pos, size);

	Vector2 extent = mFont->calcTextExtent(str);
	int fontSize = mFont->getSize();
	int strLen = strlen( str );
	Vector2 renderPos = pos + (size - extent) / 2;
	drawTextImpl(renderPos.x, renderPos.y, str);

	if( beClip )
		endClip();
}

void GLGraphics2D::drawTextImpl(float  ox, float  oy, char const* str)
{
#if	IGNORE_NSIGHT_UNSUPPORT_CODE
	return;
#endif
	assert( mFont );
	glColor4f( mColorFont.r , mColorFont.g , mColorFont.b , 1.0 );
	mFont->draw(GetCommandList(), Vector2(int(ox),int(oy)) , str );
}


void GLGraphics2D::drawPolygonBuffer()
{
	using namespace Render;
#if	IGNORE_NSIGHT_UNSUPPORT_CODE
	return;
#endif

	assert(!mBuffer.empty());

	if( mDrawBrush )
	{
		TRenderRT<RTVF_XY>::Draw(GetCommandList(), EPrimitive::Polygon, mBuffer.data(), mBuffer.size() / 2, LinearColor(mColorBrush, mAlpha));
	}
	if( mDrawPen )
	{
		TRenderRT<RTVF_XY>::Draw(GetCommandList(), EPrimitive::LineLoop, mBuffer.data(), mBuffer.size() / 2, LinearColor(mColorPen, mAlpha));
	}

}

void GLGraphics2D::drawLineBuffer()
{
	using namespace Render;

#if	IGNORE_NSIGHT_UNSUPPORT_CODE
	return;
#endif
	assert(!mBuffer.empty());

	if (mDrawPen)
	{
		TRenderRT<RTVF_XY>::Draw(GetCommandList(), EPrimitive::LineLoop, mBuffer.data(), mBuffer.size() / 2, LinearColor(mColorPen, mAlpha));
	}
}


Render::RHICommandList& GLGraphics2D::GetCommandList()
{
	using namespace Render;
	return RHICommandList::GetImmediateList();
}

void GLGraphics2D::drawTexture(GLTexture2D& texture, Vector2 const& pos, Color3ub const& color /*= Color3ub(255, 255, 255)*/)
{
	drawTexture(texture, pos, Vector2(texture.getSizeX(), texture.getSizeY()), color);
}

void GLGraphics2D::drawTexture(GLTexture2D& texture, Vector2 const& pos, Vector2 const& size, Color3ub const& color /*= Color3ub(255,255,255) */)
{
	glEnable(GL_TEXTURE_2D);
	{
		GL_BIND_LOCK_OBJECT(texture);
		glColor4fv(Color4f(Color3f(color), mAlpha));
		Render::DrawUtility::Sprite( Render::RHICommandList::GetImmediateList(), pos, size, Vector2(0, 0), Vector2(0, 0), Vector2(1, 1));
	}
	glDisable(GL_TEXTURE_2D);
}

void GLGraphics2D::drawTexture(GLTexture2D& texture, Vector2 const& pos, Vector2 const& texPos, Vector2 const& texSize, Color3ub const& color)
{
	drawTexture(texture, pos, Vector2(texture.getSizeX(), texture.getSizeY()), texPos , texSize , color);
}

void GLGraphics2D::drawTexture(GLTexture2D& texture, Vector2 const& pos, Vector2 const& size, Vector2 const& texPos, Vector2 const& texSize, Color3ub const& color)
{
	glEnable(GL_TEXTURE_2D);
	{
		GL_BIND_LOCK_OBJECT(texture);
		glColor4fv( Color4f( Color3f(color), mAlpha) );
		Vector2 textureSize = Vector2(texture.getSizeX(), texture.getSizeY());
		Render::DrawUtility::Sprite(Render::RHICommandList::GetImmediateList(), pos, size, Vector2(0, 0), Vector2( texPos.div(textureSize) ), Vector2( texSize.div(textureSize) ));
	}
	glDisable(GL_TEXTURE_2D);
}

void GLGraphics2D::beginClip(Vec2i const& pos, Vec2i const& size)
{
	using namespace Render;
	RHISetRasterizerState(GetCommandList(), TStaticRasterizerState< ECullMode::None , EFillMode::Solid ,EFrontFace::Default, true >::GetRHI());
	RHISetScissorRect(GetCommandList(), pos.x, mHeight - pos.y - size.y, size.x, size.y);
}

void GLGraphics2D::endClip()
{
	using namespace Render;
	RHISetRasterizerState(GetCommandList(), TStaticRasterizerState< ECullMode::None, EFillMode::Solid, EFrontFace::Default, false >::GetRHI());
}

void GLGraphics2D::beginBlend(Vector2 const& pos , Vector2 const& size , float alpha)
{
	using namespace Render;
	RHISetBlendState(GetCommandList(), TStaticBlendState< CWM_RGBA , Blend::eSrcAlpha , Blend::eOneMinusSrcAlpha >::GetRHI());
	mAlpha = alpha;
}

void GLGraphics2D::endBlend()
{
	using namespace Render;
	RHISetBlendState(GetCommandList(), TStaticBlendState<>::GetRHI());
	mAlpha = 1.0f;
}

