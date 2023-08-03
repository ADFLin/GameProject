#ifndef Voronoi_h__
#define Voronoi_h__

#include "Math/TVector2.h"

#include <queue>
#include <set>
#include <new>
#include <list>
#include <cassert>


namespace Voronoi
{
	typedef TVector2<float> Vector2;

	template< class T >
	struct BTreeNode
	{
		T* left;
		T* right;
		T* parent;
		bool isLeaf() const { return left != NULL; }
		void linkLeft( T& node )
		{
			left = &node;
			node.parent = this;
		}
		void linkRight( T& node )
		{
			right = &node;
			node.parent = this;
		}

		T* getLParent()
		{
			T* result = parent;
			T* child = this;
			while( result && result->left == child )
			{
				child = result;
				result = result->parent;
			}
			return result;
		}
		T* getRParent()
		{
			T* result = parent;
			T* child = this;
			while( result && result->right == child )
			{
				child = result;
				result = result->parent;
			}
			return result;
		}
		T* getLChild() 
		{
			T* result = right;
			while( result )
			{
				result = result->left;
			}
			return result;
		}
		T* getRChild()
		{
			T* result = left;
			while( result )
			{
				result = result->right;
			}
			return result;
		}
	};
	struct Para : public BTreeNode< Para >
	{
		// ( x - p.x )^2 + ( y - p.y )^2 = ( y - d )^2;
		float getY( float x , float d )
		{
			//y = [( x - p.x )^2 / ( p.y - d ) + ( p.y + d ) ] / 2
			float dx = x - p.x;
			return 0.5 * ( dx * dx / ( p.y - d ) + ( p.y + d ) );
		}
		int getX( float y , float d , float outX[] )
		{
			//x = p.x +/- sqrt( ( y - d )^2 - ( y - p.y )^2 )
			float d1 = y - d;
			float d2 = y - p.y;
			float delta = d1 * d1 - d2 * d2;
			if ( delta < 0.0 )
				return 0;

			if ( delta == 0.0 )
			{
				outX[0] = outX[1] = p.x;
				return 1;
			}

			delta = sqrt( delta );
			outX[0] = p.x - delta;
			outX[1] = p.x + delta;
			return 2;
		}
		int getIntersect( Para const& rhs , float d , float outX[] )
		{
			return getIntersect( p , rhs.p , d , outX );
		}

		static int getIntersect( Vector2 const& p1 , Vector2 const& p2 , float d , float outX[] )
		{
			// qi = yi - d
			// ( 1/q1 - 1/q2 ) x^2 - 2 ( x1/q1 - x2/q2 )x + y1 - y2 + x1^2/q1 - x2^2/q2 = 0
			float q1 = 1 / ( p1.y - d );
			float q2 = 1 / ( p2.y - d );
			float cy = p1.y - p2.y;
			float a_div = 1 / ( q1 - q2 );
			float qx1 = p1.x * q1;
			float qx2 = p2.x * q2;
			float b_N2 = ( qx1 - qx2 ) * a_div;
			float c = ( qx1 * p1.x - qx2 * p2.x + cy ) * a_div;

			float delta = b_N2 * b_N2 - c;
			if ( delta < 0 )
				return 0;
			if ( delta == 0 )
			{
				outX[0] = outX[1] = b_N2;
				return 1;
			}

			delta = sqrt( delta );
			outX[0] = b_N2 - delta;
			outX[1] = b_N2 + delta;
			return 2;
		}
		Vector2 p;
		
	};


	class CacheFactoryBase
	{
	public:
		~CacheFactoryBase()
		{
			cleanup();
		}

		void* fetchObject( size_t size )
		{
			if ( mCaches.empty() )
				return ::malloc( size );
			void* result = mCaches.back();
			mCaches.pop_back();
			return result;
		}

		void destroyObject( void* ptr )
		{
			mCaches.push_back( ptr );
		}

		void cleanup()
		{
			for( PtrVec::iterator iter = mCaches.begin() , itEnd = mCaches.end();
				 iter != itEnd; ++iter )
			{
				::free( *iter );
			}
			mCaches.clear();
		}

