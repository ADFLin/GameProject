#pragma once
#ifndef ReferenceWarpper_H_9FBBBADE_7E19_40B6_B3F0_FC59290E5C42
#define ReferenceWarpper_H_9FBBBADE_7E19_40B6_B3F0_FC59290E5C42

template< typename T >
class TReferenceWarpper
{
public:
	TReferenceWarpper() = delete;
	TReferenceWarpper(T& rhs)
		:mPtr(&rhs){}

	operator T&      () { return mValue; }
	operator T const&() const { return mValue; }
private:
	T* mPtr;
};
#endif // ReferenceWarpper_H_9FBBBADE_7E19_40B6_B3F0_FC59290E5C42
