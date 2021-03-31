#include "TestRenderStageBase.h"

#include "RHI/DrawUtility.h"
#include "Renderer/MeshUtility.h"

#include "PerlinNoise.h"
#include "ProfileSystem.h"
#include "AsyncWork.h"
#include "RHI/RHIGraphics2D.h"
#include "RHI/D3D11Command.h"

#include "DrawEngine.h"

namespace MarchingCubes
{
	const int EdgeTable[256] =
	{
		0x0  , 0x109, 0x203, 0x30a, 0x406, 0x50f, 0x605, 0x70c,
		0x80c, 0x905, 0xa0f, 0xb06, 0xc0a, 0xd03, 0xe09, 0xf00,
		0x190, 0x99 , 0x393, 0x29a, 0x596, 0x49f, 0x795, 0x69c,
		0x99c, 0x895, 0xb9f, 0xa96, 0xd9a, 0xc93, 0xf99, 0xe90,
		0x230, 0x339, 0x33 , 0x13a, 0x636, 0x73f, 0x435, 0x53c,
		0xa3c, 0xb35, 0x83f, 0x936, 0xe3a, 0xf33, 0xc39, 0xd30,
		0x3a0, 0x2a9, 0x1a3, 0xaa , 0x7a6, 0x6af, 0x5a5, 0x4ac,
		0xbac, 0xaa5, 0x9af, 0x8a6, 0xfaa, 0xea3, 0xda9, 0xca0,
		0x460, 0x569, 0x663, 0x76a, 0x66 , 0x16f, 0x265, 0x36c,
		0xc6c, 0xd65, 0xe6f, 0xf66, 0x86a, 0x963, 0xa69, 0xb60,
		0x5f0, 0x4f9, 0x7f3, 0x6fa, 0x1f6, 0xff , 0x3f5, 0x2fc,
		0xdfc, 0xcf5, 0xfff, 0xef6, 0x9fa, 0x8f3, 0xbf9, 0xaf0,
		0x650, 0x759, 0x453, 0x55a, 0x256, 0x35f, 0x55 , 0x15c,
		0xe5c, 0xf55, 0xc5f, 0xd56, 0xa5a, 0xb53, 0x859, 0x950,
		0x7c0, 0x6c9, 0x5c3, 0x4ca, 0x3c6, 0x2cf, 0x1c5, 0xcc ,
		0xfcc, 0xec5, 0xdcf, 0xcc6, 0xbca, 0xac3, 0x9c9, 0x8c0,
		0x8c0, 0x9c9, 0xac3, 0xbca, 0xcc6, 0xdcf, 0xec5, 0xfcc,
		0xcc , 0x1c5, 0x2cf, 0x3c6, 0x4ca, 0x5c3, 0x6c9, 0x7c0,
		0x950, 0x859, 0xb53, 0xa5a, 0xd56, 0xc5f, 0xf55, 0xe5c,
		0x15c, 0x55 , 0x35f, 0x256, 0x55a, 0x453, 0x759, 0x650,
		0xaf0, 0xbf9, 0x8f3, 0x9fa, 0xef6, 0xfff, 0xcf5, 0xdfc,
		0x2fc, 0x3f5, 0xff , 0x1f6, 0x6fa, 0x7f3, 0x4f9, 0x5f0,
		0xb60, 0xa69, 0x963, 0x86a, 0xf66, 0xe6f, 0xd65, 0xc6c,
		0x36c, 0x265, 0x16f, 0x66 , 0x76a, 0x663, 0x569, 0x460,
		0xca0, 0xda9, 0xea3, 0xfaa, 0x8a6, 0x9af, 0xaa5, 0xbac,
		0x4ac, 0x5a5, 0x6af, 0x7a6, 0xaa , 0x1a3, 0x2a9, 0x3a0,
		0xd30, 0xc39, 0xf33, 0xe3a, 0x936, 0x83f, 0xb35, 0xa3c,
		0x53c, 0x435, 0x73f, 0x636, 0x13a, 0x33 , 0x339, 0x230,
		0xe90, 0xf99, 0xc93, 0xd9a, 0xa96, 0xb9f, 0x895, 0x99c,
		0x69c, 0x795, 0x49f, 0x596, 0x29a, 0x393, 0x99 , 0x190,
		0xf00, 0xe09, 0xd03, 0xc0a, 0xb06, 0xa0f, 0x905, 0x80c,
		0x70c, 0x605, 0x50f, 0x406, 0x30a, 0x203, 0x109, 0x0
	};

