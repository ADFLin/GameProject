#include "StageRegister.h"
#include <algorithm>
#include <queue>
#include <utility>

using namespace std;

struct ListNode 
{
	int val;
	ListNode *next;
	ListNode() : val(0), next(nullptr) {}
	ListNode(int x) : val(x), next(nullptr) {}
	ListNode(int x, ListNode *next) : val(x), next(next) {}
};


ListNode* MakeList(std::vector<int> values)
{
	if ( values.empty())
		return nullptr;

	ListNode* head = nullptr;
	ListNode** link = &head;
	for (auto v : values)
	{
		ListNode* node = new ListNode(v);
		*link = node;
		link = &node->next;
	}
	return head;
}

class Solution
{
public:
	int numDistinct(string s, string t)
	{
		vector< int > dp;
		dp.resize(t.length(), 0);
		dp[0] = 1;

		for (auto c : s)
		{
			for (int j = t.length() - 1; j >= 0; --j)
			{
				if (c == t[j])
				{
					dp[j + 1] += dp[j];
				}
			}
		}

		return dp.back();
	}

};

void LeetCodeTest()
{
	Solution solution;

	auto a = MakeList({ 1,2,3,4,5 });
	auto b = MakeList({ 1,2, 3,4 });
	auto c = MakeList({ 1 });


	vector<vector<int>> buildings = {{2, 9, 10}, {3, 7, 15}, {5, 12, 12}, {15, 20, 10}, {19, 24, 8}};

	vector<int> nums = { 3,30,34,5,9 };
	vector<string> wordList = { "leet","code" };
	vector<int> ratings = { 20000,20000 };
	auto result = solution.numDistinct("rabbbit", "rabbit");

	int kk = 1;
}

REGISTER_MISC_TEST_ENTRY("LeetCode" , LeetCodeTest);