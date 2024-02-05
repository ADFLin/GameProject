#ifndef BJLevel_h__
#define BJLevel_h__

#include "DataStructure/Array.h"

namespace Bejeweled
{
	namespace EAction 
	{
		enum Type
		{
			None = 0,
			Destroy,
			BecomePower,
			BecomeHypercube ,
		};
	};
	struct ActionContext
	{
		static int const BoardSize = 8;
		static int const BoardStorageSize = BoardSize * BoardSize;

		int swapIndices[2];

		int removeIndexMax[BoardSize];
		int actionMap[BoardSize * BoardSize];
		int actionCount;
		void init(int idx1, int idx2)
		{
			swapIndices[0] = idx1;
			swapIndices[1] = idx2;
		}
		void clearMark()
		{
			std::fill_n(actionMap, BoardSize * BoardSize, EAction::None);
			actionCount = 0;
		}
	};

	struct GemData
	{

		enum EType : uint8
		{
			eNone,
			eNormal,
			ePower,
			eHypercube,
			eRock,
		};

		bool isGem() const
		{
			return type == eNormal || type == ePower;
		}
		bool isSameType(GemData const& other) const
		{
			if (isGem())
			{
				if (!other.isGem())
					return false;
				if (meta != other.meta)
					return false;
			}
			else
			{
				if (type != other.type)
					return false;
			}
			return true;
		}

		GemData(GemData const&) = default;
		GemData& operator = (GemData const& other) = default;
		bool operator == (GemData const& other) const 
		{
			return value == other.value; 
		}
		bool operator != (GemData const& other) const
		{
			return !this->operator == (other);
		}

		static GemData None() { return GemData(0); }
		static GemData Normal(int id) { return GemData(eNormal, id); }
		static GemData Hypercube()
		{
			return GemData(eHypercube, 0);
		}

		union
		{
			uint16 value;
			struct
			{
				EType type;
				uint8 meta;
			};
		};

		GemData()
		{

		}

		explicit GemData(uint16 value)
			:value(value)
		{

		}
		GemData(EType type, uint8 meta)
			:type(type),meta(meta)
		{

		}
	};

	class Level
	{
	public:

		static int const BoardSize = 8;
		static int const BoardStorageSize  = BoardSize * BoardSize;
		static int const MaxGemTypeNum = 7;
		static int const MinRmoveCount = 3;

		enum Dir
		{
			eRIGHT ,
			eDOWN  ,
			eLEFT  ,
			eTOP   ,
		};
		static bool isVerticalDir( Dir dir ){ return ( dir & 0x1 ) != 0;   }
		static int  getAdjoinedIndex( int idx , Dir dir )
		{
			int const dirOffset[] = { 1 , BoardSize , -1 , -BoardSize };
			return idx + dirOffset[ dir ];
		}
		static bool isNeighboring( int idx1 , int idx2 )
		{
			int offset = Math::Abs( idx2 - idx1 );
			if (offset == BoardSize)
				return true;
			if ( offset == 1 && ( idx1 / BoardSize ) == ( idx2 / BoardSize ) )
				return true;
			return false;
		}
		static int ToIndex( int x , int y ){ return y * BoardSize + x;  }
		static void ToCell(int index, int& outX, int& outY)
		{
			outX = index % BoardSize;
			outY = index / BoardSize;
		}
		static bool IsVaildRange(int x, int y)
		{
			return 0 <= x && x < BoardSize &&
				   0 <= y && y < BoardSize;
		}
		class Listener
		{
		public:
			virtual void prevCheckGems(){}
			virtual void postCheckGems(){}
			virtual void onDestroyGem( int idx , GemData type ){}
			virtual void onFillGem( int idx , GemData type ){}
			virtual void onMoveDownGem( int idxFrom , int idxTo ){}
		};

		Level( Listener* listener );
		Level(Level& level, Listener* listener)
			: mNumGemType(level.mNumGemType)
			, mListener(listener)
		{
			std::copy(level.mBoard, level.mBoard + BoardSize * BoardSize, mBoard);
		}

		GemData  getBoardGem( int idx ) const { return mBoard[idx]; }
		int      getGemTypeNum() const { return mNumGemType;  }
		void     setGemTypeNum( int num ){  mNumGemType = num;  }
		void     setBoardGem( int idx , GemData type ){ mBoard[idx] = type; }

		void     generateRandGems();
		bool     swapSequence(ActionContext& context, bool bRequestFill = true);
		void     stepSwap(ActionContext& context);
		size_t   stepCheckSwap(ActionContext& context);
		void     stepFill(ActionContext& context);
		size_t   stepCheckFill(ActionContext& context);


		struct RemoveDesc
		{
			enum EMethod
			{
				eNormal,
				eLine,
				eBomb,
				eTheSameId,
			};


			int  index;
			int  num;


			bool    bVertical;
			EMethod method;
		};

		void     markRemoveGem(ActionContext& context, int index);
		void     markRemove(ActionContext& context, RemoveDesc const& desc);

		void     stepAction(ActionContext& context);

		bool     testRemoveGeom( int idx1 );

		bool     testSwapPairHaveRemove( int idx1 , int idx2 );
		void     swapGeom(int idx1, int idx2);
	private:
		void     checkFillGem(ActionContext& context, int x , int maxIdx );
		int      checkGemVertical(ActionContext& context, int idx );
		int      checkGemHorizontal(ActionContext& context, int idx );
		void     fillGem( int x , int idxMax );
		GemData  produceGem( int idx );
		int      countGemsVertical( int idx , int& idxStart );
		int      countGemsHorizontal( int idx ,  int& idxStart );
		void     destroyGem(int index);

		Listener*    mListener;
		int          mNumGemType;
		GemData      mBoard[ BoardSize * BoardSize ];
	};

}//namespace Bejeweled

#endif // BJLevel_h__
