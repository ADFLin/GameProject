#include "RenderUtility.h"

#include "Texture.h"
#include "Base.h"

#include <cassert>

#include "RHI/RHICommand.h"
#include "RHI/DrawUtility.h"

using namespace Render;

void drawRect( Vec2f const& poz, Vec2f const& size )
{
	glBegin(GL_QUADS);
	glVertex2f(poz.x, poz.y);
	glVertex2f(poz.x+size.x, poz.y);
	glVertex2f(poz.x+size.x, poz.y+size.y);
	glVertex2f(poz.x, poz.y+size.y);
	glEnd();
}

void drawSprite(Vec2f const& pos, Vec2f const& size, Texture* tex)
{
	assert( tex );

	glEnable(GL_TEXTURE_2D);

	tex->bind();

	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0); glVertex2f(pos.x, pos.y);
	glTexCoord2f(1.0, 0.0); glVertex2f(pos.x+size.x, pos.y);
	glTexCoord2f(1.0, 1.0); glVertex2f(pos.x+size.x, pos.y+size.y);
	glTexCoord2f(0.0, 1.0); glVertex2f(pos.x, pos.y+size.y);
	glEnd();
	glBindTexture(GL_TEXTURE_2D,0);
	glDisable(GL_TEXTURE_2D);

}

void drawSprite(Vec2f const& pos, Vec2f const& size, float rot, Texture* tex )
{
	Vec2f hotspot = size / 2;
	glPushMatrix();
	glTranslatef( pos.x + hotspot.x , pos.y + hotspot.y , 0);
	glRotatef( Math::RadToDeg( rot ),0,0,1);
	glTranslatef( -hotspot.x , -hotspot.y , 0 );
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_EQUAL,1.0f);
	glEnable(GL_TEXTURE_2D);

	tex->bind();

	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0); glVertex2f(0.0, 0.0);
	glTexCoord2f(1.0, 0.0); glVertex2f(size.x, 0.0);
	glTexCoord2f(1.0, 1.0); glVertex2f(size.x, size.y);
	glTexCoord2f(0.0, 1.0); glVertex2f(0.0, size.y);
	glEnd();
	glBindTexture(GL_TEXTURE_2D,0);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_ALPHA_TEST);
	glPopMatrix();
}


void drawSprite(Vec2f const& pos, Vec2f const& size, float rot )
{
	Vec2f pivot = size / 2;

	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_EQUAL,1.0f);

	glPushMatrix();
	glTranslatef( pos.x + pivot.x , pos.y + pivot.y , 0);
	glRotatef( Math::RadToDeg( rot ),0,0,1);
	glTranslatef( -pivot.x , -pivot.y , 0 );

	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0); glVertex2f(0.0, 0.0);
	glTexCoord2f(1.0, 0.0); glVertex2f(size.x, 0.0);
	glTexCoord2f(1.0, 1.0); glVertex2f(size.x, size.y);
	glTexCoord2f(0.0, 1.0); glVertex2f(0.0, size.y);
	glEnd();

	glPopMatrix();

	glDisable(GL_ALPHA_TEST);
}

void drawRectLine( Vec2f const& pos , Vec2f const size )
{
	glBegin(GL_LINE_LOOP);
	glVertex2f(pos.x         , pos.y       );
	glVertex2f(pos.x + size.x, pos.y       );
	glVertex2f(pos.x + size.x, pos.y+size.y);
	glVertex2f(pos.x         , pos.y+size.y);
	glEnd();	
}

struct Vertex
{
	Vec2f pos;
	Vec2f uv;
};

void PrimitiveDrawer::drawRect(Vec2f const& pos, Vec2f const& size)
{
	Vertex vertices[4] =
	{
		{ mStack.get().transformPosition(pos), Vector2(0,0) },
		{ mStack.get().transformPosition(pos + Vec2f(size.x,0)), Vector2(1,0) },
		{ mStack.get().transformPosition(pos + size), Vector2(1,1) },
		{ mStack.get().transformPosition(pos + Vec2f(0, size.y)), Vector2(0,1) },
	};

	TRenderRT< RTVF_XY_T2 >::Draw(*mCommandList, EPrimitive::Quad, vertices, ARRAY_SIZE(vertices));
}

void PrimitiveDrawer::drawRect(Vec2f const& pos, Vec2f const& size, float rot, Vec2f const& pivot)
{
	Vec2f offset = size.mul(pivot);
	mStack.push();
	mStack.translate(pos + offset);
	mStack.rotate(rot);
	mStack.translate(-offset);
	drawRect(Vec2f::Zero(), size);
	mStack.pop();
}

void PrimitiveDrawer::drawRect(Vec2f const& pos, Vec2f const& size, Vec2f const& pivot)
{
	Vec2f offset = size.mul(pivot);
	drawRect(pos + offset, size);
}


void PrimitiveDrawer::drawRect(Vec2f const& pos, Vec2f const& size, Vec2f const& uvMin, Vec2f const& uvMax)
{
	Vertex vertices[4] =
	{
		{ mStack.get().transformPosition(pos), uvMin },
		{ mStack.get().transformPosition(pos + Vec2f(size.x,0)), Vec2f(uvMax.x,uvMin.y) },
		{ mStack.get().transformPosition(pos + size), uvMax },
		{ mStack.get().transformPosition(pos + Vec2f(0, size.y)), Vec2f(uvMin.x,uvMax.y) },
	};
	TRenderRT< RTVF_XY_T2 >::Draw(*mCommandList, EPrimitive::Quad, vertices, ARRAY_SIZE(vertices));
}

void PrimitiveDrawer::drawRectLine(Vec2f const& pos, Vec2f const size)
{
	Vertex vertices[4] =
	{
		{ mStack.get().transformPosition(pos), Vec2f(0,0) },
		{ mStack.get().transformPosition(pos + Vec2f(size.x,0)), Vec2f(1,0) },
	    { mStack.get().transformPosition(pos + size), Vector2(1,1) },
	    { mStack.get().transformPosition(pos + Vec2f(0, size.y)), Vec2f(0,1) },
	};
	TRenderRT< RTVF_XY_T2 >::Draw(*mCommandList, EPrimitive::LineLoop, vertices, ARRAY_SIZE(vertices));
}
