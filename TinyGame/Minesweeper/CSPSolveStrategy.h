#ifndef CSPSolveStrategy_h__
#define CSPSolveStrategy_h__

#include "GameInterface.h"
#include <vector>
#include <cassert>
#include <algorithm>


//constraint satifity problem
namespace Mine
{
	int const MaxVariableNum = 8;

	struct CellData;
	class  ConstraintGroup;
	class  CSPSolveStrategy;

	enum CheckResult
	{
		CHECK_NONE ,
		CHECK_OPEN ,
		CHECK_MARK ,
	};
	class Constraint
	{
	public:
		void  init()
		{
			numVar   = 0;
		}
		bool  isCoupleWith( Constraint const& c );
		bool  trySimplify( Constraint& c );


		CheckResult  check();
		bool  findVariable( CellData* data );
		bool  isSatifity();
		void  addVarrible( CellData& var );

		int       curConstant;
		int       curNumCheckedVar;
		int       constant;
		int       numVar;
		CellData* variables[ MaxVariableNum ];
	};


	struct CellData
	{
		int        x, y;
		int        x1,x2,y1,y2;
		int        number;
		Constraint ct;
		ConstraintGroup* group;

		static CellData* cast( Constraint* ct )
		{
			char* ptr = reinterpret_cast< char* >( ct ) - 
				reinterpret_cast< int >( &((CellData*)0)->ct );
			return reinterpret_cast< CellData* >( ct );
		}
	};

	typedef std::vector< Constraint* > ContraintVec;

	class ConstraintGroup
	{
	public:
		ConstraintGroup( Constraint& ct );
		static int const MaxVariableNum = 8;
		struct SNode 
		{
			bool checkConstraint()
			{
				for( int i = 0 ; i < numCheckCT ; ++i )
				{
					if ( !checkCT[i]->isSatifity() )
						return false;
				}
				return true;
			}


			void setUnAssignValue()
			{

			}

			bool updateAssignValue()
			{
				if ( assignValue > 1 )
					return false;

				++assignValue;
				if ( assignValue )
				{
					for( int i = 0 ; i < numVarCT ; ++i )
						varCT[i];
				}
			}
			CellData*   variable;
			Constraint* varCT[MaxVariableNum];
			int numVarCT;
			Constraint* checkCT[MaxVariableNum];
			int numCheckCT;
			int assignValue;
		};


		bool trySimplify( Constraint& ct )
		{
			bool result = false;
			for( ContraintVec::iterator iter = constraints.begin() , itEnd = constraints.end();
				 iter != itEnd ; ++iter )
			{
				Constraint* c = *iter;
				assert( c != &ct );
				result |= c->trySimplify( ct );
			}
			return result;
		}

		bool addConstraint( Constraint& ct )
		{
			if ( trySimplify( ct ) )
			{
				if ( ct.numVar == 0 )
					return false;
			}
			constraints.push_back( &ct );
			flag |= eBuildNode;
			return true;
		}

		void addConstraint( ContraintVec const& ctVec )
		{
			constraints.insert( constraints.end() , ctVec.begin() , ctVec.end() );
			flag |= eBuildNode;
		}

		void buildNode()
		{
			for( int i = 0 ; i < constraints.size() ; ++i )
			{
				Constraint& ct = *constraints[i];
				for( int n = 0 ; n < ct.numVar ; ++n )
				{
					bool beFound = false;
					for( int idx = 0; idx < nodes.size() ; ++idx )
					{
						SNode& node = nodes[i];
						if ( ct.variables[i] == node.variable )
						{
							beFound = true;
							node.varCT[ node.numVarCT++ ] = &ct;
						}
					}
					if ( !beFound )
					{
						nodes.push_back( SNode() );
						nodes.back().variable = ct.variables[i];
					}
				}
			}

			struct NodeCmp
			{
				bool operator()( SNode const& n1 , SNode const& n2 )
				{
					return n1.numVarCT < n2.numVarCT;
				}
			};
			std::sort( nodes.begin() , nodes.end() , NodeCmp() );
		}


