#ifndef GoCore_h__
#define GoCore_h__

#include <memory>
#include <vector>

class SocketBuffer;

namespace Go
{

	namespace StoneColor
	{
		enum Enum
		{
			eEmpty = 0,
			eBlack = 1,
			eWhite = 2,
		};

		inline int Opposite( int color )
		{
			assert(color == eBlack || color == eWhite);
			return (color == eBlack) ? eWhite : eBlack;
		}
	}

	inline int ReadCoord( char const* coord , uint8 outPos[2] )
	{
		if( !('A' <= coord[0] && coord[0] <= 'Z') )
			return 0;

		uint8 x = coord[0] - 'A';
		if( coord[0] > 'I' )
		{
			--x;
		}

		if( !('0' <= coord[1] && coord[1] <= '9') )
			return 0;

		int result = 2;
		uint8 y = coord[1] - '0';
		if( ('0' <= coord[2] && coord[2] <= '9') )
		{
			y = 10 * y + (coord[2] - '0');
			++result;
		}

		--y;
		outPos[0] = x;
		outPos[1] = y;
		return result;
	}

	inline int WriteCoord(int const pos[2], char coord[3])
	{
		coord[0] = 'A' + pos[0];
		if( coord[0] >= 'I' )
			++coord[0];
		if( pos[1] > 9 )
		{
			coord[1] = '0' + ( pos[1] + 1 ) / 10;
			coord[2] = '0' + ( pos[1] + 1 ) % 10;
			return 3;
		}

		coord[1] = '0' + (pos[1] + 1);
		return 2;
	}

	struct PlayVertex
	{
		union
		{
			struct
			{
				short x;
				short y;
			};
			int data;
		};

		bool operator == (PlayVertex const& rhs) const { return data == rhs.data; }
		bool operator != (PlayVertex const& rhs) const { return !this->operator==(rhs); }

		bool isOnBoard() const { return data > 0; }

		static PlayVertex Pass()
		{
			PlayVertex result; 
			result.data = -2;
			return result;
		}
		static PlayVertex Resign()
		{
			PlayVertex result;
			result.data = -3;
			return result;
		}
		static PlayVertex Undefiend()
		{
			PlayVertex result;
			result.data = -1;
			return result;
		}
	};

	class Board
	{
	public:
		class Pos
		{
		public:
			Pos(){}
			explicit Pos( int idx ):index( idx ){}
			int toIndex() const { return index; }
		private:
			friend class Board;
			int index;
		};

		typedef char DataType;
		enum Dir
		{
			eLeft  = 0,
			eRight  ,
			eTop    ,
			eBottom ,

			NumDir ,
		};

		Board();
		~Board();


		void     copy(Board const& other);
		void     setup( int size , bool bClear = true);
		void     clear();


		bool     checkRange( int x , int y ) const;
		int      getSize() const { return mSize; }

		Pos      getPos( int x , int y ) const { assert( checkRange( x, y ) ); return Pos( getDataIndex( x , y ) );}
		Pos      getPos( int idx ) const { return Pos( idx ); }
		void     getPosCoord(int idx, int outCoord[2]) const
		{
			outCoord[0] = idx % getDataSizeX();
			outCoord[1] = idx / getDataSizeX() - 1;
		}

		bool     getLinkPos( Pos const& pos , int dir , Pos& result ) const;

		DataType getData( int x , int y ) const;
		DataType getData( Pos const& p ) const { return DataType( getData( p.toIndex() ) ); }

		int      getLiberty(Pos const& p) const
		{

			return 0;
		}
		
		int      getCaptureCount( Pos const& p ) const { return getCaptureCount( p.toIndex() ); }
		int      getCaptureCount( int x , int y ) const;

		void     putStone( Pos const& p , DataType color ){ putStone( p.toIndex()  , color ); }
		int      fillLinkedStone( Pos const& p , DataType color );
		void     removeStone( Pos const& p );
		int      captureLinkedStone( Pos const& p );

		int      peekCaptureStone( Pos const& p , unsigned& bitDir) const;

