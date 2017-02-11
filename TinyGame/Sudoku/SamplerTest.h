#pragma once

int probTemp[] =
{
	0,0,0, 0,0,0, 0,0,0,
	0,0,0, 0,0,0, 0,0,0,
	0,0,0, 0,0,0, 0,0,0,

	0,0,0, 0,0,0, 0,0,0,
	0,0,0, 0,0,0, 0,0,0,
	0,0,0, 0,0,0, 0,0,0,

	0,0,0, 0,0,0, 0,0,0,
	0,0,0, 0,0,0, 0,0,0,
	0,0,0, 0,0,0, 0,0,0,
};

int probNakedPair[] =
{
	4,0,0, 0,0,0, 9,3,8,
	0,3,2, 0,9,4, 1,0,0,
	0,9,5, 3,0,0, 2,4,0,

	3,7,0, 6,0,9, 0,0,4,
	5,2,9, 0,0,1, 6,7,3,
	6,0,4, 7,0,3, 0,9,0,

	9,5,7, 0,0,8, 3,0,0,
	0,0,3, 9,0,0, 4,0,0,
	2,4,0, 0,3,0, 7,0,9,
};

int prob1[] =
{

	2,0,0, 1,7,0, 0,0,6,
	0,0,0, 0,0,4, 0,0,0,
	7,0,4, 0,2,0, 3,0,0,

	0,0,0, 0,9,0, 5,0,0,
	0,7,1, 0,3,0, 6,2,0,
	0,0,2, 0,4,0, 0,0,0,

	0,0,8, 0,6,0, 7,0,3,
	0,0,0, 2,0,0, 0,0,0,
	6,0,0, 0,8,3, 0,0,4,

};

int prob2[] =
{
	0,0,4, 1,6,0, 7,0,0,
	7,0,0, 0,8,0, 0,1,0,
	0,1,0, 4,0,0, 0,2,3,

	1,0,0, 5,0,0, 0,0,2,
	0,5,8, 0,0,0, 1,3,0,
	2,0,0, 0,0,1, 0,0,6,

	3,8,0, 0,0,6, 0,4,0,
	0,9,0, 0,1,0, 0,0,7,
	0,0,2, 0,4,3, 8,0,0,
};

int prob3[] =
{
	//0,0,0, 7,5,0, 0,0,0,
	//0,3,0, 0,4,8, 0,2,0,
	//1,0,0, 0,0,0, 0,0,6,

	//0,4,0, 0,0,0, 0,0,8,
	//7,9,0, 0,0,0, 0,3,1,
	//2,0,0, 0,0,0, 0,7,0,

	//5,0,0, 0,0,0, 0,0,7,
	//0,8,0, 3,2,0, 0,4,0,
	//0,0,0, 0,6,9, 0,0,0,

	8,0,0, 7,5,0, 0,0,3,
	0,3,0, 0,4,8, 0,2,0,
	1,0,0, 0,0,0, 0,0,6,

	3,4,0, 0,7,0, 0,0,8,
	7,9,0, 4,8,0, 0,3,1,
	2,0,8, 0,0,0, 0,7,4,

	5,0,0, 8,1,4, 0,0,7,
	0,8,0, 3,2,7, 0,4,0,
	4,0,0, 5,6,9, 0,0,2,

};

int prob4[] =
{


	0,2,4,1,0,0,6,7,0,
	0,6,0,0,7,0,4,1,0,
	7,0,0,9,6,4,0,2,0,
	2,4,6,5,9,1,3,8,7,
	1,3,5,4,8,7,2,9,6,
	8,7,9,6,2,3,1,5,4,
	4,0,0,0,0,9,7,6,0,
	3,5,0,7,1,6,9,4,0,
	6,9,7,0,4,0,0,3,1,
};

int probSimpleColour[] =
{
	3,2,5, 0,0,4, 7,1,6,
	7,4,1, 2,6,3, 9,8,5,
	8,9,6, 0,1,0, 0,0,0,

	0,5,2, 6,0,0, 0,7,9,
	6,7,8, 4,0,9, 5,0,1,
	0,3,9, 0,0,0, 0,6,0,

	2,8,3, 0,4,0, 6,0,7,
	9,1,7, 3,0,6, 0,0,2,
	5,6,4, 0,0,0, 1,0,0,
};


int probBoxLine[] =
{
	0,1,6, 0,0,7, 8,0,3,
	0,9,0, 8,0,0, 0,0,0,
	8,7,0, 0,0,1, 0,6,0,

	0,4,8, 0,0,0, 3,0,0,
	6,5,0, 0,0,9, 0,8,2,
	0,3,9, 0,0,0, 6,5,0,

	0,6,0, 9,0,0, 0,2,0,
	0,8,0, 0,0,2, 9,3,6,
	9,2,4, 6,0,0, 5,1,0,
};

int probXWing[] =
{
	7,0,5, 8,0,1, 4,2,9,
	0,2,8, 9,4,0, 1,7,0,
	9,4,1, 7,0,2, 0,0,8,

	0,8,3, 1,0,7, 2,9,4,
	1,0,4, 2,9,3, 0,8,7,
	2,7,9, 4,8,0, 3,1,0,

	4,0,2, 6,7,8, 9,0,1,
	0,9,7, 0,1,4, 8,6,2,
	8,1,6, 0,2,9, 7,4,0,
};

