#pragma once
#ifndef Combination_H_18C8E5B0_09B0_41A2_A527_D2036F44F061
#define Combination_H_18C8E5B0_09B0_41A2_A527_D2036F44F061

template <typename Iter>
inline bool NextCombination(const Iter first, Iter k, const Iter last)
{
	if ((first == last) || (first == k) || (last == k))
		return false;
	Iter itr1 = first;
	Iter itr2 = last;
	++itr1;
	if (last == itr1)
		return false;
	itr1 = last;
	--itr1;
	itr1 = k;
	--itr2;
	while (first != itr1)
	{
		if (*--itr1 < *itr2)
		{
			Iter j = k;
			while (!(*itr1 < *j)) ++j;
			std::iter_swap(itr1, j);
			++itr1;
			++j;
			itr2 = k;
			std::rotate(itr1, j, last);
			while (last != j)
			{
				++j;
				++itr2;
			}
			std::rotate(k, itr2, last);
			return true;
		}
	}
	std::rotate(first, k, last);
	return false;
}

#endif // Combination_H_18C8E5B0_09B0_41A2_A527_D2036F44F061
