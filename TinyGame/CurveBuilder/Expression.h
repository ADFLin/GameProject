#ifndef Expression_H
#define Expression_H

#include "ExpressionCompiler.h"
#include <typeindex>

namespace CB
{

	struct IEvalResource
	{
#if _DEBUG
		IEvalResource(std::type_index typeIndex)
			:typeIndex(typeIndex) {}
#endif
		virtual ~IEvalResource(){}
#if _DEBUG
		std::type_index typeIndex;
#endif
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
#if _DEBUG
			TTypedResource():IEvalResource(typeid(T)){}
#endif
			T instance;
		};
		template< typename T >
		T&  acquireEvalResource()
		{
			if (mEvalResource == nullptr)
			{
				mEvalResource = new TTypedResource<T>;
			}
			else
			{
#if _DEBUG
				CHECK(mEvalResource->typeIndex == typeid(T));
#endif
			}
			return static_cast<TTypedResource<T>*>(mEvalResource)->instance;
		}

		template< typename T >
		T&  getEvalResource()
		{
			CHECK(mEvalResource);
#if _DEBUG
			CHECK(mEvalResource->typeIndex == typeid(T));
#endif
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




