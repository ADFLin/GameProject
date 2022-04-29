#include "CSPSolveStrategy.h"

namespace Mine
{
	bool Constraint::isCoupleWith( Constraint const& c )
	{
		for( int i = 0 ; i < numVar ; ++i )
		{
			CellData* var = variables[i];
			for( int j = 0 ; j < c.numVar ; ++j )
			{
				if ( var == c.variables[j] )
					return true;
			}
		}
		return false;
	}

	bool Constraint::trySimplify( Constraint& c )
	{
		if ( c.numVar >= numVar )
			return c.trySimplify( *this );

		int idxRemove[ MaxVariableNum ];
		for( int i = 0 ; i < c.numVar ; ++i )
		{
			int idx = 0;
			for( ; idx < numVar ; ++idx )
			{
				if ( variables[idx] == c.variables[i] )
				{
					idxRemove[ i ] = idx;
					break;
				}
			}
			if ( idx == numVar )
				return false;
		}

		int idxSwap = numVar - 1;
		for( int i = 0 ; i < c.numVar ; ++i )
		{
			if ( idxRemove[i] != idxSwap )
			{
				variables[ idxRemove[i] ] = NULL;
				variables[ idxRemove[i] ] = variables[ idxSwap ];
				--idxSwap;
			}
		}

		numVar   -= c.numVar;
		constant -= c.constant;

		return true;
	}

	CheckResult Constraint::check()
	{
		for( int i = 0 ; i < numVar ; ++i )
		{
			if ( variables[i]->number > 0  )
			{
				--numVar;
				variables[i] = variables[ numVar ];
				--i;
			}

			if ( variables[i]->number == CV_FLAG )
			{
				--constant;
			}	
		}

		if ( constant == 0 )
		{
			return CHECK_OPEN;
		}
		else if ( constant == numVar )
		{
			return CHECK_MARK;
		}
		return CHECK_NONE;
	}

	bool Constraint::findVariable( CellData* data )
	{
		for( int i = 0 ; i < numVar ; ++i )
		{
			if ( variables[i] == data )
				return true;
		}
		return false;
	}

	bool Constraint::isSatifity()
	{
		if ( curConstant > 0 )
			return true;

		if ( curConstant == 0 )
			return curNumCheckedVar == numVar;

		return false;
	}

	void Constraint::addVarrible( CellData& var )
	{
		assert( var.number == CV_UNPROBLED );
		variables[ numVar ] = &var;
		++numVar;
	}

	bool ConstraintGroup::trySpilt( ContraintVec& other )
	{
		other.reserve( constraints.size() );

		int size = constraints.size();
		int start = 0;
		//   start      end
		//   | i------>  |j------->
		//  [0][1][2][3][4]....  [size]
		for( int end = 0 ; end < size ; ++end )
		{
			for( int i = start ; i < end ; ++ i )
			{
				for( int j = end ; j < size ; ++j )
				{
					if ( constraints[j]->isCoupleWith( *constraints[i] ) )
					{
						if ( j != end )
						{
							std::swap( constraints[i] , constraints[j] );
						}

						goto next;
					}
				}
			}
			other.assign( constraints.begin() + end  , constraints.end());
			constraints.resize( end );
			return true;
next:
			;
		}
		return false;
	}

	ConstraintGroup::ConstraintGroup( Constraint& ct )
	{
		constraints.push_back( &ct );
		flag = eBuildNode;
	}


	void CSPSolveStrategy::loadMap(IMineControl& control )
	{
		restoreData(control);

		for( int i = 0 ; i < mData.size() ; ++i )
		{
			CellData& cell = mData[i];
			int num = control.lookCell( cell.x , cell.y , false );
			cell.number = num;
		}

		for( int i = 0 ; i < mData.size() ; ++i )
		{
			CellData& cell = mData[i];
			if ( cell.number >= 0 )
			{
				++mNumSolvedCell;
				if ( cell.number > 0 )
				{
					buildConstraint( cell , NULL );
				}
			}
		}
	}

	void CSPSolveStrategy::buildConstraint( CellData& cell , ConstraintGroup* group )
	{
		cell.ct.init();
		cell.ct.constant = cell.number;
		for( int i = cell.x1 ; i <= cell.x2 ; ++i )
		{
			for( int j = cell.y1 ; j <= cell.y2 ; ++j )
			{
				int idx = i + j * mSizeX;
				CellData& neighbor = mData[idx];
				if ( neighbor.number == CV_FLAG )
				{
					cell.ct.constant -= 1;
				}
				else if ( neighbor.number == CV_UNPROBLED )
				{
					cell.ct.addVarrible( mData[idx] );
				}
			}
		}

		if ( cell.ct.constant == cell.ct.numVar )
		{
			for( int i = 0 ; i < cell.ct.numVar; ++i )
			{
				SolveInfo info;
				info.number = CV_FLAG;
				info.cell   = cell.ct.variables[i];
				mSolvedCell.push_back( info );
			}
			cell.ct.numVar = 0;
		}

		if ( group )
		{
			if ( !group->addConstraint( cell.ct ) )
				return;
			cell.group = group;
		}

		bool needNewGroup = true;
		for ( int i = 0 ; i < cell.ct.numVar ; ++i )
		{
			CellData* var = cell.ct.variables[i];

			if ( var->group == cell.group )
				continue;

			if ( cell.group )
			{
				mergeCTGroup( var->group , cell.group );
			}
			else
			{
				if ( !var->group->addConstraint( cell.ct ) )
					return;

				var->group = var->group;
			}
		}

		if ( cell.group == nullptr )
		{
			cell.group = new ConstraintGroup( cell.ct );
			addCTGroup( cell.group );
		}
	}

	void CSPSolveStrategy::restoreData(IMineControl& control)
	{
		mControl = &control;

		mSizeX = mControl->getSizeX();
		mSizeY = mControl->getSizeY();

		mData.resize( mSizeX * mSizeY );

		int idx = 0;
		int xMax = mSizeX - 1;
		int yMax = mSizeY - 1;
		for( int j = 0 ; j < mSizeY ; ++j )
		{
			for (int i = 0; i < mSizeX; ++i, ++idx)
			{
				CellData& data = mData[idx];
				data.x = i;
				data.y = j;
				data.x1 = std::max(0, i - 1);
				data.y1 = std::max(0, j - 1);
				data.x2 = std::min(xMax, i + 1);
				data.y2 = std::min(yMax, j + 1);
				data.group = NULL;
				data.number = CV_UNPROBLED;
			}
		}

		for (int i = 0; i < mCTGroup.size(); ++i)
		{
			delete mCTGroup[i];
		}

		mCTGroup.clear();
		mGroupUpdate.clear();
	}

	void CSPSolveStrategy::restart(IMineControl& control)
	{
		restoreData(control);
	}

}//namespace csp
