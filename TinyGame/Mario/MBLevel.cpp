#include "TinyGamePCH.h"
#include "MBLevel.h"

namespace Mario
{
	Block* gBlockMap[ 256 ] = { NULL };

	float const gTileMargin = 0.001f;
	//float const gTileMargin = 0;

	void Block::initMap()
	{
		static Block block;
		static BlockSlope slope;
		gBlockMap[ BLOCK_NULL   ] = NULL;
		gBlockMap[ BLOCK_STATIC ] = &block;
		gBlockMap[ BLOCK_BRICK  ] = &block;
		gBlockMap[ BLOCK_SLOPE_21 ] = &slope;
		gBlockMap[ BLOCK_SLOPE_11 ] = &slope;
	}

	Block* Block::get( int type )
	{
		return gBlockMap[ type ];
	}

	bool Block::checkCollision( Tile const& tile , Vec2f const& pos , Vec2f const& size )
	{
		Vec2f dif = pos - TileLength * tile.pos;
		return true;
	}

	float Block::calcFixPosX( Tile const& tile , Vec2f const& pos , Vec2f const& size , float offset )
	{
		if ( offset > 0 )
			return TileLength * tile.pos.x - size.x;
		return TileLength * ( tile.pos.x + 1 );
	}

	float Block::calcFixPosY( Tile const& tile , Vec2f const& pos , Vec2f const& size , float offset )
	{
		if ( offset > 0 )
			return TileLength * tile.pos.y - size.y;
		return TileLength * ( tile.pos.y + 1 );
	}

	void Player::tick()
	{
		float const GodMoveSpeed = 120.0f;
		float const StopAcc = 1.0f;
		float const WalkMaxSpeed = 60.0f;
		float const WalkAcc = 1.5f;
		float const GravityAcc = 5.0f;
		float const JumpSpeed = 60.0f;
		float const JumpMoveAcc = 1.5f;
		float const FallMaxSpeed = 250.0f;
		int const JumpMaxCount = 30;

		switch( moveType )
		{
		case eRUN:
			if ( button & ACB_RIGHT )
			{
				vel.x += WalkAcc;
				if ( vel.x > WalkMaxSpeed )
					vel.x = WalkMaxSpeed;
			}
			else if ( button & ACB_LEFT )
			{
				vel.x -= WalkAcc;
				if ( vel.x < -WalkMaxSpeed )
					vel.x = -WalkMaxSpeed;
			}
			else if ( moveType == eRUN )
			{
				if ( vel.x > StopAcc )
					vel.x -= StopAcc;
				else if ( vel.x < -StopAcc )
					vel.x += StopAcc;
				else
					vel.x = 0;
			}

			if ( button & ACB_JUMP )
			{
				jumpLoop = 0;
				moveType = eJUMP;
				vel.y = JumpSpeed;
			}
			else
			{
				vel.y = -GravityAcc;
			}
			break;
		case eFALLING:
		case eJUMP:
			if ( button & ACB_RIGHT )
			{
				vel.x += WalkAcc;
				if ( vel.x > WalkMaxSpeed )
					vel.x = WalkMaxSpeed;
			}
			else if ( button & ACB_LEFT )
			{
				vel.x -= WalkAcc;
				if ( vel.x < -WalkMaxSpeed )
					vel.x = -WalkMaxSpeed;
			}

			if ( moveType == eJUMP )
			{
				if ( button & ACB_JUMP )
				{
					++jumpLoop;
					if ( jumpLoop > JumpMaxCount )
					{
						moveType = eFALLING;
					}
				}
				else
				{
					moveType = eFALLING;
				}
			}
			else if ( moveType == eFALLING )
			{
				vel.y -= GravityAcc;
				if ( vel.y < -FallMaxSpeed )
					vel.y = -FallMaxSpeed;
			}
			break;
		case eGOD_MODE:
			if ( button & ACB_RIGHT )
			{
				vel.x = GodMoveSpeed;
			}
			else if ( button & ACB_LEFT )
			{
				vel.x = -GodMoveSpeed;
			}
			else
			{
				vel.x = 0;
			}

			if ( button & ACB_UP )
			{
				vel.y = GodMoveSpeed;
			}
			else if ( button & ACB_DOWN )
			{
				vel.y = -GodMoveSpeed;
			}
			else
			{
				vel.y = 0;
			}
			break;
		}


		if ( vel.x != 0 )
		{
			float offset = vel.x * TICK_TIME;
			pos.x += offset;
			if ( Tile* tile = getWorld()->testTileCollision( pos , size ) )
			{
				Block* block = Block::get( tile->block );
				
				bool haveBlock = true;
				if ( moveType == eRUN && 0 )
				{
					switch( tile->block )
					{
					case BLOCK_SLOPE_11:
						{
							int dir = BlockSlope::getDir( tile->meta );
							if ( dir == 0 && offset < 0 )
							{
								float factor = 0.5;
								pos.x -= ( 1.0f - factor)  * offset;
								pos.y += -factor * offset;

								if ( !getWorld()->testTileCollision( pos , size ) )
								{
									haveBlock = false;
								}
	
							}
							else if ( dir == 1 && offset > 0 )
							{
								float factor = 0.5;
								pos.x -= ( 1.0f - factor) * offset;
								pos.y += factor * offset;

								if ( !getWorld()->testTileCollision( pos , size ) )
								{
									haveBlock = false;
								}
							}
						}
						
						break;
					}

				}
				if  ( moveType == eFALLING && vel.y < 5 * GravityAcc )
				{
					float yTile = ( tile->pos.y + 1 ) * TileLength;
					if ( pos.y + 1 > yTile )
					{
						Vec2f testPos;
						testPos.x = pos.x;
						testPos.y = yTile;

						if ( getWorld()->testTileCollision( testPos , size ) == NULL )
						{
							pos = testPos;
							haveBlock = false;
						}
					}
				}

				if ( haveBlock )
				{
					float fixPos = block->calcFixPosX( *tile , pos , size , offset );
					pos.x = fixPos;
					vel.x = 0;
				}
			}
		}

		if ( vel.y != 0 )
		{
			float offset = vel.y * TICK_TIME;
			pos.y += offset;
			if ( Tile* tile = getWorld()->testTileCollision( pos , size ) )
			{
				Block* block = Block::get( tile->block );
				pos.y = block->calcFixPosY( *tile , pos , size , offset );

				if ( vel.y > 0 )
				{
					if ( moveType == eJUMP )
					{
						moveType = eFALLING;
						vel.y = 0;
					}
				}
				else 
				{
					vel.y = 0;
					if ( moveType != eGOD_MODE )
						moveType = eRUN;
				}
			}
			else if ( moveType == eRUN )
			{
				moveType = eFALLING;
			}
		}

		button = 0;
	}

