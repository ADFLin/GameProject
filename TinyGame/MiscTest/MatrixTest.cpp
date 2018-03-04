

#include "Stage/TestStageHeader.h"

#include <algorithm>
#include "CompilerConfig.h"

namespace Homework
{
	class MatrixBase
	{
	public:
		MatrixBase(size_t inRow, size_t inCol)
		{
			col = inCol;
			row = inRow;
		}
		size_t col, row;
		size_t toIndex(size_t i, size_t j) const
		{
			assert(i < row && j < col);
			return i * col + j;
		}
	};


	template< class T >
	class SparseMatrix : public MatrixBase
	{

	public:
		SparseMatrix(size_t inRow , size_t inCol )
			:MatrixBase(inRow,inCol)
		{
		}
		struct Proxy
		{
			Proxy const& operator = (T const& value) const
			{
				if( idxLowerBound < matrix.data.size() && matrix.data[idxLowerBound].index == index )
					matrix.data[idxLowerBound].value = value;
				else
					matrix.data.insert( matrix.data.begin() + idxLowerBound, { index , value });
				return *this;
			}

			operator T() const
			{ 
				if ( idxLowerBound < matrix.data.size() && matrix.data[idxLowerBound].index == index )
					return matrix.data[idxLowerBound].value;
				return 0;
			}
			SparseMatrix& matrix;
			size_t idxLowerBound;
			size_t index;
		};
		Proxy operator()( size_t i, size_t j )
		{
			size_t index = toIndex(i, j);
			size_t idxLowerBound = lowerBound(index);
			return { *this , idxLowerBound , index };
		}

		T const& operator()( size_t i, size_t j ) const
		{
			size_t index = toIndex(i, j);
			size_t idxLowerBound = lowerBound(index);
			if( data[idxLowerBound].index == index )
				return data[idxLowerBound].value;
			return 0;
		}

		void set(size_t i, size_t j, T const& value)
		{
			size_t index = toIndex(i, j);
			size_t idxLowerBound = lowerBound(index);
			if( idxLowerBound < data.size() && data[idxLowerBound].index == index )
				data[idxLowerBound].value = value;
			else
				data.insert(matrix.data.begin() + idxLowerBound, { index , value });
		}


		struct Element
		{
			size_t index;
			T value;
		};

#define USE_LEGACY_METHOD 0

		struct CSData
		{
			std::vector< Element > data;
#if USE_LEGACY_METHOD
			std::vector< size_t >  offsets;
#else
			std::vector< std::pair< size_t, size_t > >  offsets;
#endif
		};

		CSData makeCSR() const
		{
			assert(!data.empty());

			std::vector< Element > tempData = data;
			
#if USE_LEGACY_METHOD
			std::vector< size_t >  tempOffsets;
			tempOffsets.resize(row + 1);
			tempOffsets[0] = 0;
			size_t cur = 0;
#else
			std::vector< std::pair< size_t , size_t > >  tempOffsets;
			size_t cur = -1;
#endif
			for( size_t n = 0; n < tempData.size(); ++n )
			{
				auto& e = tempData[n];
				size_t idxRow = e.index / col;
				size_t idxCol = e.index % col;

				e.index = idxCol;
				if( idxRow != cur )
				{
#if USE_LEGACY_METHOD
					for( size_t i = cur + 1; i <= idxRow; ++i )
						tempOffsets[i] = n;
#else
					tempOffsets.push_back({idxRow,n});
#endif
					cur = idxRow;
				}
			}

#if USE_LEGACY_METHOD
			for( size_t i = cur + 1; i <= row; ++i )
				tempOffsets[i] = tempData.size();
#else
			tempOffsets.push_back({ row,tempData.size() });
#endif
			return{ std::move(tempData) , std::move(tempOffsets) };
		}

		CSData makeCSC() const
		{
			assert(!data.empty());
			
			//TODO
			std::vector< Element > tempData = data;
			

			for( auto& e : tempData )
			{
				size_t idxRow = e.index / col;
				size_t idxCol = e.index % col;

				e.index = idxCol * row + idxRow;
			}

			std::sort( tempData.begin() , tempData.end() , 
				[](Element const& a, Element const& b)
				{
					return a.index < b.index;
				});

#if USE_LEGACY_METHOD
			std::vector< size_t >  tempOffsets;
			tempOffsets.resize(col + 1);
			tempOffsets[0] = 0;
			size_t cur = 0;
#else
			std::vector< std::pair< size_t, size_t > >  tempOffsets;
			size_t cur = -1;
#endif
			for( size_t n = 0 ; n < tempData.size() ; ++n )
			{
				auto& e = tempData[n];
				size_t idxRow = e.index % row;
				size_t idxCol = e.index / row;

				e.index = idxRow;
				if( idxCol != cur )
				{
#if USE_LEGACY_METHOD
					for( size_t i = cur + 1; i <= idxCol; ++i )
						tempOffsets[i] = n;
#else
					tempOffsets.push_back( { idxCol , n } );
#endif
					cur = idxCol;
				}
			}

#if USE_LEGACY_METHOD
			for( size_t i = cur + 1; i <= col; ++i )
				tempOffsets[i] = tempData.size();
#else
			tempOffsets.push_back({ col ,tempData.size() });
#endif
			return{ std::move(tempData) , std::move(tempOffsets) };
		}

