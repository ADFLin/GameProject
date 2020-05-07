
template< class RetType TEMP_ARG >
class DelegateN
{
public:
	DelegateN(){ mPtr = 0;  }
	template < class OBJ >
	DelegateN( OBJ* obj , RetType (OBJ::*func)( SIG_ARG ) ){  bind( obj , func );  }
	template < class OBJ >
	DelegateN( OBJ* obj )                                 {  bind( obj );  }
	DelegateN( RetType (*func)( SIG_ARG ) )                {  bind( func );  }

	template < class OBJ >
	void bind( OBJ* obj , RetType (OBJ::*func)( SIG_ARG ) ){  new ( mStorage ) MemFun< OBJ >( obj , func );  }
	template < class OBJ >
	void bind( OBJ* obj )                                 {  new ( mStorage ) OpFun< OBJ >( obj );  }
	void bind( RetType (*func)( SIG_ARG ) )                {  new ( mStorage ) BaseFun( func );  }

public:
	RetType  operator()( FUN_ARG ){ return  castFunPtr()->exec( PARAM_ARG ); }

private:
	struct FuncBase
	{
	public:
		virtual RetType exec( FUN_ARG ) = 0;
	};
	FuncBase*     castFunPtr(){ return reinterpret_cast< FuncBase* >( mStorage ); }

	template < class OBJ >
	struct MemFun : public FuncBase
	{
		typedef RetType (OBJ::*FuncType)( SIG_ARG );
		MemFun( OBJ* obj , FuncType func ):obj(obj),func(func){}
		virtual RetType exec( FUN_ARG ){  return ( obj->*func )( PARAM_ARG );  }
		OBJ*    obj;
		FuncType func;
	};

	struct BaseFun : public FuncBase
	{
		typedef RetType (*FuncType)( SIG_ARG );
		BaseFun(FuncType func ):func(func){}
		virtual RetType exec( FUN_ARG ){  return ( *func )( PARAM_ARG );  }
		FuncType func;
	};

	template < class OBJ >
	struct OpFun : public FuncBase
	{
		OpFun( OBJ* obj ):obj(obj){}
		virtual RetType exec( FUN_ARG ){  return ( *obj)( PARAM_ARG );  }
		OBJ* obj;
	};

	bool  empty(){  return mPtr != 0;  }
	void  clear(){  mPtr = NULL;  }
	union
	{
		void*  mPtr;
		char   mStorage[ sizeof( void* ) * 4 ];
	};
};


//template< class RetType TEMP_ARG >
//class Delegate< RetType ( SIG_ARG ) > : public DelegateN< RetType TEMP_ARG2 >
//{
//	typedef DelegateN< RetType , SIG_ARG > BaseClass;
//public:
//	DelegateN():BaseClass(){}
//	template < class OBJ >
//	DelegateN( OBJ* obj , RetType (OBJ::*fun)( SIG_ARG ) ):BaseClass( obj , fun ){}
//	template < class OBJ >
//	DelegateN( OBJ* obj ):BaseClass( obj ){}
//	DelegateN( RetType (*fun)( SIG_ARG ) ):BaseClass( fun ){}
//};

#undef DelegateN
#undef PARAM_ARG
#undef TEMP_ARG
#undef TEMP_ARG2
#undef SIG_ARG
#undef FUN_ARG
