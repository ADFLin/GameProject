#ifndef Expression_H
#define Expression_H

#include "ExpressionCompiler.h"

namespace CB
{

	struct IEvalResource
	{
		virtual ~IEvalResource(){}
	};
	class Expression
	{
	public:
		Expression(std::string const& ExprStr = "");
		Expression(Expression const& rhs);
		Expression& operator=(const Expression& expr);

		~Expression();

		bool  isParsed() { return mIsParsed; }
		void  setExprString(std::string const& ExprStr)
		{
			mStrExpr = ExprStr;
			mIsParsed = false;
		}

		std::string const& getExprString() const { return mStrExpr; }

		template< typename T >
		class TTypedResource : public IEvalResource
		{
		public:
			T instance;
		};
		template< typename T >
		T&  GetOrCreateEvalResource()
		{
			if (mEvalResource == nullptr)
			{
				mEvalResource = new TTypedResource<T>;
			}
			return static_cast<TTypedResource<T>*>(mEvalResource)->instance;
		}

		template< typename T >
		T&  GetEvalResource()
		{
			CHECK(mEvalResource);
			return static_cast<TTypedResource<T>*>(mEvalResource)->instance;
		}

	private:
		bool           mIsParsed;
		std::string    mStrExpr;
		IEvalResource* mEvalResource = nullptr;

		friend class FunctionParser;
	};

}//namespace CB


#endif  //Expression_H




