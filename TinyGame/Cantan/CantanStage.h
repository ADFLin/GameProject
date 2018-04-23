#include "StageBase.h"

#include "CantanLevel.h"

#include "RenderUtility.h"

namespace Cantan
{
	class LevelStage : public StageBase
	{
		typedef StageBase BaseClass;
	public:
		LevelStage(){}

		virtual bool onInit()
		{
			::Global::GUI().cleanupWidget();
			mCellManager.buildIsotropicMap( 4 );
			restart();
			return true;
		}

		virtual void onEnd()
		{

		}

		virtual void onUpdate( long time )
		{
			BaseClass::onUpdate( time );

			int frame = time / gDefaultTickTime;
			for( int i = 0 ; i < frame ; ++i )
				tick();

			updateFrame( frame );
		}

		void onRender( float dFrame )
		{
			Graphics2D& g = Global::getGraphics2D();

			bool drawCoord = false;
			g.setTextColor(Color3ub(0,0,255));
			FixString< 256 > str;
			RenderUtility::SetBrush( g , Color::eYellow );
			for( MapCellManager::CellVec::iterator iter = mCellManager.mCells.begin() , itEnd = mCellManager.mCells.end();
				 iter != itEnd ; ++iter )
			{
				MapCell* cell = *iter;
				Vector2 rPos = convertToScreenPos( cell->pos );
				g.drawCircle( rPos , 3 );
				if ( drawCoord )
					g.drawText( rPos , str.format("(%d %d)" , cell->pos.x , cell->pos.y) );

			}


			RenderUtility::SetPen( g , Color::eBlue );
			for( MapCellManager::CellEdgeVec::iterator iter = mCellManager.mCellEdges.begin() , itEnd = mCellManager.mCellEdges.end();
				iter != itEnd ; ++iter )
			{
				MapCell::Edge* edge = *iter;
				Vector2 rPosA = convertToScreenPos( edge->v[0]->pos );
				Vector2 rPosB = convertToScreenPos( edge->v[1]->pos );
				g.drawLine( rPosA , rPosB );
			}

			RenderUtility::SetBrush( g , Color::eRed );
			for( MapCellManager::CellVertexVec::iterator iter = mCellManager.mCellVertices.begin() , itEnd = mCellManager.mCellVertices.end();
				iter != itEnd ; ++iter )
			{
				MapCell::Vertex* v = *iter;
				Vector2 rPos = convertToScreenPos( v->pos );
				g.drawCircle( rPos , 3 );
				if ( drawCoord )
					g.drawText( rPos , str.format("(%d %d)" , v->pos.x , v->pos.y) );
			}

		}

		Vector2 convertToScreenPos( Vec2i const& cPos )
		{
#define SQRT_3 1.73205080756887729
			return Vector2( ::Global::getDrawEngine()->getScreenSize() / 2 ) + 40 * Vector2( 0.5 * SQRT_3 * cPos.x , cPos.y - 0.5 * cPos.x );
		}

		void restart()
		{

		}


		void tick()
		{

		}

		void updateFrame( int frame )
		{

		}

		bool onMouse( MouseMsg const& msg )
		{
			if ( !BaseClass::onMouse( msg ) )
				return false;
			return true;
		}

		bool onKey( unsigned key , bool isDown )
		{
			if ( !isDown )
				return false;

			switch( key )
			{
			case Keyboard::eR: restart(); break;
			}
			return false;
		}

	protected:

		MapCellManager mCellManager;

	};
}//namespace Cantan