		bool search()
		{
			int level = 0;
			int numSolution = 0;

			while( 1 )
			{
				if ( level == nodes.size() )
				{

				}



			}
		}
		std::vector< SNode > nodes;

		bool trySpilt( ContraintVec& other );

		enum
		{
			eCheckSplit  = 1 << 0,
			eBuildNode   = 1 << 1,
			eCheckUpdate = 1 << 2,
		};

		ContraintVec constraints;
		unsigned     flag;
	};

	class CSPSolveStrategy : public ISolveStrategy
	{
	public:
		virtual void restart(IMineControl& control);
		virtual void loadMap(IMineControl& control);
		virtual bool solveStep()
		{
			if ( !mSolvedCell.empty() )
			{
				SolveInfo info = mSolvedCell.back();
				mSolvedCell.pop_back();
				assert( info.number != CV_UNPROBLED );
			}
			return true;
		}


		void restoreData(IMineControl& control);

		void  addUpdateCTGroup( ConstraintGroup* group )
		{
			mGroupUpdate.push_back( group );
			group->flag |= ConstraintGroup::eCheckUpdate;
		}

		void  removeCTGroup( ConstraintGroup* group )
		{
			mCTGroup.erase( std::find( mCTGroup.begin() , mCTGroup.end() , group ) );
			delete group;
		}
		void  addCTGroup( ConstraintGroup* group )
		{
			mCTGroup.push_back( group );
		}

		void  mergeCTGroup( ConstraintGroup* g1 , ConstraintGroup* g2 )
		{
			g1->addConstraint( g2->constraints );
			for( ContraintVec::iterator iter = g2->constraints.begin() , itEnd = g2->constraints.end();
				  iter != itEnd ; ++iter )
			{
				CellData* cell = CellData::cast( *iter );
				cell->group = g1;
			}
			removeCTGroup( g2 );
		}

		void buildConstraint( CellData& cell , ConstraintGroup* group );

		void markCell( CellData& cell , Constraint* ct )
		{
			cell.number == CV_FLAG;
			++mNumSolvedCell;
			addUpdateCTGroup( cell.group );
		}

		void openCell( CellData& cell , Constraint* ct )
		{
			//ActionInfo action;
			//action.x = cell.x;
			//action.y = cell.y;
			//ActionData* data = new ActionData;
			//data->cell = &cell;
			//data->ct   = ct;
			//action.data = data;

			//mControl->addAction( action );
			//assert( cell.number == CN_UNPROBLED );
			//int num = mControl->probe( cell.x , cell.y );
			//if ( num >= 0 )
			//	updateSolveCell( cell , num , ct , true );
		}
		struct ActionData 
		{
			CellData*   cell;
			Constraint* ct;
		};

		void updateSolveCell( CellData& cell , int num , Constraint* ct , bool beWait )
		{
			assert( num >= 0 );
			cell.number = num;

			++mNumSolvedCell;
			if ( num > 0 )
			{
				ConstraintGroup* group = nullptr;
				if ( ct )
					cell.group = CellData::cast( ct )->group;
				buildConstraint( cell , group );
			}
			else if ( num == 0 )
			{
				for( int i = cell.x1 ; i <= cell.x2 ; ++i )
				for( int j = cell.y1 ; j <= cell.y2 ; ++j )
				{
					int idx = i + j * mSizeX;
					scanCell( mData[idx] , ct , false );
				}
			}

		}
		void scanCell( CellData& cell , Constraint* ct , bool beWait )
		{
			if ( cell.number != CV_UNPROBLED )
				return;

			int num = mControl->lookCell( cell.x , cell.y , beWait );

			if ( num >= 0 )
				updateSolveCell( cell , num , ct , false );

		}
		struct SolveInfo
		{
			CellData*   cell;
			ConstraintGroup* linkGroup;
			int         number;
		};

		std::vector< ConstraintGroup* > mGroupUpdate;
		std::vector< SolveInfo >        mSolvedCell;
		std::vector< CellData >         mData;
		std::vector< ConstraintGroup* > mCTGroup;
		IMineControl* mControl;
		int       mNumSolvedCell;
		int       mSizeX;
		int       mSizeY;
	};

}//namespace Mine


#endif // CSPSolveStrategy_h__