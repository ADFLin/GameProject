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
			DirType dir;
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
			Body& body = mBodies[mIdxTail];

			Body const& head = getHead();
			body.dir = moveDir;
			body.pos = head.pos + GetDirOffset(moveDir);

			mIdxHead = mIdxTail;
			++mIdxTail;
			if (mIdxTail >= mBodies.size())
				mIdxTail = 0;
		}

		void    moveStep(Vec2i const& pos, DirType moveDir)
		{
			Body& body = mBodies[mIdxTail];
			body.dir = moveDir;
			body.pos = pos;

			mIdxHead = mIdxTail;
			++mIdxTail;
			if (mIdxTail >= mBodies.size())
				mIdxTail = 0;
		}

		void    growBody(size_t num = 1)
		{
			Body newBody = mBodies[mIdxTail];
			mBodies.insert(mBodies.begin() + mIdxTail, num, newBody);

			//resolve case : [ Tail ] [][]....[][ Head ]
			if (mIdxTail <= mIdxHead)
				mIdxHead += num;
		}
		Body const& getHead() const { return mBodies[mIdxHead]; }
		Body const& getTail() const { return mBodies[mIdxTail]; }
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

		typedef TArray< Body > BodyList;
		class Iterator
		{
		public:
			Body const& getElement() { return mBodies[mIdxCur]; }
			int  getIndex() { return mCount; }
			bool haveMore() { return mCount < mBodies.size(); }
			void goNext()
			{
				++mCount;
				if (mIdxCur == 0)
					mIdxCur = (unsigned)mBodies.size();
				--mIdxCur;
			}
			bool isTail() const { return mCount == mBodies.size() - 1; }
		private:
			Iterator(BodyList const& bodies, unsigned idxHead)
				:mBodies(bodies), mIdxCur(idxHead), mCount(0)
			{
			}
			BodyList const& mBodies;
			size_t   mCount;
			unsigned mIdxCur;
			friend class Snake;
		};

		Iterator       createIterator() { return Iterator(mBodies, mIdxHead); }
		Body const& getElementByIndex(unsigned idx) { return mBodies[idx]; }


		void       addTial(DirType dir)
		{
			Body newBody = mBodies[mIdxTail];
			newBody.dir = dir;
			newBody.pos += GetDirOffset(InverseDir(dir));
			mBodies.insert(mBodies.begin() + mIdxTail, 1, newBody);

			//resolve case : [ Tail ] [][]....[][ Head ]
			if (mIdxTail <= mIdxHead)
				mIdxHead += 1;
		}

		void       reset(Vec2i const& pos, DirType dir, size_t length)
		{
			mIdxTail = 0;
			mIdxHead = length - 1;

			mBodies.clear();
			Body body;
			body.pos = pos;
			body.dir = dir;
			mBodies.resize(length, body);
		}
	private:
		BodyList  mBodies;
		unsigned  mIdxTail;
		unsigned  mIdxHead;
	};


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

			//Entity
			Rock,


			SnakeHead,
			SnakeBody,
		};

		FORCEINLINE bool IsEntity(uint8 id)
		{
			if (id == ETile::Rock)
				return true;

			return false;
		}

		template< typename T , typename ...TArgs >
		constexpr uint32 MakeBitFlags(T value , TArgs ...args)
		{
			return BIT(value) | MakeBitFlags(args...);
		}

		template< typename T >
		constexpr uint32 MakeBitFlags(T value)
		{
			return BIT(value);
		}
		FORCEINLINE static bool CanMoveTo(uint8 tileId)
		{
			constexpr uint32 BitFlags = MakeBitFlags(ETile::None, ETile::Goal, ETile::Apple, ETile::Trap, ETile::Portal);
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
			constexpr uint32 BitFlags = MakeBitFlags(ETile::None, ETile::SnakeBody, ETile::Trap, ETile::Portal);
			return !!(BitFlags & BIT(tileId));
		}

	}

	class WorldData
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
		};

		struct Portal
		{
			Vec2i   pos;
			DirType dir;
			uint8   color;
			int     link;
		};

		Snake mSnake;
		TGrid2D<Tile>    mMap;
		TArray< Entity > mEntities;
		TArray<Portal>   mPortals;

		int getEntityIndex(Vec2i const& pos)
		{
			return mEntities.findIndexPred([&](auto const& e) { return e.pos == pos; });
		}

		uint8 getTile(Vec2i const& pos, bool bCheckEntity = true);
		uint8 getThing(Vec2i const& pos);
		int   getSnakeBody(Vec2i const& pos);

		bool checkSnakeFall();
		int checkPortalMove(Vec2i pos, DirType dir, Vec2i& outPos, DirType& outDir);

	};

}//namespace AppleSnake

#endif // AppleSnakeLevel_H_DCE072DF_F131_4167_B598_E75AC8CB5130