		typedef std::vector< void* > PtrVec;
		PtrVec mCaches;
	};
	template< class T >
	class CacheFactory : public CacheFactoryBase
	{
	public:
		T* create()
		{
			void* ptr = fetchObject( sizeof( T ) );
			T* result = new (ptr) T();
			return result;
		}

		template< class P1 >
		T* create( P1 p1 )
		{
			void* ptr = fetchObject( sizeof( T ) );
			T* result = new (ptr) T( p1 );
			return result;
		}

		template< class P1 , class P2 >
		T* create( P1 p1 , P2 p2 )
		{
			void* ptr = fetchObject( sizeof( T ) );
			T* result = new (ptr) T( p1 , p2 );
			return result;
		}

		template< class P1 , class P2 , class P3 >
		T* create( P1 p1 , P2 p2 , P3 p3 )
		{
			void* ptr = fetchObject( sizeof( T ) );
			T* result = new (ptr) T( p1 , p2 , p3 );
			return result;
		}

		void destroy( T* ptr )
		{
			ptr->~T();
			destroyObject( ptr );
		}
	};

	class VEdge;
	class VEvent;
	class VParabola;

	/*
	A structure that stores 2D point
	*/

	struct VPoint
	{
	public:

		double x, y;

		/*
		Constructor for structure; x, y - coordinates
		*/

		VPoint(){}
		VPoint(double nx, double ny) 
		{
			x = nx; 
			y = ny;
		}
	};

	/*
	The class for storing place / circle event in event queue.

	point		: the point at which current event occurs (top circle point for circle event, focus point for place event)
	pe			: whether it is a place event or not
	y			: y coordinate of "point", events are sorted by this "y"
	arch		: if "pe", it is an arch above which the event occurs
	*/

	class VEvent
	{
	public:
		VPoint *	point;
		bool		pe;
		double		y;
		VParabola * arch;

		/*
		Constructor for the class

		p		: point, at which the event occurs
		pev		: whether it is a place event or not
		*/

		VEvent(VPoint * p, bool pev)
		{
			assert( p );
			point	= p;
			pe		= pev;
			y		= p->y;
			arch	= 0;
		}

		void inate(){ point = nullptr; }
		bool isValid() const { return !!point; }
		/*
		function for comparing two events (by "y" property)
		*/

		struct CompareEvent
		{
			bool operator()(const VEvent* l, const VEvent* r) const { return (l->y < r->y); }
		};
	};

	/*
	A class that stores an edge in Voronoi diagram

	start		: pointer to start point
	end			: pointer to end point
	left		: pointer to Voronoi place on the left side of edge
	right		: pointer to Voronoi place on the right side of edge

	neighbour	: some edges consist of two parts, so we add the pointer to another part to connect them at the end of an algorithm

	direction	: directional vector, from "start", points to "end", normal of |left, right|
	f, g		: directional coeffitients satisfying equation y = f*x + g (edge lies on this line)
	*/

	class VEdge
	{
	public:

		VPoint *	start;
		VPoint *	end;
		VPoint *	left;
		VPoint *	right;

		VPoint	direction;

		double		f;
		double		g;

		VEdge * neighbour;

		/*
		Constructor of the class

		s		: pointer to start
		a		: pointer to left place
		b		: pointer to right place
		*/

		VEdge(VPoint * s, VPoint * a, VPoint * b)
			:direction( b->y - a->y, -(b->x - a->x) )
		{
			start		= s;
			left		= a;
			right		= b;
			neighbour	= 0;
			end			= 0;

			f = (b->x - a->x) / (a->y - b->y) ;
			g = s->y - f * s->x ;
		}

		/*
		Destructor of the class
		direction belongs only to the current edge, other pointers can be shared by other edges
		*/

		~VEdge()
		{

		}

	};

	/*
	A class that stores information about an item in beachline sequence (see Fortune's algorithm). 
	It can represent an arch of parabola or an intersection between two archs (which defines an edge).
	In my implementation I build a binary tree with them (internal nodes are edges, leaves are archs).
	*/

