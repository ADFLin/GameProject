#ifndef SortFunction_h__
#define SortFunction_h__

template < class Iter , class Cmp >
void insertSort( Iter begin , Iter end , Cmp cmp = Cmp() )
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

#endif // SortFunction_h__