		SparseMatrix operator * ( SparseMatrix const& rhs )
		{
			assert(col == rhs.row);
			if ( data.empty() || rhs.data.empty() )
				return SparseMatrix(row, rhs.col);

			CSData csr = makeCSR();
			CSData csc = rhs.makeCSC();

			std::vector< Element > tempData;


#if USE_LEGACY_METHOD
			for( size_t j = 0; j < rhs.col; ++j )
			{
				size_t numC = csc.offsets[j + 1] - csc.offsets[j];
				if( numC == 0 )
					continue;
				size_t idxCol = j;
				auto pDataC = &csc.data[csc.offsets[j]];
#else
			for( size_t j = 0; j < csc.offsets.size() - 1; ++j )
			{
				size_t numC = csc.offsets[j + 1].second - csc.offsets[j].second;
				if( numC == 0 )
					continue;
				size_t idxCol = csc.offsets[j].first;
				auto pDataC = &csc.data[csc.offsets[j].second];
#endif


#if USE_LEGACY_METHOD
				for( size_t i = 0; i < row; ++i )
				{
					size_t numR = csr.offsets[i + 1] - csr.offsets[i];
					if( numR == 0 )
						continue;
					size_t idxRow = i;
					auto pDataR = &csr.data[csr.offsets[i]];
#else
				for( size_t i = 0; i < csr.offsets.size() - 1; ++i )
				{
					size_t numR = csr.offsets[i + 1].second - csr.offsets[i].second;
					if( numR == 0 )
						continue;
					size_t idxRow = csr.offsets[i].first;
					auto pDataR = &csr.data[csr.offsets[i].second];
#endif
					
					//dot
					size_t idxC = 0;
					size_t idxR = 0;
					T value = 0;
					while(idxR < numR && idxC < numC)
					{
						auto const& ec = pDataC[idxC];
						auto const& er = pDataR[idxR];
						if( ec.index == er.index )
						{
							value += ec.value * er.value;
							++idxC;
							++idxR;
						}
						else if( ec.index < er.index )
						{
							++idxC;
						}
						else
						{
							++idxR;
						}
					}
					if( value != 0 )
					{
						tempData.push_back({ idxRow * rhs.col + idxCol , value });
					}

				}
			}
			return SparseMatrix(row,rhs.col,std::move(tempData));
		}
		template< class Fun >
		SparseMatrix opFun(SparseMatrix const& rhs) const
		{
			assert(col == rhs.col && row == rhs.row);

			if( data.empty() )
			{
				if( rhs.data.empty() )
					return SparseMatrix(col, row);
				else
					return SparseMatrix(col, row, rhs.data);
			}
			if ( rhs.data.empty() )
				return SparseMatrix(col, row, data);


			std::vector< Element > tempData;
			size_t idx1 = 0;
			size_t idx2 = 0;
			for(;; )
			{
				auto const& e1 = data[idx1];
				auto const& e2 = rhs.data[idx2];
				if( e1.index == e2.index )
				{
					T value = Fun()(e1.value, e2.value);
					if ( value != 0 )
						tempData.push_back({ e1.index  , });
					++idx1;
					++idx2;
				}
				else if( e1.index < e2.index )
				{
					if ( !Fun::bZeroMissmatch )
						tempData.push_back({ e1.index  , e1.value });
					++idx1;
				}
				else
				{
					if( !Fun::bZeroMissmatch )
						tempData.push_back({ e2.index  , e2.value });
					++idx2;
				}

				if ( idx1 >= data.size() )
				{
					if( !Fun::bZeroMissmatch )
					{
						while( idx2 < rhs.data.size() )
						{
							auto const& ele = rhs.data[idx2++];
							tempData.push_back({ ele.index , ele.value });
						}
					}
					break;
				}
				else if( idx2 >= rhs.data.size() )
				{
					if( !Fun::bZeroMissmatch )
					{
						while( idx1 < data.size() )
						{
							auto const& ele = data[idx1++];
							tempData.push_back({ ele.index , ele.value });
						}
					}
					break;
				}
			}

			return SparseMatrix(col, row, std::move(tempData));
		}

		FORCEINLINE SparseMatrix operator + (SparseMatrix const& rhs) const
		{
			struct Fun
			{
				enum { bZeroMissmatch = false };
				float operator()(T const& v1, T const& v2) const
				{
					return v1 + v2;
				}
			};
			return opFun< Fun >( rhs );
		}
		FORCEINLINE SparseMatrix operator - (SparseMatrix const& rhs) const
		{
			struct Fun
			{
				enum { bZeroMissmatch = false };
				float operator()(T const& v1, T const& v2) const
				{
					return v1 - v2;
				}
			};
			return opFun< Fun >( rhs );
		}

		size_t lowerBound(size_t index) const
		{
			size_t size = data.size();
			size_t result = 0;
			while( size > 0 )
			{
				size_t size2 = size / 2;
				size_t mid = result + size2;
				if( data[mid].index <= index )
				{
					result = mid + 1;
					size -= size2 + 1;
				}
				else
				{
					size = size2;
				}
			}
			return result;
		}

		std::vector< Element > data;

	private:

		SparseMatrix(size_t inCol, size_t inRow , std::vector< Element >&& inData )
			:MatrixBase(inCol, inRow)
			,data(std::move(inData))
		{
		}
		SparseMatrix(size_t inCol, size_t inRow, std::vector< Element > const& inData)
			:MatrixBase(inCol, inRow)
			, data(inData)
		{
		}
	};

	template< class T >
	class SparseMatrixCSR : public MatrixBase
	{
	public:
		SparseMatrixCSR(typename SparseMatrix<T>::CSData&& cdata)
			:data(std::move(cdata.data))
			,offsets(std::move(cdata.offsets))
		{

		}
		SparseMatrixCSR(size_t inCol, size_t inRow)
			:MatrixBase(inCol, inRow)
		{
			offsets.resize(inRow + 1, 0);
		}

		SparseMatrixCSR(SparseMatrix<T> const& rhs)
		{



		}

		void mulVector(T* v, T* outValue)
		{
			for( size_t i = 0; i < row; ++i )
			{
				outValue[i] = 0;
				size_t num = offsets[i + i] - offsets[i];
				for( size_t j = 0; j < num; ++j )
				{
					size_t offset = offsets[i] + j;

					auto const& e = data[idx];
					outValue[i] += v[e.index] * e.value;
				}
			}
		}

		struct Element
		{
			size_t index;
			T value;
		};
		std::vector< Element > data;
		std::vector< size_t >  offsets;
	};

	template< class T >
	class Matrix : public MatrixBase
	{
	public:

		Matrix(size_t inRow, size_t inCol, T initValue = 0)
			:MatrixBase(inRow, inCol)
			, data(inCol * inRow, initValue)
		{
		}

		Matrix(SparseMatrix const& rhs)
			:MatrixBase(rhs.row, rhs.col)
			,data(rhs.row*rhs.col , 0)
		{
			for( auto const& e : rhs.data )
			{
				data[e.index] = e.value;
			}
		}
		T const& operator()(size_t i, size_t j) const
		{
			return data[toIndex(i, j)];
		}
		T& operator()(size_t i, size_t j)
		{
			return data[toIndex(i, j)];
		}

		Matrix& operator += (SparseMatrix<T> const& rhs)
		{
			for( auto const& e : rhs.data )
			{
				data[e.index] += e.value;
			}
			return *this;
		}

		Matrix& operator + (SparseMatrix<T> const& rhs)
		{
			Matrix result(*this);
			result += rhs;
			return result;
		}

		Matrix& operator -= (SparseMatrix<T> const& rhs)
		{
			for( auto const& e : rhs.data )
			{
				data[e.index] += e.value;
			}
			return *this;
		}

		Matrix& operator - (SparseMatrix<T> const& rhs)
		{
			Matrix result(*this);
			result -= rhs;
			return result;
		}
	private:
		std::vector<T> data;
	};


	void Test()
	{
		SparseMatrix< float > m1(3,5);
		m1(1, 1) = 2;
		m1(1, 2) = 3;
		m1(2, 4) = 3;
		SparseMatrix< float > m2(5,3);
		m2(0, 0) = 5;
		m2(0, 1) = 1;
		m2(1, 1) = 2;
		m2(1, 2) = 4;
		m2(4, 2) = 3;

		SparseMatrix< float > m3 = m1 * m2;
		//SparseMatrix< float > m3 = m1 + m2;

		::LogMsgF("%f", m3(1, 2));


	}


	REGISTER_MISC_TEST("MatrixTest", Test);
}