	class VParabola
	{
	public:

		/*
		isLeaf		: flag whether the node is Leaf or internal node
		site		: pointer to the focus point of parabola (when it is parabola)
		edge		: pointer to the edge (when it is an edge)
		cEvent		: pointer to the event, when the arch disappears (circle event)
		parent		: pointer to the parent node in tree
		*/

		bool		isLeaf;
		VPoint *	site;
		VEdge *		edge;
		VEvent *	cEvent;
		VParabola * parent;

		/*
		Constructors of the class (empty for edge, with focus parameter for an arch).
		*/

		VParabola();
		VParabola( VPoint* s );

		/*
		Access to the children (in tree).
		*/

		void		SetLeft (VParabola * p) {_left  = p; p->parent = this;}
		void		SetRight(VParabola * p) {_right = p; p->parent = this;}

		VParabola *	Left () { return _left;  }
		VParabola * Right() { return _right; }

		/*
		Some useful tree operations

		GetLeft			: returns the closest left leave of the tree
		GetRight		: returns the closest right leafe of the tree
		GetLeftParent	: returns the closest parent which is on the left
		GetLeftParent	: returns the closest parent which is on the right
		GetLeftChild	: returns the closest leave which is on the left of current node
		GetRightChild	: returns the closest leave which is on the right of current node
		*/

		static VParabola * GetLeft			(VParabola * p);
		static VParabola * GetRight			(VParabola * p);
		static VParabola * GetLeftParent	(VParabola * p);
		static VParabola * GetRightParent	(VParabola * p);
		static VParabola * GetLeftChild		(VParabola * p);
		static VParabola * GetRightChild	(VParabola * p);

	private:

		VParabola * _left;
		VParabola * _right;
	};


	/*
	Useful data containers for Vertices (places) and Edges of Voronoi diagram
	*/

	typedef std::list<VPoint *>		Vertices	;
	typedef std::list<VEdge *>		Edges		;

	/*
	Class for generating the Voronoi diagram
	*/

	class Solver
	{
	public:

		/*
		Constructor - without any parameters
		*/

		Solver();

		/*
		The only public function for generating a diagram

		input:
		v		: Vertices - places for drawing a diagram
		w		: width  of the result (top left corner is (0, 0))
		h		: height of the result
		output:
		pointer to list of edges

		All the data structures are managed by this class
		*/

		Edges *			GetEdges(Vertices * v, int w, int h);

	private:

		/*
		places		: container of places with which we work
		edges		: container of edges which will be teh result
		width		: width of the diagram
		height		: height of the diagram
		root		: the root of the tree, that represents a beachline sequence
		ly			: current "y" position of the line (see Fortune's algorithm)
		*/

		Vertices *		mPlaces;
		Edges *			mEdges;
		double			width, height;
		VParabola *		mRoot;
		double			mSweepY;

		/*
		points		: list of all new points that were created during the algorithm
		queue		: priority queue with events to process
		*/
		
		std::priority_queue<VEvent *, std::vector<VEvent *>, VEvent::CompareEvent> queue;

		/*
		InsertParabola		: processing the place event
		RemoveParabola		: processing the circle event
		FinishEdge			: recursively finishes all infinite edges in the tree
		GetXOfEdge			: returns the current x position of an intersection point of left and right parabolas
		GetParabolaByX		: returns the Parabola that is under this "x" position in the current beachline
		CheckCircle			: checks the circle event (disappearing) of this parabola
		GetEdgeInterse
		*/

		void		InsertParabola(VPoint * p);
		void		RemoveParabola(VEvent * e);
		void		FinishEdge(VParabola * n);
		double		GetXOfEdge(VParabola * par, double y);
		VParabola * GetParabolaByX(double xx);
		double		GetY(VPoint * p, double x);
		void		CheckCircle(VParabola * b);
		bool        GetEdgeIntersection(VEdge * a, VEdge * b , VPoint& result );


		std::vector<VPoint *> points;

