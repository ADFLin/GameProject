#ifndef SortFunction_h__
#define SortFunction_h__

template < class Iter , class Cmp >
void InsertSort( Iter begin , Iter end , Cmp cmp = Cmp() )
{
#if 0
	Iter it = begin;
	Iter prev = it;
	++it;
	for( ; it != end ; ++it )
	{
		Iter hold = it;
		for( Iter it2 = begin ; it2 != prev ; ++it2 )
		{
			if ( cmp( *it2 , *hold ) )
		}

	}
	for( int i = 1 ; i < num ; ++i )
	{
		int cmpIndex = i - 1;
		for(  ; cmpIndex >= 0 ; --cmpIndex )
		{
			if ( array[cmpIndex] < array[ cmpIndex + 1] )
				break;
			else
			{
				std::swap( array[cmpIndex] , array[ cmpIndex + 1 ] );
			}
		}
	}
#endif
}


namespace SortImpl
{
	int GetChildIndex(int index)
	{
		return 2 * index;
	}

	int GetParentIndex(int index)
	{
		return index / 2;
	}
		

	template < class Iter , class Cmp >
	void HeapFilterDown(Iter iter, int index , int indexMax )
	{
		while( index <= indexMax )
		{
			Iter child = std::next(iter, GetChildIndex(index) - index );
			if( Cmp()(*iter , *child) )
			{
				return;
			}
			std::swap(*iter, *child);
			Iter child2 = std::next(child, 1);

			if (  )

		}
	}

	template < class Iter  , class Cmp >
	void HeapFilterUp( Iter iter, int index, int indexMax)
	{

	}
}

template< class Iter >
void HeapSort( Iter begin , Iter end )
{


}

#endif // SortFunction_h__
