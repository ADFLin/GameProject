#pragma once
#ifndef TransformPushScope_H_139DE160_38A9_4F36_B8C4_CCA027719B51
#define TransformPushScope_H_139DE160_38A9_4F36_B8C4_CCA027719B51

namespace Render
{

	template< typename StackType >
	struct TTransformStackTraits
	{
	};

	template<typename StackType>
	class TScopedTransformPush
	{
	public:
		using TransformType = typename TTransformStackTraits<StackType>::TransformType;

		FORCEINLINE TScopedTransformPush(StackType& stack)
			:mStack(stack)
		{
			mStack.push();
		}
		FORCEINLINE TScopedTransformPush(StackType& stack, TransformType const& transform, bool bApplyPrev = true)
			: mStack(stack)
		{
			mStack.pushTransform(transform, bApplyPrev);
													}

		FORCEINLINE ~TScopedTransformPush()
		{
			mStack.pop();
		}

		StackType& mStack;
	};

	template< typename StackType, typename ...Args>
	auto MakeScopedTransformPush(StackType& stack, Args&& ...args)
	{
		return TScopedTransformPush<StackType>(stack, std::forward<Args>(args)...);
	}

#define TRANSFORM_PUSH_SCOPE( STACK , ... ) auto ANONYMOUS_VARIABLE(Scope) = ::Render::MakeScopedTransformPush(STACK,##__VA_ARGS__);
}

#endif // TransformPushScope_H_139DE160_38A9_4F36_B8C4_CCA027719B51
