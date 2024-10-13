#pragma once

#ifndef AppleSnakeLevel_H_DCE072DF_F131_4167_B598_E75AC8CB5130
#define AppleSnakeLevel_H_DCE072DF_F131_4167_B598_E75AC8CB5130

#include "Math/TVector2.h"
#include "DataStructure/Array.h"
#include "DataStructure/Grid2D.h"

namespace AppleSnake
{
	using Vec2i = TVector2<int>;

	typedef int DirType;
	inline Vec2i GetDirOffset(DirType dir)
	{
		Vec2i const offset[] = { Vec2i(1,0) , Vec2i(0,1), Vec2i(-1,0), Vec2i(0,-1) };
		return offset[dir];
	}

	inline int GetDirAxis(DirType dir)
	{
		return dir & 0x1;
	}

	inline DirType InverseDir(DirType dir)
	{
		return (dir + 2) % 4;
	}

	inline Vec2i GetBlockCorner(DirType dir1, DirType dir2)
	{
		CHECK(dir1 != dir2 && dir1 != InverseDir(dir2));
		Vec2i result;
		auto Func = [&result](DirType dir)
		{
			int axis = GetDirAxis(dir);
			result[axis] = 1 - (dir >> 1);
		};

		Func(dir1);
		Func(dir2);
		return result;
	}

	inline Vec2i const* GetBlockEdge(DirType dir)
	{
		static Vec2i const StaticEdges[] = { Vec2i(1,0) , Vec2i(1,1), Vec2i(0,1), Vec2i(0,0), Vec2i(1,0) };
		return &StaticEdges[dir];
	}

	DirType const GravityDir = 1;

	class Snake
	{
	public:
		struct Body
		{
			Vec2i   pos;
			DirType moveDir;
			DirType linkDir;
		};

		Snake() {}
		Snake(Vec2i const& pos, DirType dir, size_t length = 1)
		{
			mBodies.reserve(16);
			reset(pos, dir, length);
		}

		void init(Vec2i const& pos, DirType dir, size_t length)
		{
			mBodies.reserve(16);
			reset(pos, dir, length);
		}

		int  getLength() const { return (int)mBodies.size(); }

		void    moveStep(DirType moveDir)
		{
			Body& head = mBodies[mIdxHead];
			head.linkDir = moveDir;

			int indexTail = getTailIndex();
			Body& body = mBodies[indexTail];
			body.moveDir = moveDir;
			body.linkDir = moveDir;
			body.pos = head.pos + GetDirOffset(moveDir);

			mIdxHead = indexTail;
		}

		void    moveStep(Vec2i const& pos, DirType actionDir, DirType moveDir)
		{
			Body& head = mBodies[mIdxHead];
			head.linkDir = actionDir;

			int indexTail = getTailIndex();
			Body& body = mBodies[indexTail];
			body.moveDir = moveDir;
			body.linkDir = moveDir;
			body.pos = pos;

			mIdxHead = indexTail;
		}

		void    growBody(size_t num = 1)
		{
			int indexTail = getTailIndex();
			Body newBody = mBodies[indexTail];
			mBodies.insert(mBodies.begin() + indexTail, num, newBody);

			//resolve case : [ Tail ] [][]....[][ Head ]
			if (indexTail == 0)
				mIdxHead += num;
		}

		void       addTial(DirType dir)
		{
			int indexTail = getTailIndex();
			Body newBody = mBodies[indexTail];
			newBody.moveDir = dir;
			newBody.linkDir = dir;
			newBody.pos += GetDirOffset(InverseDir(dir));
			mBodies.insert(mBodies.begin() + indexTail, 1, newBody);

			//resolve case : [ Tail ] [][]....[][ Head ]
			if (indexTail == 0)
				mIdxHead += 1;
		}

		Body const& getHead() const { return mBodies[mIdxHead]; }
		Body const& getTail() const { return mBodies[getTailIndex()]; }
		int  getTailIndex() const
		{
			int index = mIdxHead + 1;
			if (index == mBodies.size())
				index = 0;
			return index;
		}

		Body& getBody(int index)
		{
			CHECK(0 <= index && index < mBodies.size());
			int idx = mIdxHead - index;
			if (idx < 0)
				idx += mBodies.size();

			return mBodies[idx];
		}
		Body const& getBody(int index) const
		{
			CHECK(0 <= index && index < mBodies.size());
			int idx = mIdxHead - index;
			if (idx < 0)
				idx += mBodies.size();

			return mBodies[idx];
		}

		void applyOffset(Vec2i const& offset)
		{
			for (auto& ele : mBodies)
			{
				ele.pos += offset;
			}
		}

		void splitFront(Snake& outSnake)
		{


		}

		void removeBack(int numRemoved)
		{
			int indexTail = getTailIndex();
			if (indexTail > mIdxHead)
			{
				int num = mBodies.size() - indexTail;
				if (numRemoved >= num)
				{
					int numCheck = mBodies.eraseToEnd(mBodies.begin() + indexTail);
					CHECK(numCheck == num);
					numRemoved -= num;
				}
				else
				{
					auto it = mBodies.begin() + indexTail;
					mBodies.erase(it, it + numRemoved);
					numRemoved = 0;
				}
			}

			if (numRemoved)
			{
				mBodies.erase(mBodies.begin(), mBodies.begin() + numRemoved);
				mIdxHead = mBodies.size() - 1;
			}
		}

		void splitBack(int indexStart, Snake& outSnake)
		{
			outSnake.mBodies.clear();
			for (int index = mBodies.size() - 1; index >= indexStart; --index)
			{
				outSnake.mBodies.push_back(getBody(index));
			}
			outSnake.mIdxHead = outSnake.mBodies.size() - 1;

			removeBack(outSnake.mBodies.size());
		}

