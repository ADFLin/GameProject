#pragma once
#ifndef DuffDevice_H_8B18D1C6_5F46_4037_BAF6_E881183FA3DF
#define DuffDevice_H_8B18D1C6_5F46_4037_BAF6_E881183FA3DF

#define DUFF_DEVICE_2( DIM , OP )\
	{\
		int constexpr DUFF_BLOCK_SIZE = 4;\
		int blockCount = ((DIM) + DUFF_BLOCK_SIZE - 1) / DUFF_BLOCK_SIZE;\
		switch ((DIM) % DUFF_BLOCK_SIZE)\
		{\
		case 0: do { OP;\
		case 1: OP;\
			} while (--blockCount > 0);\
		}\
	}


#define DUFF_DEVICE_4( DIM , OP )\
	{\
		int constexpr DUFF_BLOCK_SIZE = 4;\
		int blockCount = ((DIM) + DUFF_BLOCK_SIZE - 1) / DUFF_BLOCK_SIZE;\
		switch ((DIM) % DUFF_BLOCK_SIZE)\
		{\
		case 0: do { OP;\
		case 3: OP;\
		case 2: OP;\
		case 1: OP;\
			} while (--blockCount > 0);\
		}\
	}


#define DUFF_DEVICE_8( DIM , OP )\
	{\
		int constexpr DUFF_BLOCK_SIZE = 8;\
		int blockCount = ((DIM) + DUFF_BLOCK_SIZE - 1) / DUFF_BLOCK_SIZE;\
		switch ((DIM) % DUFF_BLOCK_SIZE)\
		{\
		case 0: do { OP;\
		case 7: OP;\
		case 6: OP;\
		case 5: OP;\
		case 4: OP;\
		case 3: OP;\
		case 2: OP;\
		case 1: OP;\
			} while (--blockCount > 0);\
		}\
	}

#endif // DuffDevice_H_8B18D1C6_5F46_4037_BAF6_E881183FA3DF