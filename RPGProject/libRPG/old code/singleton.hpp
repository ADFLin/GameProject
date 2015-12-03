#ifndef singleton_h__
#define singleton_h__

template< class T >
class Singleton
{
public:
	static T& instance()
	{
		static T _instance;
		return _instance;
	}

protected:
	Singleton(){}
	Singleton(Singleton& sing){}
};
#endif // singleton_h__