		VPoint*     createPoint( double x , double y )
		{
			VPoint * result = mPointFactory.create( x , y ); 
			points.push_back(result);
			return result;
		}

		CacheFactory< VPoint >    mPointFactory;
		CacheFactory< VEvent >    mEventFactory;
		CacheFactory< VParabola > mParabolaFactory;
		CacheFactory< VEdge >     mEdgeFactory;
	};


	


}//namespace Voronoi

namespace Vor2
{


#define DELETED -2

	struct Freenode
	{
		Freenode* nextfree;
	};

	struct Freelist
	{
		Freenode * head;
		int nodesize;
	};

	typedef struct tagPoint
	{
		float x ;
		float y ;
	} Point ;

	/* structure used both for sites and for vertices */

	typedef struct tagSite
	{
		Point coord ;
		int sitenbr ;
		int refcnt ;
	} Site ;


	typedef struct tagEdge
	{
		float a, b, c ;
		Site * ep[2] ;
		Site * reg[2] ;
		int edgenbr ;
	} Edge ;

#define LEDGE 0
#define REDGE 1

	struct Halfedge
	{
		Halfedge * ELleft ;
		Halfedge * ELright ;
		Edge * ELedge ;
		int ELrefcnt ;
		char ELpm ;
		Site * vertex ;
		float ystar ;
		Halfedge * PQnext ;
	} ;

	/* edgelist.c */
	class EdgeList
	{
	public:

		void initialize();
		Halfedge* create(Edge * e, int pm);
		void insert(Halfedge * lb, Halfedge * newE );
		Halfedge* gethash(int b);
		Halfedge* leftbnd(Point * p);
		void remove(Halfedge * he);

		int        ELhashsize ;
		Freelist   hfl ;
		Halfedge*  ELleftend;
		Halfedge*  ELrightend;
		Halfedge** ELhash ;
	};

	EdgeList mEL;

	Halfedge * ELright(Halfedge *) ;
	Halfedge * ELleft(Halfedge *) ;
	Site * leftreg(Halfedge *) ;
	Site * rightreg(Halfedge *) ;


	/* geometry.c */
	void geominit(void) ;
	Edge * bisect(Site *, Site *) ;
	Site * intersect(Halfedge *, Halfedge *) ;
	int right_of(Halfedge *, Point *) ;
	void endpoint(Edge *, int, Site *) ;
	float dist(Site *, Site *) ;
	void makevertex(Site *) ;
	void decref(Site *) ;
	void incref(Site *) ;
	extern float deltax, deltay ;
	extern int nsites, nedges, sqrt_nsites, nvertices ;
	extern Freelist sfl, efl ;

	class PQueue
	{
	public:
		void      initialize();
		void      insert(Halfedge * he, Site * v, float offset) ;
		void      remove(Halfedge * he);
		int       bucket(Halfedge * he);
		int       empty();
		Point     min();
		Halfedge* extractmin();
		
		int PQmin, PQcount, PQhashsize ;
		Halfedge * PQhash ;
	};

	PQueue mQueue;

	/* heap.c */


	/* main.c */
	extern int sorted, triangulate, plot, debug, nsites, siteidx ;
	extern float xmin, xmax, ymin, ymax ;
	extern Site * sites ;
	extern Freelist sfl ;

	/* getopt.c */
	extern int getopt(int, char *const *, const char *);

	/* memory.c */
	void freeinit(Freelist *, int) ;
	char *getfree(Freelist *) ;
	void makefree(Freenode *, Freelist *) ;
	char *myalloc(unsigned) ;

	/* output.c */
	void openpl(void) ;
	void line(float, float, float, float) ;
	void circle(float, float, float) ;
	void range(float, float, float, float) ;
	void out_bisector(Edge *) ;
	void out_ep(Edge *) ;
	void out_vertex(Site *) ;
	void out_site(Site *) ;
	void out_triple(Site *, Site *, Site *) ;
	void plotinit(void) ;
	void clip_line(Edge *) ;

	/* voronoi.c */
	void voronoi(Site *(*)()) ;

}
#endif // Voronoi_h__
