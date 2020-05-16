#ifndef AStarDefultPolicy_h__
#define AStarDefultPolicy_h__

#include <vector>
#include <algorithm>

namespace AStar
{
	template< class T , class CmpFunc >
	class STLHeapQueuePolicy
	{
	public:
		bool      empty(){ return mStorage.empty(); }
		void      insert(T const& val)
		{ 
			mStorage.push_back( val );
			push_heap( mStorage.begin() , mStorage.end() , CmpFunc() );
		}
		void      pop()
		{
			pop_heap( mStorage.begin() , mStorage.end() , CmpFunc() );
			mStorage.pop_back();
		}
		T&       front(){ return mStorage.front(); }
		void     clear(){ mStorage.clear(); }
	protected:
		std::vector<T> mStorage;
	};

	template< class T >
	class STLVecotorMapPolicy
	{
	public:
		void    insert(T const& val){ mImpl.push_back(val); }
		void    clear(){ mImpl.clear(); }

		typedef typename std::vector<T>::iterator iterator;
		void    erase( iterator iter ){ mImpl.erase( iter ); }

		iterator begin(){ return mImpl.begin(); }
		iterator end(){ return mImpl.end(); }
	protected:
		std::vector<T> mImpl;
	};

	template < class T>
	class NewDeletePolicy
	{
	public:
		T*   alloc(){ return new T; }
		void free(T* ptr){ delete ptr; }
	};


}//namespace AStar

#endif // AStarDefultPolicy_h__