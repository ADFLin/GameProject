#ifndef defalutPolicy_h__
#define defalutPolicy_h__

#include <vector>
#include <algorithm>

template< class T , class CmpFun >
class StlHeapOpenPolicy
{
public:
	typedef typename std::vector<T>::iterator iterator;

	bool empty(){ return m_vals.empty(); }
	inline void push(T const& val)
	{ 
		m_vals.push_back( val );
		push_heap( m_vals.begin() , m_vals.end() , CmpFun() );
	}
	inline void pop()
	{
		pop_heap( m_vals.begin() , m_vals.end() , CmpFun() );
		m_vals.pop_back();
	}
	T& front(){ return m_vals.front(); }
	void clear(){ m_vals.clear(); }
	iterator begin(){ return m_vals.begin(); }
	iterator end(){ return m_vals.end(); }
	inline void  erase( iterator iter )
	{
		m_vals.erase( iter );
		make_heap( m_vals.begin() , m_vals.end() , CmpFun() );
	}

	template < class FindNodeFun >
	iterator find_if( FindNodeFun fun )
	{
		return std::find_if( m_vals.begin() , m_vals.end() , fun );
	}

protected:
	std::vector<T> m_vals;
};


template< class T >
class StlVectorClosePolicy
{
public:
	typedef typename  std::vector<T>::iterator iterator;
	void push(T const& val){ m_vals.push_back(val); }
	void clear(){ m_vals.clear(); }
	void  erase( iterator iter ){ m_vals.erase( iter ); }

	iterator begin(){ return m_vals.begin(); }
	iterator end(){ return m_vals.end(); }

	template < class FindNodeFun >
	inline iterator find_if( FindNodeFun fun )
	{
		return std::find_if( m_vals.begin() , m_vals.end() , fun );
	}

protected:
	std::vector<T> m_vals;
};


template < class T>
class NewPolicy
{
public:
	T* alloc(){ return new T; }
	void free(T* ptr){ delete ptr; }
};

template< class SNode >
class NoDebugPolicy
{
public:
	void getCloseInfo(SNode* sNode){}
	void getOpenInfo(SNode* sNode){}
	void getCurrectStateInfo( SNode* sNode ){}
};

struct NoExtraData
{

};
#endif // defalutPolicy_h__