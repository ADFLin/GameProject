#ifndef BspFile30_h__
#define BspFile30_h__

// BSP30.H
// Defines data structures for loading a BSP-30 (Quake 1 / Half-Life 1) file
#include "Core/IntegerType.h"

namespace BspV30 
{
	// Constant maximums of various kinds of data
	const int MAX_MODELS = 400;
	const int MAX_BRUSHES = 4096;
	const int MAX_ENTITIES = 1024;
	const int MAX_PLANES = 32767;
	const int MAX_NODES = 32767;
	const int MAX_CLIPNODES = 32767;
	const int MAX_LEAVES = 8192;
	const int MAX_VERTS = 65535;
	const int MAX_FACES = 65535;
	const int MAX_MARKSURFACES = 65535;
	const int MAX_TEXINFO = 8192;
	const int MAX_EDGES = 256000;
	const int MAX_SURFEDGES = 512000;
	const int MAX_TEXTURES = 512;
	const int MAX_MIPTEX = 0x200000;
	const int MAX_LIGHTING = 0x200000;
	const int MAX_VISIBILITY = 0x200000;
	const int MAX_PORTALS = 65536;

	const int MAX_LIGHT_MAP_NUM = 4;

	// Possible contents of leaves and clipnodes
	enum
	{
		CONTENTS_EMPTY = -1,
		CONTENTS_SOLID = -2,
		CONTENTS_WATER = -3,
		CONTENTS_SLIME = -4,
		CONTENTS_LAVA = -5,
		CONTENTS_SKY = -6,
		CONTENTS_ORIGIN = -7,
		CONTENTS_CLIP = -8,
		CONTENTS_CURRENT_0 = -9,			// I don't know what these and the following are for
		CONTENTS_CURRENT_90 = -10,
		CONTENTS_CURRENT_180 = -11,
		CONTENTS_CURRENT_270 = -12,
		CONTENTS_CURRENT_UP = -13,
		CONTENTS_CURRENT_DOWN = -14,
		CONTENTS_TRANSLUCENT = -15
	};

	// Automatic ambient sounds
	enum
	{
		AMBIENT_WATER = 0,
		AMBIENT_SKY,
		AMBIENT_SLIME,
		AMBIENT_LAVA
	};

	// BSP-30 files contain these lumps
	enum
	{
		LUMP_ENTITIES = 0,
		LUMP_PLANES,
		LUMP_TEXTURES,
		LUMP_VERTICES,
		LUMP_VISIBILITY,
		LUMP_NODES,
		LUMP_TEXINFO,
		LUMP_FACES,
		LUMP_LIGHTING,
		LUMP_CLIPNODES,
		LUMP_LEAVES,
		LUMP_MARKSURFACES,
		LUMP_EDGES,
		LUMP_SURFEDGES,
		LUMP_MODELS,
		NUM_LUMPS						// This must be the last enum in the list
	};

	// BSP files begin with a header, followed by a directory giving the location and length of each data lump
	// BSP-30 header and directory
	struct header
	{
		uint32 version;			// This must be 30 for a BSP-30 file
		struct lump
		{
			uint32 offset;
			uint32 length;
		}directory[NUM_LUMPS];
	};

	// Entities lump is just a long, null-terminated string

	// Planes lump contains plane structures
	struct plane_t
	{
		float normal[3];
		float dist;						// Plane equation is: Normal * X = Dist
		enum Type
		{
			PLANE_X = 0,				// Plane is perpendicular to given axis
			PLANE_Y,
			PLANE_Z,
			PLANE_ANYX,					// Dominant axis (axis along which projection of normal has greatest magnitude)
			PLANE_ANYY,
			PLANE_ANYZ
		} type;
	};

	// Textures lump begins with a header, followed by offsets to miptex structures, then miptex structures
	struct texture_lump_header
	{
		uint32  numMiptex;		// Number of miptex structures
		uint32  miptexOffset[1];
	};
	// Followed by 32-bit offsets (within texture lump) to (num_miptex) miptex structures
	struct miptex_t
	{
		char name[16];					// Name of texture, for reference from external WAD3 file
		uint32 width, height;
		uint32 offsets[4];		// Offsets to texture data, apparently - 0 if texture data not included
	};

	// Vertices lump contains vertex structures
	struct vertex_t
	{
		float x, y, z;
	};

	// Visibility lump is a huge bitfield of unknown format

	// Nodes lump contains node structures
	struct node_t
	{
		uint32 plane_idx;	    // Index into planes lump
		int16  children[2];		// If > 0, then indices into nodes; otherwise, bitwise inverse = indices into leaves
		int16  min[3], max[3];	// Bounding box
		uint16 firstface, numfaces;		// Index and count into faces lump
	};

	// Texinfo lump contains texinfo structures
	struct texinfo_t
	{
		union
		{
			struct  
			{
				float s[4], t[4];	// 1st and 2nd rows of texture matrix - multiply by vert location to get texcoords
			};
			float m[2][4];
		};

		uint32 miptex;		// Index into textures lump
		uint32 flags;		// Not sure what this is for - seems to always be 0
	};

	// Faces lump contains face structures
	struct face_t
	{
		uint16 planeIndex;		// Index into planes lump
		uint16 side;			// If nonzero, must flip the normal of the given plane to match face orientation
		uint32 firstEdge;		// Index (and then count) into surfedges lump
		uint16 numEdges;
		uint16 texinfo;			// Index into texinfo lump
		uint8  lightStyles[4];  // Has something to do with lightmapping
		uint32 lightOffset;     // Ditto
	};

	// Lighting lump is of unknown format

	// Clipnodes lump contains clipnode structures
	struct clipnode_t
	{
		uint32 planeIndex;	// Index into planes lump
		int16  children[2];	// If > 0, then indices into clipnodes; otherwise, contents enum
	};

	// Leaves lump contains leaf structures
	struct leaf_t
	{
		int32  contents;	    // Contents enum
		int32  visOffset;		// Something to do with visibility
		int16  min[3], max[3];	// Bounding box
		uint16 firstMarkSurface;
		uint16 numMarkSurfaces;	// Index and count into marksurfaces lump
		uint8  ambientLevel[4];	// Ambient sound levels, indexed by ambient enum
	};

	// Marksurfaces lump is array of unsigned short indices into faces lump.  Leaves index into marksurfaces,
	// which index into faces; but nodes index into faces directly.
	typedef uint16 marksurface;

	// Edges lump contains edge structures - note, edge 0 is never used
	struct edge_t
	{
		uint16 vertex[2];		// Indices into vertices lump
	};

	// Surfedges lump is array of signed int indices into edge lump, where a negative index indicates
	// using the referenced edge in the opposite direction.  Faces index into surfedges, which index
	// into edges, which finally index into vertices.
	typedef int32 surfedge;

	// Models lump contains model structures - these are entity-tied BSP models like func_doors and such
	struct model_t
	{
		float min[3], max[3];			// Bounding box
		float origin[3];				// Local origin
		uint32 headnode[4];		// Indices into either nodes or clipnodes - has to do with collision
		uint32 visleaves;			// Unknown; something to do with visibility?
		uint32 firstface, numfaces;	// Index and count into faces lump
	};
}

#endif // BspFile30_h__