		typedef TArray< Body > BodyList;
		class Iterator
		{
		public:
			Body const& getElement() { return mBodies[mIdxCur]; }
			int  getIndex() { return mCount; }
			bool haveMore() { return mCount < (int)mBodies.size(); }
			void goNext()
			{
				++mCount;
				if (mIdxCur == 0)
					mIdxCur = (int)mBodies.size();
				--mIdxCur;
			}
			bool isTail() const { return mCount == (int)mBodies.size() - 1; }
		private:
			Iterator(BodyList const& bodies, unsigned idxHead)
				:mBodies(bodies), mIdxCur(idxHead), mCount(0)
			{
			}
			BodyList const& mBodies;
			int mCount;
			int mIdxCur;
			friend class Snake;
		};

		Iterator    createIterator() { return Iterator(mBodies, mIdxHead); }
		Body const& getElementByIndex(unsigned idx) { return mBodies[idx]; }


	
		void       reset(Vec2i const& pos, DirType dir, size_t length)
		{
			mIdxHead = length - 1;

			mBodies.clear();
			Body body;
			body.pos = pos;
			body.moveDir = dir;
			body.linkDir = dir;
			mBodies.resize(length, body);
		}
	private:
		BodyList  mBodies;
		unsigned  mIdxHead;
	};

	template< typename T, typename ...TArgs >
	constexpr uint32 MakeBitFlags(T value, TArgs ...args)
	{
		return BIT(value) | MakeBitFlags(args...);
	}

	template< typename T >
	constexpr uint32 MakeBitFlags(T value)
	{
		return BIT(value);
	}

	namespace ETile
	{
		enum Type
		{
			None,
			Block,
			Apple,
			Goal,
			Trap,
			Portal,
			Cobweb,

			//Entity
			Rock,

			SnakeDead,
			SnakeHead,
			SnakeBody,
		};


		FORCEINLINE bool IsEntity(uint8 tileId)
		{
			constexpr uint32 BitFlags = MakeBitFlags(ETile::Rock, ETile::SnakeDead);
			return !!(BitFlags & BIT(tileId));
		}

		FORCEINLINE static bool CanEnityMoveTo(uint8 tileId)
		{
			constexpr uint32 BitFlags = MakeBitFlags(ETile::None, ETile::Trap, ETile::Portal, ETile::Cobweb);
			return !!(BitFlags & BIT(tileId));
		}

		FORCEINLINE static bool CanMoveTo(uint8 tileId)
		{
			constexpr uint32 BitFlags = MakeBitFlags(ETile::None, ETile::Goal, ETile::Apple, ETile::Trap, ETile::Portal, ETile::Cobweb);
			return !!(BitFlags & BIT(tileId));
		}
	
		FORCEINLINE bool CanPush(uint8 tileId)
		{
			if (!ETile::IsEntity(tileId))
				return false;

			return true;
		}

		FORCEINLINE bool CanFall(uint8 tileId)
		{
			if (!ETile::IsEntity(tileId))
				return false;

			return true;
		}

		FORCEINLINE bool CanFallIn(uint8 tileId)
		{
			constexpr uint32 BitFlags = MakeBitFlags(ETile::None, ETile::SnakeBody, ETile::Trap, ETile::Portal, ETile::Cobweb);
			return !!(BitFlags & BIT(tileId));
		}

		FORCEINLINE bool CanEat(uint8 tileId)
		{
			constexpr uint32 BitFlags = MakeBitFlags(ETile::Apple);
			return !!(BitFlags & BIT(tileId));
		}

	}

	namespace EApple
	{
		enum Type
		{
			Noraml,
			Fire,
			Gold,
			Double,
			Poison,
		};
	};

	class World
	{
	public:

		struct Tile
		{
			uint8 id;
			uint8 meta;

			Tile& operator = (uint8 inId) { id = inId; return *this; }
			operator uint8() { return id; }
		};

		struct Entity
		{
			uint8 id;
			Vec2i pos;
			uint8 meta;
			uint8 flags = 0;
		};

		struct Portal
		{
			Vec2i   pos;
			DirType dir;
			uint8   color;
			int     link;
		};

		struct DeadSnake : Snake
		{
			int entityIndex;
		};

		Snake mSnake;
		TGrid2D<Tile>      mMap;
		TArray<Entity>     mEntities;
		TArray<Portal>     mPortals;
		TArray<DeadSnake>  mDeadSnakes;

		void addDeadSnake(DeadSnake&& snake)
		{
			snake.entityIndex = mEntities.size();
			Entity e;
			e.id = ETile::SnakeDead;
			e.pos = snake.getHead().pos;
			e.meta = mDeadSnakes.size();
			for (int i = 0; i < snake.getLength(); ++i)
			{
				auto& body = snake.getBody(i);
				body.pos -= e.pos;
			}
			mEntities.push_back(e);
			mDeadSnakes.push_back(std::move(snake));
		}


		Portal& getPortalChecked(Vec2i const& pos)
		{
			auto const& tile = mMap(pos);
			CHECK(tile.id == ETile::Portal);
			return mPortals[tile.meta];
		}

		int getEntityIndex(Vec2i const& pos);

		bool checkMoveTo(Entity& entity, DirType dir, bool bSnakeMove);

		uint8 getTile(Vec2i const& pos, bool bCheckEntity = true);
		uint8 getThing(Vec2i const& pos, bool bCheckEntity = true);
		int   getSnakeBody(Vec2i const& pos);

		bool checkSnakeFall();
		int  checkPortalMove(Vec2i pos, DirType dir, Vec2i& outPos, DirType& outDir);

	};

}//namespace AppleSnake

#endif // AppleSnakeLevel_H_DCE072DF_F131_4167_B598_E75AC8CB5130
