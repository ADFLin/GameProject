#ifndef Holder_HPP
#define Holder_HPP

template<class T>
class Holder
{
public:
	Holder():m_ptr(0){}
	explicit Holder(T* ptr):m_ptr(ptr){}
	~Holder(){	delete m_ptr;  }
	T& operator*() const  { return *m_ptr; }
	T* operator->() const { return m_ptr; }
	operator T*() const { return m_ptr;  }

	void reset( T* ptr )
	{
		delete m_ptr;
		m_ptr = ptr;
	}

	T* release()
	{ 
		T* temp = m_ptr; 
		m_ptr = NULL ; 
		return temp;
	}
	T* get() const { return m_ptr; }

private:
	Holder<T>& operator=(T* ptr);
	Holder( Holder<T> const& );
	Holder<T>& operator=(Holder<T> const&);
	T* m_ptr;
};

#endif //Holder_HPP