		int      getLinkToRootDist(Pos const& p) const {  return getLinkToRootDist(p.toIndex());  }

		void     calcScore(int outScores[2])
		{
			for( int i = 0; i < getSize(); ++i )
			{

				outScores[0] = outScores[1] = 0;
				for( int j = 0; j < getSize(); ++j )
				{
					char data = getData(i, j);
					switch( data )
					{
					case StoneColor::eBlack:
						outScores[0] += 1;
						break;
					case StoneColor::eWhite:
						outScores[1] += 1;
						break;
					case StoneColor::eEmpty:
						break;
					}
				}
			}


		}
	private:
		typedef short LinkType;

		int      calcLinkIndex( int idx , int dir ) const { return idx + mIndexOffset[ dir ]; }
		int      offsetIndex( int idx , int ox , int oy ){  return idx + ox + oy * getDataSizeX(); }

		DataType getData( int idx )  const  { return mData[ idx ]; }
		int      getDataSize()  const { return getDataSizeX() * getDataSizeY(); }
		int      getDataSizeY() const { return mSize + 2; }
		int      getDataSizeX() const { return mSize + 1; }
		int      getDataIndex( int x , int y ) const {  return x + getDataSizeX() * ( y + 1 );  }

		void     putStone( int idx, DataType color );

		int      getCaptureCount( int index ) const;
		void     addRootCaptureCount( int idxRoot , int val );
		int      getLiberty_R(int idx) const;

		int      relink( int idx );
		void     linkRoot( int idxRoot );
		int      getLinkRoot( int idx ) const;
		int      getLinkToRootDist(int idx) const;

		int      relink_R( int idx );
		int      fillLinkedStone_R( int idx );
		int      captureLinkedStone_R( int idx );
		int      peekCaptureConStone(Pos const& p) const;
		int      peekCaptureLinkedStone_R( int idx ) const;
   

		void     removeVisitedMark_R( int idx ) const;


		struct IntersectionData
		{
			char     data;
			uint8    checkCount;
			LinkType link;
		};

		//IntersectionData* mData;

		int       mIndexOffset[ NumDir ];
		mutable DataType mCacheColorR;
		int       mCacheIdxConRoot;

		int       mSize;
		mutable std::unique_ptr< LinkType[] > mLinkIndex;
		std::unique_ptr< char[] > mData;
	};

	struct GameRule
	{
		int   sec; // 25 hand
		int   komi; //�K��
		float handicapNum;
	};

	struct GameSetting
	{
		int   boardSize;
		int   numHandicap;
		float komi;
		bool  bBlackFrist;
		bool  bFixedHandicap;

		GameSetting()
		{
			boardSize = 19;
			numHandicap = 0;
			komi = 6.5;
			bBlackFrist = true;
			bFixedHandicap = true;
		}

	
		bool operator == (GameSetting const& rhs)
		{
			return boardSize == rhs.boardSize &&
				numHandicap == rhs.numHandicap &&
				komi == rhs.komi &&
				bBlackFrist == rhs.bBlackFrist 	&& 
				bFixedHandicap == rhs.boardSize;
		}
	};


	struct GameDescription
	{
		std::string blackName;
		std::string whiteName;
		std::string date;
	};

	class IGameCopier
	{
	public:
		virtual void emitSetup(GameSetting const& setting) = 0;
		virtual void emitPlayStone(int x, int y , int color ) = 0;
		virtual void emitAddStone(int x, int y, int color) = 0;
		virtual void emitPlayPass(int color) = 0;
		virtual void emitUndo() = 0;
	};

	class Game
	{
	public:
		typedef Board::DataType DataType;
		typedef Board::Pos      Pos;

		Game();

		void    setup( int size );
		GameSetting const& getSetting() const { return mSetting; }
		void    setSetting(GameSetting const& setting) { mSetting = setting; }
		void    restart();
		bool    canPlay(int x, int y) const;
		bool    addStone(int x, int y, DataType color);
		void    addStone(Pos const& pos, DataType color){  assert(!isReviewing()); addStoneInternal(pos, color, false);  }
		bool    playStone(int x, int y);
		bool    playStone(Pos const & pos) { assert(!isReviewing()); return playStoneInternal(pos, false); }
		
