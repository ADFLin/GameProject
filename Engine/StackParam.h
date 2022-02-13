#ifndef StackParam_h__
#define StackParam_h__

class StackParam
{
public:

	void   clearParams();
	void*  pushParamAny( char const* name , size_t size );

	struct  ParamVarInfo
	{
		char const* varName;
		int  dataOffset;
	};


private:

};
#endif // StackParam_h__