	const int TriTable[256][16] =
	{
		{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{0, 8, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{0, 1, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{1, 8, 3, 9, 8, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{1, 2, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{0, 8, 3, 1, 2, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{9, 2, 10, 0, 2, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{2, 8, 3, 2, 10, 8, 10, 9, 8, -1, -1, -1, -1, -1, -1, -1},
		{3, 11, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{0, 11, 2, 8, 11, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{1, 9, 0, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{1, 11, 2, 1, 9, 11, 9, 8, 11, -1, -1, -1, -1, -1, -1, -1},
		{3, 10, 1, 11, 10, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{0, 10, 1, 0, 8, 10, 8, 11, 10, -1, -1, -1, -1, -1, -1, -1},
		{3, 9, 0, 3, 11, 9, 11, 10, 9, -1, -1, -1, -1, -1, -1, -1},
		{9, 8, 10, 10, 8, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{4, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{4, 3, 0, 7, 3, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{0, 1, 9, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{4, 1, 9, 4, 7, 1, 7, 3, 1, -1, -1, -1, -1, -1, -1, -1},
		{1, 2, 10, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{3, 4, 7, 3, 0, 4, 1, 2, 10, -1, -1, -1, -1, -1, -1, -1},
		{9, 2, 10, 9, 0, 2, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1},
		{2, 10, 9, 2, 9, 7, 2, 7, 3, 7, 9, 4, -1, -1, -1, -1},
		{8, 4, 7, 3, 11, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{11, 4, 7, 11, 2, 4, 2, 0, 4, -1, -1, -1, -1, -1, -1, -1},
		{9, 0, 1, 8, 4, 7, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1},
		{4, 7, 11, 9, 4, 11, 9, 11, 2, 9, 2, 1, -1, -1, -1, -1},
		{3, 10, 1, 3, 11, 10, 7, 8, 4, -1, -1, -1, -1, -1, -1, -1},
		{1, 11, 10, 1, 4, 11, 1, 0, 4, 7, 11, 4, -1, -1, -1, -1},
		{4, 7, 8, 9, 0, 11, 9, 11, 10, 11, 0, 3, -1, -1, -1, -1},
		{4, 7, 11, 4, 11, 9, 9, 11, 10, -1, -1, -1, -1, -1, -1, -1},
		{9, 5, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{9, 5, 4, 0, 8, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{0, 5, 4, 1, 5, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{8, 5, 4, 8, 3, 5, 3, 1, 5, -1, -1, -1, -1, -1, -1, -1},
		{1, 2, 10, 9, 5, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{3, 0, 8, 1, 2, 10, 4, 9, 5, -1, -1, -1, -1, -1, -1, -1},
		{5, 2, 10, 5, 4, 2, 4, 0, 2, -1, -1, -1, -1, -1, -1, -1},
		{2, 10, 5, 3, 2, 5, 3, 5, 4, 3, 4, 8, -1, -1, -1, -1},
		{9, 5, 4, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{0, 11, 2, 0, 8, 11, 4, 9, 5, -1, -1, -1, -1, -1, -1, -1},
		{0, 5, 4, 0, 1, 5, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1},
		{2, 1, 5, 2, 5, 8, 2, 8, 11, 4, 8, 5, -1, -1, -1, -1},
		{10, 3, 11, 10, 1, 3, 9, 5, 4, -1, -1, -1, -1, -1, -1, -1},
		{4, 9, 5, 0, 8, 1, 8, 10, 1, 8, 11, 10, -1, -1, -1, -1},
		{5, 4, 0, 5, 0, 11, 5, 11, 10, 11, 0, 3, -1, -1, -1, -1},
		{5, 4, 8, 5, 8, 10, 10, 8, 11, -1, -1, -1, -1, -1, -1, -1},
		{9, 7, 8, 5, 7, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{9, 3, 0, 9, 5, 3, 5, 7, 3, -1, -1, -1, -1, -1, -1, -1},
		{0, 7, 8, 0, 1, 7, 1, 5, 7, -1, -1, -1, -1, -1, -1, -1},
		{1, 5, 3, 3, 5, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{9, 7, 8, 9, 5, 7, 10, 1, 2, -1, -1, -1, -1, -1, -1, -1},
		{10, 1, 2, 9, 5, 0, 5, 3, 0, 5, 7, 3, -1, -1, -1, -1},
		{8, 0, 2, 8, 2, 5, 8, 5, 7, 10, 5, 2, -1, -1, -1, -1},
		{2, 10, 5, 2, 5, 3, 3, 5, 7, -1, -1, -1, -1, -1, -1, -1},
		{7, 9, 5, 7, 8, 9, 3, 11, 2, -1, -1, -1, -1, -1, -1, -1},
		{9, 5, 7, 9, 7, 2, 9, 2, 0, 2, 7, 11, -1, -1, -1, -1},
		{2, 3, 11, 0, 1, 8, 1, 7, 8, 1, 5, 7, -1, -1, -1, -1},
		{11, 2, 1, 11, 1, 7, 7, 1, 5, -1, -1, -1, -1, -1, -1, -1},
		{9, 5, 8, 8, 5, 7, 10, 1, 3, 10, 3, 11, -1, -1, -1, -1},
		{5, 7, 0, 5, 0, 9, 7, 11, 0, 1, 0, 10, 11, 10, 0, -1},
		{11, 10, 0, 11, 0, 3, 10, 5, 0, 8, 0, 7, 5, 7, 0, -1},
		{11, 10, 5, 7, 11, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{10, 6, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{0, 8, 3, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{9, 0, 1, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{1, 8, 3, 1, 9, 8, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1},
		{1, 6, 5, 2, 6, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{1, 6, 5, 1, 2, 6, 3, 0, 8, -1, -1, -1, -1, -1, -1, -1},
		{9, 6, 5, 9, 0, 6, 0, 2, 6, -1, -1, -1, -1, -1, -1, -1},
		{5, 9, 8, 5, 8, 2, 5, 2, 6, 3, 2, 8, -1, -1, -1, -1},
		{2, 3, 11, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{11, 0, 8, 11, 2, 0, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1},
		{0, 1, 9, 2, 3, 11, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1},
		{5, 10, 6, 1, 9, 2, 9, 11, 2, 9, 8, 11, -1, -1, -1, -1},
		{6, 3, 11, 6, 5, 3, 5, 1, 3, -1, -1, -1, -1, -1, -1, -1},
		{0, 8, 11, 0, 11, 5, 0, 5, 1, 5, 11, 6, -1, -1, -1, -1},
		{3, 11, 6, 0, 3, 6, 0, 6, 5, 0, 5, 9, -1, -1, -1, -1},
		{6, 5, 9, 6, 9, 11, 11, 9, 8, -1, -1, -1, -1, -1, -1, -1},
		{5, 10, 6, 4, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{4, 3, 0, 4, 7, 3, 6, 5, 10, -1, -1, -1, -1, -1, -1, -1},
		{1, 9, 0, 5, 10, 6, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1},
		{10, 6, 5, 1, 9, 7, 1, 7, 3, 7, 9, 4, -1, -1, -1, -1},
		{6, 1, 2, 6, 5, 1, 4, 7, 8, -1, -1, -1, -1, -1, -1, -1},
		{1, 2, 5, 5, 2, 6, 3, 0, 4, 3, 4, 7, -1, -1, -1, -1},
		{8, 4, 7, 9, 0, 5, 0, 6, 5, 0, 2, 6, -1, -1, -1, -1},
		{7, 3, 9, 7, 9, 4, 3, 2, 9, 5, 9, 6, 2, 6, 9, -1},
		{3, 11, 2, 7, 8, 4, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1},
		{5, 10, 6, 4, 7, 2, 4, 2, 0, 2, 7, 11, -1, -1, -1, -1},
		{0, 1, 9, 4, 7, 8, 2, 3, 11, 5, 10, 6, -1, -1, -1, -1},
		{9, 2, 1, 9, 11, 2, 9, 4, 11, 7, 11, 4, 5, 10, 6, -1},
		{8, 4, 7, 3, 11, 5, 3, 5, 1, 5, 11, 6, -1, -1, -1, -1},
		{5, 1, 11, 5, 11, 6, 1, 0, 11, 7, 11, 4, 0, 4, 11, -1},
		{0, 5, 9, 0, 6, 5, 0, 3, 6, 11, 6, 3, 8, 4, 7, -1},
		{6, 5, 9, 6, 9, 11, 4, 7, 9, 7, 11, 9, -1, -1, -1, -1},
		{10, 4, 9, 6, 4, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{4, 10, 6, 4, 9, 10, 0, 8, 3, -1, -1, -1, -1, -1, -1, -1},
		{10, 0, 1, 10, 6, 0, 6, 4, 0, -1, -1, -1, -1, -1, -1, -1},
		{8, 3, 1, 8, 1, 6, 8, 6, 4, 6, 1, 10, -1, -1, -1, -1},
		{1, 4, 9, 1, 2, 4, 2, 6, 4, -1, -1, -1, -1, -1, -1, -1},
		{3, 0, 8, 1, 2, 9, 2, 4, 9, 2, 6, 4, -1, -1, -1, -1},
		{0, 2, 4, 4, 2, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{8, 3, 2, 8, 2, 4, 4, 2, 6, -1, -1, -1, -1, -1, -1, -1},
		{10, 4, 9, 10, 6, 4, 11, 2, 3, -1, -1, -1, -1, -1, -1, -1},
		{0, 8, 2, 2, 8, 11, 4, 9, 10, 4, 10, 6, -1, -1, -1, -1},
		{3, 11, 2, 0, 1, 6, 0, 6, 4, 6, 1, 10, -1, -1, -1, -1},
		{6, 4, 1, 6, 1, 10, 4, 8, 1, 2, 1, 11, 8, 11, 1, -1},
		{9, 6, 4, 9, 3, 6, 9, 1, 3, 11, 6, 3, -1, -1, -1, -1},
		{8, 11, 1, 8, 1, 0, 11, 6, 1, 9, 1, 4, 6, 4, 1, -1},
		{3, 11, 6, 3, 6, 0, 0, 6, 4, -1, -1, -1, -1, -1, -1, -1},
		{6, 4, 8, 11, 6, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{7, 10, 6, 7, 8, 10, 8, 9, 10, -1, -1, -1, -1, -1, -1, -1},
		{0, 7, 3, 0, 10, 7, 0, 9, 10, 6, 7, 10, -1, -1, -1, -1},
		{10, 6, 7, 1, 10, 7, 1, 7, 8, 1, 8, 0, -1, -1, -1, -1},
		{10, 6, 7, 10, 7, 1, 1, 7, 3, -1, -1, -1, -1, -1, -1, -1},
		{1, 2, 6, 1, 6, 8, 1, 8, 9, 8, 6, 7, -1, -1, -1, -1},
		{2, 6, 9, 2, 9, 1, 6, 7, 9, 0, 9, 3, 7, 3, 9, -1},
		{7, 8, 0, 7, 0, 6, 6, 0, 2, -1, -1, -1, -1, -1, -1, -1},
		{7, 3, 2, 6, 7, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{2, 3, 11, 10, 6, 8, 10, 8, 9, 8, 6, 7, -1, -1, -1, -1},
		{2, 0, 7, 2, 7, 11, 0, 9, 7, 6, 7, 10, 9, 10, 7, -1},
		{1, 8, 0, 1, 7, 8, 1, 10, 7, 6, 7, 10, 2, 3, 11, -1},
		{11, 2, 1, 11, 1, 7, 10, 6, 1, 6, 7, 1, -1, -1, -1, -1},
		{8, 9, 6, 8, 6, 7, 9, 1, 6, 11, 6, 3, 1, 3, 6, -1},
		{0, 9, 1, 11, 6, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{7, 8, 0, 7, 0, 6, 3, 11, 0, 11, 6, 0, -1, -1, -1, -1},
		{7, 11, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{7, 6, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{3, 0, 8, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{0, 1, 9, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{8, 1, 9, 8, 3, 1, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1},
		{10, 1, 2, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{1, 2, 10, 3, 0, 8, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1},
		{2, 9, 0, 2, 10, 9, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1},
		{6, 11, 7, 2, 10, 3, 10, 8, 3, 10, 9, 8, -1, -1, -1, -1},
		{7, 2, 3, 6, 2, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{7, 0, 8, 7, 6, 0, 6, 2, 0, -1, -1, -1, -1, -1, -1, -1},
		{2, 7, 6, 2, 3, 7, 0, 1, 9, -1, -1, -1, -1, -1, -1, -1},
		{1, 6, 2, 1, 8, 6, 1, 9, 8, 8, 7, 6, -1, -1, -1, -1},
		{10, 7, 6, 10, 1, 7, 1, 3, 7, -1, -1, -1, -1, -1, -1, -1},
		{10, 7, 6, 1, 7, 10, 1, 8, 7, 1, 0, 8, -1, -1, -1, -1},
		{0, 3, 7, 0, 7, 10, 0, 10, 9, 6, 10, 7, -1, -1, -1, -1},
		{7, 6, 10, 7, 10, 8, 8, 10, 9, -1, -1, -1, -1, -1, -1, -1},
		{6, 8, 4, 11, 8, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{3, 6, 11, 3, 0, 6, 0, 4, 6, -1, -1, -1, -1, -1, -1, -1},
		{8, 6, 11, 8, 4, 6, 9, 0, 1, -1, -1, -1, -1, -1, -1, -1},
		{9, 4, 6, 9, 6, 3, 9, 3, 1, 11, 3, 6, -1, -1, -1, -1},
		{6, 8, 4, 6, 11, 8, 2, 10, 1, -1, -1, -1, -1, -1, -1, -1},
		{1, 2, 10, 3, 0, 11, 0, 6, 11, 0, 4, 6, -1, -1, -1, -1},
		{4, 11, 8, 4, 6, 11, 0, 2, 9, 2, 10, 9, -1, -1, -1, -1},
		{10, 9, 3, 10, 3, 2, 9, 4, 3, 11, 3, 6, 4, 6, 3, -1},
		{8, 2, 3, 8, 4, 2, 4, 6, 2, -1, -1, -1, -1, -1, -1, -1},
		{0, 4, 2, 4, 6, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{1, 9, 0, 2, 3, 4, 2, 4, 6, 4, 3, 8, -1, -1, -1, -1},
		{1, 9, 4, 1, 4, 2, 2, 4, 6, -1, -1, -1, -1, -1, -1, -1},
		{8, 1, 3, 8, 6, 1, 8, 4, 6, 6, 10, 1, -1, -1, -1, -1},
		{10, 1, 0, 10, 0, 6, 6, 0, 4, -1, -1, -1, -1, -1, -1, -1},
		{4, 6, 3, 4, 3, 8, 6, 10, 3, 0, 3, 9, 10, 9, 3, -1},
		{10, 9, 4, 6, 10, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{4, 9, 5, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{0, 8, 3, 4, 9, 5, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1},
		{5, 0, 1, 5, 4, 0, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1},
		{11, 7, 6, 8, 3, 4, 3, 5, 4, 3, 1, 5, -1, -1, -1, -1},
		{9, 5, 4, 10, 1, 2, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1},
		{6, 11, 7, 1, 2, 10, 0, 8, 3, 4, 9, 5, -1, -1, -1, -1},
		{7, 6, 11, 5, 4, 10, 4, 2, 10, 4, 0, 2, -1, -1, -1, -1},
		{3, 4, 8, 3, 5, 4, 3, 2, 5, 10, 5, 2, 11, 7, 6, -1},
		{7, 2, 3, 7, 6, 2, 5, 4, 9, -1, -1, -1, -1, -1, -1, -1},
		{9, 5, 4, 0, 8, 6, 0, 6, 2, 6, 8, 7, -1, -1, -1, -1},
		{3, 6, 2, 3, 7, 6, 1, 5, 0, 5, 4, 0, -1, -1, -1, -1},
		{6, 2, 8, 6, 8, 7, 2, 1, 8, 4, 8, 5, 1, 5, 8, -1},
		{9, 5, 4, 10, 1, 6, 1, 7, 6, 1, 3, 7, -1, -1, -1, -1},
		{1, 6, 10, 1, 7, 6, 1, 0, 7, 8, 7, 0, 9, 5, 4, -1},
		{4, 0, 10, 4, 10, 5, 0, 3, 10, 6, 10, 7, 3, 7, 10, -1},
		{7, 6, 10, 7, 10, 8, 5, 4, 10, 4, 8, 10, -1, -1, -1, -1},
		{6, 9, 5, 6, 11, 9, 11, 8, 9, -1, -1, -1, -1, -1, -1, -1},
		{3, 6, 11, 0, 6, 3, 0, 5, 6, 0, 9, 5, -1, -1, -1, -1},
		{0, 11, 8, 0, 5, 11, 0, 1, 5, 5, 6, 11, -1, -1, -1, -1},
		{6, 11, 3, 6, 3, 5, 5, 3, 1, -1, -1, -1, -1, -1, -1, -1},
		{1, 2, 10, 9, 5, 11, 9, 11, 8, 11, 5, 6, -1, -1, -1, -1},
		{0, 11, 3, 0, 6, 11, 0, 9, 6, 5, 6, 9, 1, 2, 10, -1},
		{11, 8, 5, 11, 5, 6, 8, 0, 5, 10, 5, 2, 0, 2, 5, -1},
		{6, 11, 3, 6, 3, 5, 2, 10, 3, 10, 5, 3, -1, -1, -1, -1},
		{5, 8, 9, 5, 2, 8, 5, 6, 2, 3, 8, 2, -1, -1, -1, -1},
		{9, 5, 6, 9, 6, 0, 0, 6, 2, -1, -1, -1, -1, -1, -1, -1},
		{1, 5, 8, 1, 8, 0, 5, 6, 8, 3, 8, 2, 6, 2, 8, -1},
		{1, 5, 6, 2, 1, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{1, 3, 6, 1, 6, 10, 3, 8, 6, 5, 6, 9, 8, 9, 6, -1},
		{10, 1, 0, 10, 0, 6, 9, 5, 0, 5, 6, 0, -1, -1, -1, -1},
		{0, 3, 8, 5, 6, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{10, 5, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{11, 5, 10, 7, 5, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{11, 5, 10, 11, 7, 5, 8, 3, 0, -1, -1, -1, -1, -1, -1, -1},
		{5, 11, 7, 5, 10, 11, 1, 9, 0, -1, -1, -1, -1, -1, -1, -1},
		{10, 7, 5, 10, 11, 7, 9, 8, 1, 8, 3, 1, -1, -1, -1, -1},
		{11, 1, 2, 11, 7, 1, 7, 5, 1, -1, -1, -1, -1, -1, -1, -1},
		{0, 8, 3, 1, 2, 7, 1, 7, 5, 7, 2, 11, -1, -1, -1, -1},
		{9, 7, 5, 9, 2, 7, 9, 0, 2, 2, 11, 7, -1, -1, -1, -1},
		{7, 5, 2, 7, 2, 11, 5, 9, 2, 3, 2, 8, 9, 8, 2, -1},
		{2, 5, 10, 2, 3, 5, 3, 7, 5, -1, -1, -1, -1, -1, -1, -1},
		{8, 2, 0, 8, 5, 2, 8, 7, 5, 10, 2, 5, -1, -1, -1, -1},
		{9, 0, 1, 5, 10, 3, 5, 3, 7, 3, 10, 2, -1, -1, -1, -1},
		{9, 8, 2, 9, 2, 1, 8, 7, 2, 10, 2, 5, 7, 5, 2, -1},
		{1, 3, 5, 3, 7, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{0, 8, 7, 0, 7, 1, 1, 7, 5, -1, -1, -1, -1, -1, -1, -1},
		{9, 0, 3, 9, 3, 5, 5, 3, 7, -1, -1, -1, -1, -1, -1, -1},
		{9, 8, 7, 5, 9, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{5, 8, 4, 5, 10, 8, 10, 11, 8, -1, -1, -1, -1, -1, -1, -1},
		{5, 0, 4, 5, 11, 0, 5, 10, 11, 11, 3, 0, -1, -1, -1, -1},
		{0, 1, 9, 8, 4, 10, 8, 10, 11, 10, 4, 5, -1, -1, -1, -1},
		{10, 11, 4, 10, 4, 5, 11, 3, 4, 9, 4, 1, 3, 1, 4, -1},
		{2, 5, 1, 2, 8, 5, 2, 11, 8, 4, 5, 8, -1, -1, -1, -1},
		{0, 4, 11, 0, 11, 3, 4, 5, 11, 2, 11, 1, 5, 1, 11, -1},
		{0, 2, 5, 0, 5, 9, 2, 11, 5, 4, 5, 8, 11, 8, 5, -1},
		{9, 4, 5, 2, 11, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{2, 5, 10, 3, 5, 2, 3, 4, 5, 3, 8, 4, -1, -1, -1, -1},
		{5, 10, 2, 5, 2, 4, 4, 2, 0, -1, -1, -1, -1, -1, -1, -1},
		{3, 10, 2, 3, 5, 10, 3, 8, 5, 4, 5, 8, 0, 1, 9, -1},
		{5, 10, 2, 5, 2, 4, 1, 9, 2, 9, 4, 2, -1, -1, -1, -1},
		{8, 4, 5, 8, 5, 3, 3, 5, 1, -1, -1, -1, -1, -1, -1, -1},
		{0, 4, 5, 1, 0, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{8, 4, 5, 8, 5, 3, 9, 0, 5, 0, 3, 5, -1, -1, -1, -1},
		{9, 4, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{4, 11, 7, 4, 9, 11, 9, 10, 11, -1, -1, -1, -1, -1, -1, -1},
		{0, 8, 3, 4, 9, 7, 9, 11, 7, 9, 10, 11, -1, -1, -1, -1},
		{1, 10, 11, 1, 11, 4, 1, 4, 0, 7, 4, 11, -1, -1, -1, -1},
		{3, 1, 4, 3, 4, 8, 1, 10, 4, 7, 4, 11, 10, 11, 4, -1},
		{4, 11, 7, 9, 11, 4, 9, 2, 11, 9, 1, 2, -1, -1, -1, -1},
		{9, 7, 4, 9, 11, 7, 9, 1, 11, 2, 11, 1, 0, 8, 3, -1},
		{11, 7, 4, 11, 4, 2, 2, 4, 0, -1, -1, -1, -1, -1, -1, -1},
		{11, 7, 4, 11, 4, 2, 8, 3, 4, 3, 2, 4, -1, -1, -1, -1},
		{2, 9, 10, 2, 7, 9, 2, 3, 7, 7, 4, 9, -1, -1, -1, -1},
		{9, 10, 7, 9, 7, 4, 10, 2, 7, 8, 7, 0, 2, 0, 7, -1},
		{3, 7, 10, 3, 10, 2, 7, 4, 10, 1, 10, 0, 4, 0, 10, -1},
		{1, 10, 2, 8, 7, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{4, 9, 1, 4, 1, 7, 7, 1, 3, -1, -1, -1, -1, -1, -1, -1},
		{4, 9, 1, 4, 1, 7, 0, 8, 1, 8, 7, 1, -1, -1, -1, -1},
		{4, 0, 3, 7, 4, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{4, 8, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{9, 10, 8, 10, 11, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{3, 0, 9, 3, 9, 11, 11, 9, 10, -1, -1, -1, -1, -1, -1, -1},
		{0, 1, 10, 0, 10, 8, 8, 10, 11, -1, -1, -1, -1, -1, -1, -1},
		{3, 1, 10, 11, 3, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{1, 2, 11, 1, 11, 9, 9, 11, 8, -1, -1, -1, -1, -1, -1, -1},
		{3, 0, 9, 3, 9, 11, 1, 2, 9, 2, 11, 9, -1, -1, -1, -1},
		{0, 2, 11, 8, 0, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{3, 2, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{2, 3, 8, 2, 8, 10, 10, 8, 9, -1, -1, -1, -1, -1, -1, -1},
		{9, 10, 2, 0, 9, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{2, 3, 8, 2, 8, 10, 0, 1, 8, 1, 10, 8, -1, -1, -1, -1},
		{1, 10, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{1, 3, 8, 9, 1, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{0, 9, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{0, 3, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}
	};

	//
	//    v7_________v6
	//     /|       /|
	//  v4/_|_e4_v5/ |
	//    | |______|_|                z  y
	//  e8| /v3    | / v2             | /
	//    |/_______|/                 |/___ x
	//   v0   e0  v1
	static const int EdgeToVertexMap[12][2] =
	{
		{0 , 1},{1 , 2},{2 , 3},{3 , 0},
		{4 , 5},{5 , 6},{6 , 7},{7 , 4},
		{0 , 4},{1 , 5},{2 , 6},{3 , 7},
	};
}

namespace Render
{
	struct VoxelChunkConfig
	{
		struct GridCoord
		{
			int     index;
			Vector3 pos;
		};
		Vector3    voxelSize;
		IntVector3 voxelDim;
		IntVector3 dataDim;

		std::vector< GridCoord > coords;

		int toDataIndex(IntVector3 const& voxelPos, int indexVertex) const
		{
			assert(voxelPos.x < voxelDim.x &&  voxelPos.y < voxelDim.y &&  voxelPos.z < voxelDim.z);
			IntVector3 vertexPos = voxelPos + GetVertexOffset(indexVertex);
			return vertexPos.x + dataDim.x * (vertexPos.y + dataDim.y * vertexPos.z);
		}
		int toDataIndex(IntVector3 const& dataCoord) const
		{
			assert(dataCoord.x < dataDim.x &&  dataCoord.y < dataDim.y &&  dataCoord.z < dataDim.z);
			return dataCoord.x + dataDim.x * (dataCoord.y + dataDim.y * dataCoord.z);
		}

		static IntVector3 const& GetVertexOffset(int indexVertex)
		{
			static IntVector3 offsetMap[] =
			{
				IntVector3(0,0,0),IntVector3(1,0,0),IntVector3(1,1,0),IntVector3(0,1,0),
				IntVector3(0,0,1),IntVector3(1,0,1),IntVector3(1,1,1),IntVector3(0,1,1),
			};

			return offsetMap[indexVertex];
		}

		void initialize(IntVector3 const& dim, Vector3 const& inVoxelSize)
		{
			assert(dim.x > 1 && dim.y > 1 && dim.z > 1);
			dataDim = dim;
			voxelDim = dataDim - IntVector3(1, 1, 1);
			voxelSize = inVoxelSize;
			int numData = dim.x * dim.y * dim.z;

			coords.resize(numData);

			for (int k = 0; k < dim.z; ++k)
			{
				for (int j = 0; j < dim.y; ++j)
				{
					for (int i = 0; i < dim.x; ++i)
					{
						int index = toDataIndex(IntVector3(i, j, k));
						coords[index].index = index;
						coords[index].pos = voxelSize.mul(Vector3(i, j, k));
					}
				}
			}
		}
	};


	struct VoxelChunkData
	{
		void initialize(VoxelChunkConfig const& inConfig)
		{
			config = &inConfig;
			data.resize(inConfig.coords.size());
		}

		float getInterpDataX(int indexData , float alpha) const
		{
			int nextIndex = indexData + 1;
			if (nextIndex >= data.size())
				nextIndex = indexData;
			return Math::Lerp(data[indexData], data[nextIndex], alpha);
		}
		float getInterpDataY(int indexData, float alpha) const
		{
			int nextIndex = indexData + config->dataDim.x;
			if (nextIndex >= data.size())
				nextIndex = indexData;
			return Math::Lerp(data[indexData], data[nextIndex], alpha);
		}

		float getInterpDataZ(int indexData, float alpha) const
		{
			int nextIndex = indexData + config->dataDim.x * config->dataDim.y;
			if (nextIndex >= data.size())
				nextIndex = indexData;
			return Math::Lerp(data[indexData], data[nextIndex], alpha);
		}

		std::vector< float > data;
		Vector3 offset;
		VoxelChunkConfig const* config;
	};

	struct VoxelMeshData
	{
		struct Vertex
		{
			Vector3 pos;
			Vector3 normal;
		};
		std::vector< Vertex > vertices;
		std::vector< int32 >  indices;
	};

	struct VoxelMeshBuilder
	{
	public:

		void initialize()
		{
			mWorkPool.init(8);
		}


		typedef VoxelChunkConfig::GridCoord GridCoord;
		struct GirdData
		{
			GridCoord const* coord;
			float  value;
		};

		struct TriVertexInfo
		{
			GirdData v[2];
		};

		std::unordered_map< uint64, int > mEdgeToVertexMap;

		void generateMesh(float isolevel, VoxelChunkData const& chunkData , VoxelMeshData& outMeshData)
		{
			//TimeScope scope("generateMesh");

			outMeshData.vertices.clear();
			outMeshData.indices.clear();
			mEdgeToVertexMap.clear();

#if 0
			std::vector< TriVertexInfo > triVertices;
			generateVoxelTriangle(isolevel, IntVector3(0, 0, 0), chunkData.config->cubeDim, triVertices);
			mergeMeshData(isolevel, outMeshData, triVertices);
#else
			int numWorker = mWorkPool.getAllThreadNum();
			int voxelSubZLen = (chunkData.config->voxelDim.z + numWorker - 1 ) / numWorker;

			int completeCount = 0;
			for (int i = 0; i < numWorker; ++i)
			{
				MyWork* work = new MyWork;
				work->generator = this;
				work->chunkData = &chunkData;
				work->meshData = &outMeshData;
				work->isolevel = isolevel;
				work->startVoxelPos = IntVector3(0,0,voxelSubZLen * i);
				work->genVoxelDim = chunkData.config->voxelDim;
				work->genVoxelDim.z = Math::Min(voxelSubZLen, chunkData.config->voxelDim.z - work->startVoxelPos.z);
				work->completeCount = &completeCount;
				mWorkPool.addWork(work);
			}
			{
				TimeScope scope("waitAllWorkComplete");
#if 0
				mWorkPool.waitAllWorkComplete();
#else

				while (SystemPlatform::AtomicRead(&completeCount) != numWorker)
				{
					SystemPlatform::Sleep(0);
				}
#endif
			}
#endif

			
		}

		class MyWork : public IQueuedWork
		{
		public:
			virtual void executeWork() override
			{
				{
					//TimeScope scope("generateCubeTriangle");
					generator->generateVoxelTriangle(isolevel, *chunkData, startVoxelPos, genVoxelDim, triVertices);
				}
				
				{
					//TimeScope scope("mergeMeshData");
					Mutex::Locker locker(generator->meshLock);
					generator->mergeMeshData(isolevel, *meshData, triVertices);
				}

				SystemPlatform::AtomIncrement(completeCount);
			}
			virtual void release() override
			{
				delete this;
			}

			int index;
			VoxelMeshBuilder* generator;
			VoxelChunkData const* chunkData;
			VoxelMeshData* meshData;
			std::vector< TriVertexInfo > triVertices;
			float isolevel;
			IntVector3 startVoxelPos;
			IntVector3 genVoxelDim;
			volatile int32* completeCount;

		};

		QueueThreadPool mWorkPool;
		Mutex meshLock;

		void mergeMeshData(float isolevel, VoxelMeshData& meshData, std::vector< TriVertexInfo > const& triVertices)
		{
			assert(triVertices.size() % 3 == 0);

			meshData.indices.reserve(meshData.indices.size() + triVertices.size());
			meshData.vertices.reserve(meshData.vertices.size() + triVertices.size());
			for (auto const& info : triVertices)
			{
#if 1
				int idx1 = info.v[0].coord->index;
				int idx2 = info.v[1].coord->index;
				if (idx1 > idx2)
				{
					std::swap(idx1, idx2);
				}
				uint64 vKey = (uint64(idx1) << 32) | uint64(idx2);

				auto iter = mEdgeToVertexMap.find(vKey);
				int idxVertex;
				if (iter == mEdgeToVertexMap.end())
				{
					idxVertex = meshData.vertices.size();
					VoxelMeshData::Vertex v;
					v.pos = VertexInterp(isolevel, info.v[0], info.v[1]);
					v.normal = Vector3::Zero();
					meshData.vertices.push_back(v);

					mEdgeToVertexMap.emplace(vKey, idxVertex);
				}
				else
				{
					idxVertex = iter->second;
				}
#else
				int idxVertex = meshData.vertices.size();
				VoxelMeshData::Vertex  v;
				v.pos = VertexInterp(isolevel, info.v[0], info.v[1]);
				v.normal = Vector3::Zero();
				meshData.vertices.push_back(v);
#endif

				meshData.indices.push_back(idxVertex);
			}
		}


		void generateVoxelTriangle(float isolevel , VoxelChunkData const& chunkData, IntVector3 const& voxelStartPos , IntVector3 const& size , std::vector< TriVertexInfo >& outTriVertices)
		{
			for (int k = 0; k < size.z; ++k)
			{
				for (int j = 0; j < size.y; ++j)
				{
					for (int i = 0; i < size.x; ++i)
					{
						IntVector3 voxelPos = voxelStartPos + IntVector3(i, j, k);
						GirdData gridValues[8];
						for (int idx = 0; idx < 8; ++idx)
						{
							int indexData = chunkData.config->toDataIndex(voxelPos, idx);

							gridValues[idx].value = chunkData.data[indexData];
							gridValues[idx].coord = &chunkData.config->coords[indexData];
						}
						GenerateVoxelTriangle(isolevel, gridValues, outTriVertices);
					}
				}
			}
		}


		static Vector3 VertexInterp(float isolevel, GirdData const& v1, GirdData const& v2)
		{
#if 0
			if (Math::Abs(isolevel - v1.value) < 0.00001)
				return v1.coord->pos;
			if (Math::Abs(isolevel - v2.value) < 0.00001)
				return v2.coord->pos;
#endif

			if (Math::Abs(v1.value - v2.value) < 0.00001)
				return 0.5 * (v1.coord->pos + v2.coord->pos);
			float alpha = (isolevel - v1.value) / (v2.value - v1.value);

			return Math::Lerp(v1.coord->pos, v2.coord->pos, alpha);
		}


		static void GenerateVoxelTriangle(float isolevel, GirdData grids[] , std::vector< TriVertexInfo >& outTriVertices)
		{
			uint32 cubeindex = 0;
			for (int i = 0; i < 8; ++i)
			{
				if (grids[i].value < isolevel) 
					cubeindex |= BIT(i);
			}

			int const* triangleTable = MarchingCubes::TriTable[cubeindex];
			for (int i = 0; triangleTable[i] != -1; ++i)
			{
				int edgeIndex = triangleTable[i];
				TriVertexInfo info;
				info.v[0] = grids[ MarchingCubes::EdgeToVertexMap[edgeIndex][0] ];
				info.v[1] = grids[ MarchingCubes::EdgeToVertexMap[edgeIndex][1] ];
				outTriVertices.push_back(info);
			}
		}

		static void GenerateMeshNormal(VoxelChunkData const& chunkData, VoxelMeshData& meshData)
		{
			for (int i = 0; i < meshData.vertices.size(); ++i)
			{
				auto& v = meshData.vertices[i];
				//N = div(v) = Dv/Dx i + Dv/Dy j + Dv/Dz k;
				// Dv / Dx = v(x + 1) + V(x - 1 ) / 2 * dx ;

				Vector3 voxelPosF = v.pos.div(chunkData.config->voxelSize);
				IntVector3 voxelPos;
				voxelPos.x = Math::FloorToInt(voxelPosF.x);
				voxelPos.y = Math::FloorToInt(voxelPosF.y);
				voxelPos.z = Math::FloorToInt(voxelPosF.z);
				Vector3 diff = voxelPosF - Vector3(voxelPos.x, voxelPos.y, voxelPos.z);

				int indexData = chunkData.config->toDataIndex(voxelPos);

				int indexX = indexData + 1;
				if (indexX > chunkData.data.size())
					indexX = indexData;
				float dx = chunkData.getInterpDataX(indexX, diff.x) - chunkData.getInterpDataX(indexData, diff.x);
				int indexY = indexData + chunkData.config->dataDim.x;
				if (indexY > chunkData.data.size())
					indexY = indexData;
				float dy = chunkData.getInterpDataY(indexY, diff.y) - chunkData.getInterpDataY(indexData, diff.y);
				int indexZ = indexData + +chunkData.config->dataDim.x*chunkData.config->dataDim.y;
				if (indexZ > chunkData.data.size())
					indexZ = indexData;
				float dz = chunkData.getInterpDataZ(indexZ, diff.z) - chunkData.getInterpDataZ(indexData, diff.z);
				v.normal = -Vector3(dx, dy, dz) / chunkData.config->voxelSize;
				v.normal.normalize();
			}
		}
	};

	class ShowValueProgram : public GlobalShaderProgram
	{
		DECLARE_SHADER_PROGRAM(ShowValueProgram, Global)

		virtual void bindParameters(ShaderParameterMap const& parameterMap)
		{		
			BIND_SHADER_PARAM(parameterMap, CubeSize);
			BIND_SHADER_PARAM(parameterMap, CubeOffset);
			BIND_SHADER_PARAM(parameterMap, DataTexture);
			BIND_SHADER_PARAM(parameterMap, DataDim);
			BIND_SHADER_PARAM(parameterMap, Isolevel);
		}
		static char const* GetShaderFileName()
		{
			return "Shader/Game/MarchingCubes";
		}
		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ EShader::Vertex , SHADER_ENTRY(ShowValueVS) },
				{ EShader::Pixel  , SHADER_ENTRY(ShowValuePS) },
			};
			return entries;
		}

		DEFINE_SHADER_PARAM(CubeSize);
		DEFINE_SHADER_PARAM(CubeOffset);
		DEFINE_SHADER_PARAM(DataTexture);
		DEFINE_SHADER_PARAM(DataDim);
		DEFINE_SHADER_PARAM(Isolevel);

	};
	IMPLEMENT_SHADER_PROGRAM(ShowValueProgram);



	class MeshRenderProgram : public GlobalShaderProgram
	{
		DECLARE_SHADER_PROGRAM(MeshRenderProgram, Global)
		static void SetupShaderCompileOption(ShaderCompileOption& option) 
		{
			option.addDefine(SHADER_PARAM(MESH_RENDER), true);
		}
		static char const* GetShaderFileName()
		{
			return "Shader/Game/MarchingCubes";
		}
		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ EShader::Vertex , SHADER_ENTRY(MeshRenderVS) },
				{ EShader::Pixel  , SHADER_ENTRY(MeshRenderPS) },
			};
			return entries;
		}



	};
	IMPLEMENT_SHADER_PROGRAM(MeshRenderProgram);


	class MarchingCubesTestStage : public TestRenderStageBase
	{
		using BaseClass = TestRenderStageBase;
	public:
		MarchingCubesTestStage() {}

		VoxelMeshBuilder    mMeshBuilder;
		VoxelMeshData       mMeshData;
		VoxelChunkConfig    mConfig;
		VoxelChunkData      mChunkData;

		ShowValueProgram*   mProgShowValue;
		MeshRenderProgram*  mProgMeshRender;
		RHITexture3DRef     mDataTexture;
		float mIsolevel = 0.5;
		bool  mbWireframeMode = false;
		bool  mbDrawData = true;
		Mesh mMesh;

		ERenderSystem getDefaultRenderSystem() override
		{
			return ERenderSystem::D3D11;
		}

		bool onInit() override
		{
			if (!BaseClass::onInit())
				return false;


			int chunkDataDim = 32;
			Vector3 chunkSize = Vector3(10, 10, 10);
			Vector3 voxelSize = chunkSize / float(chunkDataDim - 1);
			
			int noiseSize = 3.3;
			mConfig.initialize(IntVector3(chunkDataDim, chunkDataDim, chunkDataDim), voxelSize);
			mChunkData.initialize(mConfig);
			mChunkData.offset = Vector3::Zero();

			mMeshBuilder.initialize();

			{
				TPerlinNoise<true> noise;
				noise.repeat = noiseSize;
				float factor = noiseSize / float(chunkDataDim - 1);
				for (int k = 0; k < chunkDataDim; ++k)
				{
					for (int j = 0; j < chunkDataDim; ++j)
					{
						for (int i = 0; i < chunkDataDim; ++i)
						{
							Vector3 p = Vector3(i * factor, j * factor, k * factor);
							float nv = noise.getValue(p.x, p.y, p.z);
							p *= 2;
							float nv1 = noise.getValue(p.x, p.y, p.z);
							p *= 2;
							float nv2 = noise.getValue(p.x, p.y, p.z);
							int indexData = mConfig.toDataIndex(IntVector3(i, j, k));
							mChunkData.data[indexData] = 0.5 * ( nv + 0.5 * nv1 + 0.25 * nv2 ) / ( 1.75 )  + 0.5;
						}
					}
				}
			}
			
			::Global::GUI().cleanupWidget();
			auto devFrame = WidgetUtility::CreateDevFrame();

			devFrame->addText("Isolevel");
			FWidgetProperty::Bind(devFrame->addSlider(UI_ANY), mIsolevel, 0, 1, 1, [&](float value)
			{
				mIsolevel = value;
				updateMesh();
			});
			FWidgetProperty::Bind(devFrame->addCheckBox(UI_ANY, "Wireframe"), mbWireframeMode);
			FWidgetProperty::Bind(devFrame->addCheckBox(UI_ANY, "Draw Data"), mbDrawData);
			return true;
		}

		bool bUpdateingMesh = false;

		void updateMesh()
		{
			TimeScope Time("Update Mesh");
			TGuardValue< bool > scopedValue(::Global::GetDrawEngine().bBlockRender, true);
			mMeshBuilder.generateMesh(mIsolevel, mChunkData, mMeshData);
			MeshUtility::FillTriangleListNormal(mMesh.mInputLayoutDesc, mMeshData.vertices.data(), mMeshData.vertices.size(), mMeshData.indices.data(), mMeshData.indices.size());
			//VoxelMeshBuilder::GenerateMeshNormal(mChunkData, mMeshData);
			mMesh.createRHIResource(mMeshData.vertices.data(), mMeshData.vertices.size(), mMeshData.indices.data(), mMeshData.indices.size(), true);
		}



		virtual bool setupRenderSystem(ERenderSystem systemName) override
		{
			VERIFY_RETURN_FALSE(BaseClass::setupRenderSystem(systemName));

			VERIFY_RETURN_FALSE(SharedAssetData::createSimpleMesh());
			VERIFY_RETURN_FALSE(SharedAssetData::loadCommonShader());

			VERIFY_RETURN_FALSE(mProgShowValue = ShaderManager::Get().getGlobalShaderT< ShowValueProgram >());
			VERIFY_RETURN_FALSE(mProgMeshRender = ShaderManager::Get().getGlobalShaderT< MeshRenderProgram >());

			mDataTexture = RHICreateTexture3D(ETexture::R32F, mConfig.dataDim.x, mConfig.dataDim.y, mConfig.dataDim.z, 1, 1, TCF_DefalutValue, mChunkData.data.data());
			
			mMesh.mInputLayoutDesc.addElement(0, Vertex::ATTRIBUTE_POSITION, Vertex::eFloat3);
			mMesh.mInputLayoutDesc.addElement(0, Vertex::ATTRIBUTE_NORMAL, Vertex::eFloat3);
			updateMesh();
			return true;
		}

		virtual void preShutdownRenderSystem(bool bReInit) override
		{
			mProgShowValue = nullptr;
			mProgMeshRender = nullptr;
			mDataTexture.release();
			mMesh.releaseRHIResource();

			BaseClass::preShutdownRenderSystem(bReInit);
		}

		void onEnd() override
		{


			BaseClass::onEnd();
		}

		void restart() override
		{

		}
		void tick() override {}
		void updateFrame(int frame) override {}

		void onUpdate(long time) override
		{
			BaseClass::onUpdate(time);
		}

		void onRender(float dFrame) override
		{
			initializeRenderState();

			RHICommandList& commandList = RHICommandList::GetImmediateList();
			RHISetFrameBuffer(commandList, nullptr);
			RHISetFixedShaderPipelineState(commandList, mView.worldToClip);
			DrawUtility::AixsLine(commandList);

			{
				if (mbWireframeMode)
					RHISetRasterizerState(commandList, TStaticRasterizerState< ECullMode::None, EFillMode::Wireframe >::GetRHI());
				else
					RHISetRasterizerState(commandList, TStaticRasterizerState<>::GetRHI());

				RHISetDepthStencilState(commandList, TStaticDepthStencilState<>::GetRHI());
				RHISetShaderProgram(commandList, mProgMeshRender->getRHIResource());
				mView.setupShader(commandList, *mProgMeshRender);

				mMesh.draw(commandList);
			}

			if ( mbDrawData )
			{
				RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
				//RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());
				RHISetRasterizerState(commandList, TStaticRasterizerState<>::GetRHI());
				RHISetShaderProgram(commandList, mProgShowValue->getRHIResource());
				SET_SHADER_PARAM(commandList, *mProgShowValue, CubeSize, mConfig.voxelSize);
				SET_SHADER_PARAM(commandList, *mProgShowValue, CubeOffset, mChunkData.offset);
				SET_SHADER_PARAM(commandList, *mProgShowValue, DataDim, mConfig.dataDim);
				SET_SHADER_TEXTURE(commandList, *mProgShowValue, DataTexture, *mDataTexture);
				SET_SHADER_PARAM(commandList, *mProgShowValue, Isolevel, mIsolevel);
				mView.setupShader(commandList, *mProgShowValue);
				RHISetInputStream(commandList, nullptr, nullptr, 0);
				RHISetIndexBuffer(commandList, nullptr);
				int num = mConfig.dataDim.x * mConfig.dataDim.y * mConfig.dataDim.z;
				RHIDrawPrimitiveInstanced(commandList, EPrimitive::Points, 0, 1, num);
			}

			RHIGraphics2D& g = ::Global::GetRHIGraphics2D();
			g.beginRender();
			g.setTextColor(Color3f(1, 1, 0));
			SimpleTextLayout textLayout;
			textLayout.posX = 10;
			textLayout.posY = 10;
			textLayout.offset = 15;
			textLayout.show(g, "Mesh Triangle = %d", mMeshData.indices.size() / 3);
			textLayout.show(g, "Mesh Vertices = %d", mMeshData.vertices.size() );
			g.endRender();

		}

		bool onMouse(MouseMsg const& msg) override
		{
			if (!BaseClass::onMouse(msg))
				return false;
			return true;
		}

		bool onKey(KeyMsg const& msg) override
		{
			if (msg.isDown())
			{
				switch (msg.getCode())
				{
				default:
					break;
				}
			}
			return BaseClass::onKey(msg);
		}

		bool onWidgetEvent(int event, int id, GWidget* ui) override
		{
			switch (id)
			{
			default:
				break;
			}

			return BaseClass::onWidgetEvent(event, id, ui);
		}
	protected:
	};

	REGISTER_STAGE2("Maching Cubes", MarchingCubesTestStage, EStageGroup::FeatureDev, 1);
}