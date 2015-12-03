#ifndef SingalSystem_h__
#define SingalSystem_h__

enum DataType
{
	DT_UINT   ,
	DT_INT    ,
	DT_BOOL   ,
	DT_FLOAT  ,
	DT_DOUBLE ,
	DT_BYTE   ,
	DT_CHAR   ,
	DT_STR    ,
};

class SingalBase
{
protected:
	SingalBase( DataType dataType )
		:mDataType( dataType )
	{

	}
	unsigned mID;
	DataType mDataType;
};

template < class T >
class Singal : public SingalBase
{
public:
	void operator()( T const& val )
	{
		mData = val;
		TEvent event( this , mID , &mData );
		UG_SendEvent( event );
	}
public:
	T mData;
};


class SlotBase
{


public:
	DataType mDataType;
};

template < class T >
class Slot
{
	template< class Obj >
	fastdelegate::FastDelegate< void ( T ) > mCallFun;
};
class SingalSystem : public SingletonT< SingalSystem >
{



};

#endif // SingalSystem_h__