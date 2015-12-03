#ifndef StringTokenizer_h__
#define StringTokenizer_h__

#include <exception>

class TokenException : public std::exception
{
public:
	TokenException( char const* what ):std::exception( what ){}
};

class Tokenizer
{
public:
	Tokenizer( char const* str , char const* dropDelims , char const* stopDelims = "" )
		:mPtr( str )
		,mDropDelims( dropDelims )
		,mStopDelims( stopDelims )
	{

	}

	char const* next(){ return mPtr; }
	void        offset( int off ){ mPtr += off; }
	int  take( char* buf , char const* dropDelims , char const* stopDelims )
	{
		char cur = *( mPtr++);
		for ( ;; )
		{
			if ( cur == '\0' )
			{
				mCharDelim = cur;
				return 0;
			}

			if ( !strchr( mDropDelims , cur ) )
				break;

			cur = *( mPtr++);
		}

		if ( strchr( mStopDelims , cur ) )
		{
			*(buf++) = cur;
			mCharDelim = *buf;
			*(buf++) = '\0';
			return 1;
		}

		int num = 0;
		for( ; ; )
		{
			*(buf++) = cur;
			++num;

			cur = *mPtr;

			if ( cur == '\0' )
				break;
			if ( strchr( mDropDelims , cur ) )
				break;
			if ( strchr( mStopDelims , cur ) )
				break;

			++mPtr;
		}

		mCharDelim = *buf;
		*(buf++) = '\0';
		return num;
	}

	char mCharDelim;
	char const* mPtr;
	char const* mStopDelims;
	char const* mDropDelims;
};

class StringTokenizer
{
public:
	StringTokenizer(){ mNextToken = NULL; }
	StringTokenizer( char* str ){ begin( str ); }

	void begin( char* str )
	{
		mStr = str;
		mNextToken = NULL;
	}

	char* nextToken()
	{
		assert( mStr || mNextToken );
		char* out = strtok_s( mStr , " \r\t\n" , &mNextToken );
		mStr = NULL;
		return out;
	}

	char*   nextToken( char const* delim )
	{
		assert( mStr || mNextToken );
		char* out = strtok_s( mStr , delim , &mNextToken );
		mStr = NULL;
		return out;
	}

	double  nextFloat()
	{
		char* token = nextToken();
		if ( !token )
			throw TokenException("");
		return atof( token );
	}

	int     nextInt()
	{
		char* token = nextToken();
		if ( !token )
			throw TokenException("");
		return atoi( token );
	}

private:
	char const* tokenInternal( char const* delim , char& charEnd )
	{




	}

	char*  mStr;
	char*  mNextToken;
};
#endif // StringTokenizer_h__