		void    playPass() { assert(!isReviewing()); return playPassInternal(false); }
		bool    undo() { assert(!isReviewing()); return undoInternal(false); }

		void    copy(Game const& other);
		void    synchronizeState(IGameCopier& copier, bool bReviewOnly = false) const;
		void    synchronizeStateKeep(IGameCopier& copier, int startStep, bool bReviewOnly = false) const;
		
		void    synchronizeHistory(Game& other) const
		{
			if( mStepHistory.size() != other.mStepHistory.size() )
			{
				other.mStepHistory.insert(other.mStepHistory.end(), mStepHistory.begin() + other.mStepHistory.size(), mStepHistory.end());
			}
		}
		void    removeUnplayedHistory()
		{
			if( mStepHistory.size() > mCurrentStep )
			{
				mStepHistory.resize( mCurrentStep );
			}
		}

		struct KOState
		{
			int    index;
			uint64 hash;
			static KOState Invalid() { return{ -1 , 0 }; }

			bool operator == (KOState const& rhs) const
			{
				return index == rhs.index && hash == rhs.hash;
			}
		};
		struct StepInfo
		{
			int16   idxPos;
			union
			{
				uint8    captureDirMask;
				DataType colorAdded;
			};
			uint8  bPlay;
		};
		std::vector< StepInfo > const& getStepHistory() const { return mStepHistory; }

		DataType getNextPlayColor() const { return mNextPlayColor; }
		DataType getFristPlayColor() const { return mSetting.bBlackFrist ? StoneColor::eBlack : StoneColor::eWhite; }
		Board const& getBoard() const { return mBoard; }

		int     getBlackCapturedNum() const { return mNumBlackCaptured; }
		int     getWhiteCapturedNum() const { return mNumWhiteCaptured; }

		void    print( int x , int y );

		bool    saveSGF( char const* path , GameDescription const* description = nullptr ) const;

		int     getCurrentStep() const	{  return mCurrentStep; }
		int     getLastStep() const  {  return mStepHistory.size() - 1;  }
		bool    getStepPos(int step, int outPos[2]) const;
		bool    getLastStepPos(int outPos[2]) const { return getStepPos(getCurrentStep() - 1, outPos); }

		int     getLastPassCount() const;

		//review
		bool    isReviewing() const;
		int     reviewBeginStep();
		int     reviewPrevSetp(int numStep = 1);
		
		int     reviewNextStep(int numStep = 1);
		int     reviewLastStep();

		bool    isKOStateReached(Pos const& pos, DataType playColor, KOState const& koState) const;
	private:
		
		void     doRestart( bool beClearBoard , bool bClearStepHistory = true);
		bool     playStoneInternal(Pos const& pos, bool bReviewing);
		void     playPassInternal(bool bReviewing);
		void     addStoneInternal(Pos const& pos, DataType color, bool bReviewing);

		bool     undoInternal(bool bReviewing);
		
		int      captureStone( Pos const& pos, unsigned& bitDir);
		void     advanceStepFromHistory();

		GameSetting mSetting;
		int       mCurrentStep;
		int       mNumBlackCaptured;
		int       mNumWhiteCaptured;
		mutable Board mBoard;
		
		DataType  mNextPlayColor;

		KOState   calcKOState(Pos const& pos) const;
		void      addKOState( DataType playColor, Pos const* pos , int numCapture, KOState const& koState);
		void      removeKOState( DataType playColor , Pos const* pos );
		std::vector< KOState >   mSimpleKOStates;

		typedef std::vector< StepInfo > StepVec;
		StepVec   mStepHistory;

		void takeData( SocketBuffer& buffer );
		bool restoreData( SocketBuffer& buffer );
		
	};

}//namespace Go

#endif // GoCore_h__