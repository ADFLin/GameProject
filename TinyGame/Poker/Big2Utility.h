#ifndef Big2Utility_h__
#define Big2Utility_h__

#include "PokerBase.h"

namespace Poker { namespace Big2 {

	enum CardGroup
	{
		CG_SINGLE         ,
		CG_ONE_PAIR       ,
		CG_TWO_PAIR       ,
		CG_THREE_OF_KIND  ,
		CG_STRAIGHT       ,
		CG_FLUSH          ,
		CG_FULL_HOUSE     ,
		CG_FOUR_OF_KIND   ,
		CG_STRAIGHT_FLUSH ,
		CG_NONE ,
	};

	struct  TrickInfo
	{
		CardGroup group;
		int       power;
		int       num;
		Card      card[ 5 ];
	};

	struct CardSortCmp
	{
		bool operator()( Card const& lhs , Card const& rhs ) const
		{
			if ( lhs.getFaceRank() < rhs.getFaceRank() )
				return true;
			else if ( lhs.getFaceRank() == rhs.getFaceRank() &&
				lhs.getSuit() < rhs.getSuit() )
				return true;
			return false;
		}
	};

	class TrickUtility
	{
	public:
		static int     getGroupNum( CardGroup group );
	
		static bool    canSuppress(  TrickInfo const& info ,  TrickInfo const& prevInfo );
		static int     getRankPower( int rank );
		static int     calcPower( Card const& card );
		static int     calcPower( CardGroup group , Card cards[] );
		static bool    checkCard( Card const cards[] , int numCard , int index[] , int num ,  TrickInfo& info );

	private:
		static bool    checkCard5( TrickInfo& info );
	};

	class TrickIterator;

	enum GroupPos
	{
		POS_SINGLE ,
		POS_PAIR ,
		POS_THREE_OF_KIND ,
		POS_FOUR_OF_KIND ,
		POS_STRAIGHT ,
		POS_FLUSH    ,
		POS_STRAIGHT_FLUSH ,

		NumGroupPos ,
	};

	class TrickHelper
	{
	public:
		TrickHelper(){ init(); }

		void    parse( Card cards[] , int num );
		bool    haveGroup( CardGroup group );
		int     getFaceNum( int faceRank );
		TrickIterator getIterator( CardGroup group );
		
	private:
		friend class TrickIterator;

		struct IterFace
		{
			int combine[5];
			int count;
			int cur;
			void initCombine( int num );
			bool nextCombine( int m , int n );
		};

		struct  IterFlush
		{
			int combine[14];
			int count;
			int cur;
			void initCombine( int num );
			bool nextCombine( int m , int n );
		};
		struct  IterStraight
		{
			int cur;
			int index[5];
		};
		struct IterData
		{
			union
			{
				IterFace     face[3];
				IterStraight straight;
				IterFlush    flush;
			};
			int condition;
		};

		bool initIterator ( CardGroup group , IterData& data );
		void valueIterator( CardGroup group , IterData& data , int outIndex[] , int* power );
		bool nextIterator ( CardGroup group , IterData& data );

		bool initIteratorSingle( IterFace& data );
		bool nextIteratorSingle( IterFace& data );
		void valueIteratorSingle( IterFace& data , int outIndex[] , int* power );
		bool initIteratorOnePair( IterFace& data );
		bool nextIteratorOnePair( IterFace& data );
		void valueIteratorOnePair( IterFace& data , int outIndex[] , int* power);
		bool initIteratorThreeOfKind( IterFace &data );
		bool nextIteratorThreeOfKind( IterFace& data );
		void valueIteratorThreeOfKind( IterFace& data, int outIndex[] , int* power );

		void addSuitGroup( Card const& card , int index );
		void addDataGroup( GroupPos pos , int index );
		struct FaceGroup
		{
			int rank;
			int num;
			int index;
			unsigned suitMask;
		};
		FaceGroup mFaceGroups[ 13 ];
		int mNumFaceGroup;

		struct DataGroup
		{
			int* index;
			int  num;
		};

		DataGroup& getData( GroupPos pos ){ return mDataGroups[ pos ]; }

		struct SuitGroup
		{
			int index[ 13 ];
			int num;
		};

		SuitGroup mSuitGroup[ 4 ];
		DataGroup mDataGroups[ NumGroupPos ];
		int       mIndexStorge[ 52 ];
		Card*     mParseCards;

		void init();

	};

	class TrickIterator
	{
	public:
		TrickIterator();
		int*      getIndex( int& num );
		bool      is(){ return mbe; }
		void      goNext();
		void      goNext( int power );
		void      reset();
		bool      reset( int power );
		CardGroup getGroup(){ return mGroup; }
	private:
		typedef TrickHelper::IterData IterData;
		friend class TrickHelper;
		TrickIterator( TrickHelper& helper , CardGroup group );
		CardGroup    mGroup;
		bool         mbe;
		int          mIndex[ 5 ];
		int          mPower;
		IterData     mIterData;
		TrickHelper* mHelper;
	};

}//namespace Big2
}//namespace Poker

#endif // Big2Utility_h__