	void Player::reset()
	{
		moveType = eRUN;
		vel  = Vec2f(0,0);
		size = Vec2f( 14 , 27 );
		onGround = false;
	}


	bool BlockSlope::checkCollision( Tile const& tile , Vec2f const& pos , Vec2f const& size )
	{
		Vec2f dif = pos - TileLength * tile.pos;

		switch ( tile.block )
		{
		case BLOCK_SLOPE_21:
			break;
		case BLOCK_SLOPE_11:
			switch ( getDir( tile.meta ) )
			{
			case 0:
				if ( dif.x + dif.y > TileLength )
					return false;
				break;
			case 1:
				if ( dif.x + size.x < dif.y )
					return false;
				break;
			case 2:
				break;
			case 3:
				break;
			}
			break;
		}
		return true;
	}

	float BlockSlope::calcFixPosX( Tile const& tile , Vec2f const& pos , Vec2f const& size , float offset )
	{
		Vec2f tilePos = TileLength * tile.pos;
		Vec2f dif = pos - tilePos;

		switch ( tile.block )
		{
		case BLOCK_SLOPE_21:
			break;
		case BLOCK_SLOPE_11:
			switch ( getDir( tile.meta ) )
			{
			case 0:
				if ( -( dif.x - offset ) + gTileMargin > size.x )
					return tilePos.x - size.x;
				else if ( dif.y < 0 )
					return tilePos.x + TileLength;
				return tilePos.x + ( TileLength - dif.y );
			case 1: 
				if ( dif.x - offset + gTileMargin > TileLength )
					return tilePos.x + TileLength;
				else if ( dif.y < 0 )
					return tilePos.x - size.x;
				return tilePos.x + dif.y - size.x;
			case 2:
				break;
			case 3:
				break;
			}
			break;
		}
		return pos.x;
	}

	float BlockSlope::calcFixPosY( Tile const& tile , Vec2f const& pos , Vec2f const& size , float offset )
	{
		Vec2f tilePos = TileLength * tile.pos;
		Vec2f dif = pos - tilePos;


		switch ( tile.block )
		{
		case BLOCK_SLOPE_21:
			break;
		case BLOCK_SLOPE_11:
			switch ( getDir( tile.meta ) )
			{
			case 0:
				if ( -( dif.y - offset ) + gTileMargin > size.y )
					return tilePos.y - size.y;
				else if ( dif.x < 0 )
					return tilePos.y + TileLength;
				return tile.pos.y * TileLength + ( TileLength - dif.x );
			case 1:
				if ( -( dif.y - offset ) + gTileMargin > size.y )
					return tilePos.y - size.y;
				else if ( dif.x + size.x > TileLength )
					return tilePos.y + TileLength;
				return tile.pos.y * TileLength + ( size.x + dif.x );
			case 2:
				break;
			case 3:
				break;
			}
			break;
		}

		return pos.y;
	}


	

	Tile* World::testTileCollision( Vec2f const& pos , Vec2f const& size )
	{
		Vec2i start = Vec2i( int( ( pos.x + gTileMargin )/ TileLength ) , int( ( pos.y + gTileMargin ) / TileLength ) );
		Vec2i end   = Vec2i( int( ( pos.x + size.x - gTileMargin ) / TileLength ) , int( ( pos.y + size.y - gTileMargin ) / TileLength ) );

		for ( int i = start.x ; i <= end.x ; ++i )
		{
			if ( i < 0 || i >= mTileMap.getSizeX() )
				continue;
			for ( int j = start.y ; j <= end.y ; ++j )
			{
				if ( j < 0 || j >= mTileMap.getSizeY() )
					continue;

				Tile& tile = mTileMap.getData( i , j );
				if ( tile.block == BLOCK_NULL )
					continue;

				Block* block = Block::get( tile.block );

				if ( !block->checkCollision( tile , pos , size ) )
					continue;
				//#TODO
				return &tile;
			}
		}
		return NULL;
	}


	bool BlockCloud::checkCollision( Tile const& tile , Vec2f const& pos , Vec2f const& size )
	{
		Vec2f dif = pos - tile.pos * TileLength;
		switch( tile.meta )
		{
		case DIR_TOP:
			if ( dif.x < TileLength && TileLength < dif.x + size.x )
				return true;
			break;
		case DIR_DOWN:
			if ( dif.x < 0 && 0 < dif.x + size.x )
				return true;
			break;
		case DIR_LEFT:
			if ( dif.y < 0 && 0 < dif.y + size.y )
				return true;
			break;
		case DIR_RIGHT:
			if ( dif.y < TileLength && TileLength < dif.y + size.y )
				return true;
			break;
		}
		return false;
	}

}//namespace Mario