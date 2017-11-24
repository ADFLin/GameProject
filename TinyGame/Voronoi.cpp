#include "Voronoi.h"

namespace Voronoi
{

	/*
	Constructors
	*/

	VParabola::VParabola()
	{
		site	= 0;
		isLeaf	= false;
		cEvent	= 0;
		edge	= 0;
		parent	= 0;
	}

	VParabola::VParabola(VPoint * s)
	{
		site	= s; 
		isLeaf	= true;
		cEvent	= 0;
		edge	= 0;
		parent	= 0;
	}

	/*
	Tree operations (described in the header file)
	*/

	VParabola * VParabola::GetLeft			(VParabola * p)
	{
		return GetLeftChild(GetLeftParent(p));
	}


	VParabola * VParabola::GetRight			(VParabola * p)
	{
		return GetRightChild(GetRightParent(p));
	}

	VParabola * VParabola::GetLeftParent	(VParabola * p)
	{
		VParabola * par		= p->parent;
		VParabola * pLast	= p;
		while(par->Left() == pLast) 
		{ 
			if(!par->parent) return 0;
			pLast = par; 
			par = par->parent; 
		}
		return par;
	}

	VParabola * VParabola::GetRightParent	(VParabola * p)
	{
		VParabola * par		= p->parent;
		VParabola * pLast	= p;
		while(par->Right() == pLast) 
		{ 
			if(!par->parent) return 0;
			pLast = par; par = par->parent; 
		}
		return par;
	}

	VParabola * VParabola::GetLeftChild		(VParabola * p)
	{
		if(!p) return 0;
		VParabola * par = p->Left();
		while(!par->isLeaf) par = par->Right();
		return par;
	}

	VParabola * VParabola::GetRightChild	(VParabola * p)
	{
		if(!p) return 0;
		VParabola * par = p->Right();
		while(!par->isLeaf) par = par->Left();
		return par;
	}

	Solver::Solver()
	{
		mEdges = 0;
	}

	Edges * Solver::GetEdges(Vertices * v, int w, int h)
	{
		mPlaces = v;
		width = w;
		height = h;
		mRoot = 0;

		if(!mEdges)
		{
			mEdges = new Edges();
		}
		else 
		{
			for( std::vector< VPoint* >::iterator	i = points.begin(); i != points.end(); ++i) 
				mPointFactory.destroy(*i);
			points.clear();

			for(Edges::iterator	i = mEdges->begin(); i != mEdges->end(); ++i) 
				mEdgeFactory.destroy(*i);
			mEdges->clear();
		}

		for(Vertices::iterator i = mPlaces->begin(); i!=mPlaces->end(); ++i)
		{
			queue.push( mEventFactory.create( *i , true ) );
		}

		VEvent * e;
		while(!queue.empty())
		{
			e = queue.top();
			queue.pop();
			mSweepY = e->point->y;

			if ( e->is() )
			{
				if(e->pe) 
					InsertParabola(e->point);
				else 
					RemoveParabola(e);
			}

			mEventFactory.destroy( e );
		}

		FinishEdge( mRoot );

		for(Edges::iterator i = mEdges->begin(); i != mEdges->end(); ++i)
		{
			if( (*i)->neighbour) 
			{
				(*i)->start = (*i)->neighbour->end;
				mEdgeFactory.destroy( (*i)->neighbour );
			}
		}

		return mEdges;
	}

	void	Solver::InsertParabola(VPoint * p)
	{
		if( !mRoot )
		{
			mRoot = mParabolaFactory.create(p); 
			return;
		}

		if(mRoot->isLeaf && mRoot->site->y - p->y < 1) // degenerovan?pøípad - ob?spodn?místa ve stejn?v?ce
		{
			VPoint * fp = mRoot->site;
			mRoot->isLeaf = false;
			mRoot->SetLeft( mParabolaFactory.create(fp) );
			mRoot->SetRight( mParabolaFactory.create(p)  );
			
			VPoint * s = createPoint((p->x + fp->x)/2, height); // zaèátek hrany uprostøed míst

			if(p->x > fp->x)
				mRoot->edge = mEdgeFactory.create(s, fp, p); // rozhodnu, kter?vlevo, kter?vpravo
			else 
				mRoot->edge = mEdgeFactory.create(s, p, fp);
			
			mEdges->push_back(mRoot->edge);
			return;
		}

		VParabola * par = GetParabolaByX(p->x);

		if(par->cEvent)
		{
			par->cEvent->inate();
			par->cEvent = 0;
		}

		VPoint * start = createPoint(p->x, GetY(par->site, p->x));

		VEdge * el = mEdgeFactory.create(start, par->site, p);
		VEdge * er = mEdgeFactory.create(start, p, par->site);

		el->neighbour = er;
		mEdges->push_back(el);

		// pøestavuju strom .. vkládám novou parabolu
		par->edge = er;
		par->isLeaf = false;

		VParabola * pOA = mParabolaFactory.create(par->site);
		VParabola * p1 = mParabolaFactory.create(p);
		VParabola * pOB = mParabolaFactory.create(par->site);

		VParabola* left = mParabolaFactory.create();
		left->edge = el;
		left->SetLeft(pOA);
		left->SetRight(p1);

		par->SetRight(pOB);
		par->SetLeft( left );
		
		CheckCircle(pOA);
		CheckCircle(pOB);
	}

	void	Solver::RemoveParabola(VEvent * e)
	{
		VParabola * p1 = e->arch;

		VParabola * xl = VParabola::GetLeftParent(p1);
		VParabola * xr = VParabola::GetRightParent(p1);

		VParabola * pCL = VParabola::GetLeftChild(xl);
		VParabola * pCR = VParabola::GetRightChild(xr);

		// 	if(p0 == p2) std::cout << "chyba - prav?a lev?parabola m?stejn?ohnisko!\n";

		if(pCL->cEvent){ pCL->cEvent->inate(); pCL->cEvent = 0; }
		if(pCR->cEvent){ pCR->cEvent->inate(); pCR->cEvent = 0; }

		VPoint * p = createPoint(e->point->x, GetY(p1->site, e->point->x));
	
		xl->edge->end = p;
		xr->edge->end = p;

		VParabola * higher;
		VParabola * par = p1;
		while(par != mRoot)
		{
			par = par->parent;
			if(par == xl) higher = xl;
			if(par == xr) higher = xr;
		}
		higher->edge = mEdgeFactory.create(p, pCL->site, pCR->site);
		mEdges->push_back(higher->edge);

		VParabola * gparent = p1->parent->parent;
		if(p1->parent->Left() == p1)
		{
			if(gparent->Left()  == p1->parent) gparent->SetLeft ( p1->parent->Right() );
			if(gparent->Right() == p1->parent) gparent->SetRight( p1->parent->Right() );
		}
		else
		{
			if(gparent->Left()  == p1->parent) gparent->SetLeft ( p1->parent->Left()  );
			if(gparent->Right() == p1->parent) gparent->SetRight( p1->parent->Left()  );
		}

		mParabolaFactory.destroy( p1->parent );
		mParabolaFactory.destroy( p1 );

		CheckCircle(pCL);
		CheckCircle(pCR);
	}

	void	Solver::CheckCircle(VParabola * b)
	{
		VParabola * lp = VParabola::GetLeftParent (b);
		VParabola * rp = VParabola::GetRightParent(b);

		VParabola * a  = VParabola::GetLeftChild (lp);
		VParabola * c  = VParabola::GetRightChild(rp);

		if(!a || !c || a->site == c->site) 
			return;

		VPoint s;
		if ( !GetEdgeIntersection(lp->edge, rp->edge , s ) )
			return;

		double dx = a->site->x - s.x;
		double dy = a->site->y - s.y;

		double d = std::sqrt( (dx * dx) + (dy * dy) );

		if(s.y - d >= mSweepY )
			return;

		VEvent * e = mEventFactory.create( createPoint(s.x, s.y - d) , false );
		b->cEvent = e;
		e->arch = b;
		queue.push(e);
	}


	void	Solver::FinishEdge(VParabola * n)
	{
		if( !n->isLeaf ) 
		{ 
			double mx;
			if(n->edge->direction.x > 0.0)	mx = std::max(width,	n->edge->start->x + 10 );
			else							mx = std::min(0.0,		n->edge->start->x - 10);

			n->edge->end = createPoint( mx, mx * n->edge->f + n->edge->g );

			FinishEdge(n->Left() );
			FinishEdge(n->Right());
		}
		mParabolaFactory.destroy( n );
	}

	double	Solver::GetXOfEdge(VParabola * par, double y)
	{
		VParabola * left = VParabola::GetLeftChild(par);
		VParabola * right= VParabola::GetRightChild(par);

		VPoint * p = left->site;
		VPoint * r = right->site;

		double dp = 2.0 * (p->y - y);
		double a1 = 1.0 / dp;
		double b1 = -p->x / dp;
		double c1 = y + dp / 4 + p->x * p->x / dp;

		dp = 2.0 * (r->y - y);
		double a2 = 1.0 / dp;
		double b2 = -r->x/dp;
		double c2 = mSweepY + dp / 4 + r->x * r->x / dp;

		double a = a1 - a2;
		double b = b1 - b2;
		double c = c1 - c2;

		double disc = b*b - a * c;
		double x1 = (-b + std::sqrt(disc)) / (a);
		double x2 = (-b - std::sqrt(disc)) / (a);

		double ry;
		if(p->y < r->y ) ry =  std::max(x1, x2);
		else ry = std::min(x1, x2);

		return ry;
	}

	VParabola * Solver::GetParabolaByX(double xx)
	{
		VParabola * par = mRoot;
		double x = 0.0;

		while(!par->isLeaf) // projdu stromem dokud nenarazím na vhodn?list
		{
			x = GetXOfEdge(par, mSweepY);
			if(x>xx) par = par->Left();
			else par = par->Right();				
		}
		return par;
	}

	double	Solver::GetY(VPoint * p, double x) // ohnisko, x-souøadnice
	{
		double dp = 2 * (p->y - mSweepY);
		double a1 = 1 / dp;
		double b1 = -2 * p->x / dp;
		double c1 = mSweepY + dp / 4 + p->x * p->x / dp;

		return(a1*x*x + b1*x + c1);
	}

	bool Solver::GetEdgeIntersection(VEdge * a, VEdge * b , VPoint& result )
	{
		double x = (b->g-a->g) / (a->f - b->f);
		double y = a->f * x + a->g;

		if ( ((x - a->start->x)/a->direction.x < 0) ||
			 ((y - a->start->y)/a->direction.y < 0) ||
		     ((x - b->start->x)/b->direction.x < 0) ||
		     ((y - b->start->y)/b->direction.y < 0) )
			return false;

		result.x = x;
		result.y = y;
		return true;
	}


}//namespace Voronoi

namespace Vor2
{

	int ntry, totalsearch ;

	void EdgeList::initialize(void)
	{
		int i ;

		freeinit(&hfl, sizeof(Halfedge)) ;
		ELhashsize = 2 * sqrt_nsites ;
		ELhash = (Halfedge **)myalloc( sizeof(*ELhash) * ELhashsize) ;
		for (i = 0  ; i < ELhashsize  ; i++)
		{
			ELhash[i] = (Halfedge *)NULL ;
		}
		ELleftend = create((Edge *)NULL, 0) ;
		ELrightend = create((Edge *)NULL, 0) ;
		ELleftend->ELleft = (Halfedge *)NULL ;
		ELleftend->ELright = ELrightend ;
		ELrightend->ELleft = ELleftend ;
		ELrightend->ELright = (Halfedge *)NULL ;
		ELhash[0] = ELleftend ;
		ELhash[ELhashsize-1] = ELrightend ;
	}

	Halfedge* EdgeList::create(Edge * e, int pm)
	{
		Halfedge * answer ;

		answer = (Halfedge *)getfree(&hfl) ;
		answer->ELedge = e ;
		answer->ELpm = pm ;
		answer->PQnext = (Halfedge *)NULL ;
		answer->vertex = (Site *)NULL ;
		answer->ELrefcnt = 0 ;
		return (answer) ;
	}

	void EdgeList::insert(Halfedge * lb, Halfedge * newE )
	{ 
		newE->ELleft = lb ;
		newE->ELright = lb->ELright ;
		(lb->ELright)->ELleft = newE ;
		lb->ELright = newE ;
	}

	/* Get entry from hash table, pruning any deleted nodes */

	Halfedge* EdgeList::gethash(int b)
	{
		Halfedge * he ;

		if ((b < 0) || (b >= ELhashsize))
		{
			return ((Halfedge *)NULL) ;
		}
		he = ELhash[b] ;
		if ((he == (Halfedge *)NULL) || (he->ELedge != (Edge *)DELETED))
		{
			return (he) ;
		}
		/* Hash table points to deleted half edge.  Patch as necessary. */
		ELhash[b] = (Halfedge *)NULL ;
		if ((--(he->ELrefcnt)) == 0)
		{
			makefree((Freenode *)he, (Freelist *)&hfl) ;
		}
		return ((Halfedge *)NULL) ;
	}

	Halfedge* EdgeList::leftbnd(Point * p)
	{
		int i, bucket ;
		Halfedge * he ;

		/* Use hash table to get close to desired halfedge */
		bucket = (p->x - xmin) / deltax * ELhashsize ;
		if (bucket < 0)
		{
			bucket = 0 ;
		}
		if (bucket >= ELhashsize)
		{
			bucket = ELhashsize - 1 ;
		}
		he = mEL.gethash(bucket) ;
		if  (he == (Halfedge *)NULL)
		{
			for (i = 1 ; 1 ; i++)
			{
				if ((he = mEL.gethash(bucket-i)) != (Halfedge *)NULL)
				{
					break ;
				}
				if ((he = mEL.gethash(bucket+i)) != (Halfedge *)NULL)
				{
					break ;
				}
			}
			totalsearch += i ;
		}
		ntry++ ;
		/* Now search linear list of halfedges for the corect one */
		if (he == ELleftend || (he != ELrightend && right_of(he,p)))
		{
			do  {
				he = he->ELright ;
			} while (he != ELrightend && right_of(he,p)) ;
			he = he->ELleft ;
		}
		else
		{
			do  {
				he = he->ELleft ;
			} while (he != ELleftend && !right_of(he,p)) ;
		}
		/*** Update hash table and reference counts ***/
		if ((bucket > 0) && (bucket < ELhashsize-1))
		{
			if (ELhash[bucket] != (Halfedge *)NULL)
			{
				(ELhash[bucket]->ELrefcnt)-- ;
			}
			ELhash[bucket] = he ;
			(ELhash[bucket]->ELrefcnt)++ ;
		}
		return (he) ;
	}

	/*** This delete routine can't reclaim node, since pointers from hash
	: table may be present.
	***/

	void EdgeList::remove(Halfedge * he)
	{
		(he->ELleft)->ELright = he->ELright ;
		(he->ELright)->ELleft = he->ELleft ;
		he->ELedge = (Edge *)DELETED ;
	}

	Halfedge *
		ELright(Halfedge * he)
	{
		return (he->ELright) ;
	}

	Site * bottomsite ;

	Halfedge *
		ELleft(Halfedge * he)
	{
		return (he->ELleft) ;
	}

	Site * leftreg(Halfedge * he)
	{
		if (he->ELedge == (Edge *)NULL)
		{
			return(bottomsite) ;
		}
		return (he->ELpm == LEDGE ? he->ELedge->reg[LEDGE] :
			he->ELedge->reg[REDGE]) ;
	}

	Site *
		rightreg(Halfedge * he)
	{
		if (he->ELedge == (Edge *)NULL)
		{
			return(bottomsite) ;
		}
		return (he->ELpm == LEDGE ? he->ELedge->reg[REDGE] :
			he->ELedge->reg[LEDGE]) ;
	}


	float deltax, deltay ;
	int nedges, sqrt_nsites, nvertices ;
	Freelist efl ;

	void
		geominit(void)
	{
		freeinit(&efl, sizeof(Edge)) ;
		nvertices = nedges = 0 ;
		sqrt_nsites = (int)sqrtf( nsites+4 ) ;
		deltay = ymax - ymin ;
		deltax = xmax - xmin ;
	}

	Edge *
		bisect(Site * s1, Site * s2)
	{
		float dx, dy, adx, ady ;
		Edge * newedge ;

		newedge = (Edge *)getfree(&efl) ;
		newedge->reg[0] = s1 ;
		newedge->reg[1] = s2 ;
		incref(s1) ;
		incref(s2) ;
		newedge->ep[0] = newedge->ep[1] = (Site *)NULL ;
		dx = s2->coord.x - s1->coord.x ;
		dy = s2->coord.y - s1->coord.y ;
		adx = dx>0 ? dx : -dx ;
		ady = dy>0 ? dy : -dy ;
		newedge->c = s1->coord.x * dx + s1->coord.y * dy + (dx*dx +
			dy*dy) * 0.5 ;
		if (adx > ady)
		{
			newedge->a = 1.0 ;
			newedge->b = dy/dx ;
			newedge->c /= dx ;
		}
		else
		{
			newedge->b = 1.0 ;
			newedge->a = dx/dy ;
			newedge->c /= dy ;
		}
		newedge->edgenbr = nedges ;
		out_bisector(newedge) ;
		nedges++ ;
		return (newedge) ;
	}

	Site *
		intersect(Halfedge * el1, Halfedge * el2)
	{
		Edge * e1, * e2, * e ;
		Halfedge * el ;
		float d, xint, yint ;
		int right_of_site ;
		Site * v ;

		e1 = el1->ELedge ;
		e2 = el2->ELedge ;
		if ((e1 == (Edge*)NULL) || (e2 == (Edge*)NULL))
		{
			return ((Site *)NULL) ;
		}
		if (e1->reg[1] == e2->reg[1])
		{
			return ((Site *)NULL) ;
		}
		d = (e1->a * e2->b) - (e1->b * e2->a) ;
		if ((-1.0e-10 < d) && (d < 1.0e-10))
		{
			return ((Site *)NULL) ;
		}
		xint = (e1->c * e2->b - e2->c * e1->b) / d ;
		yint = (e2->c * e1->a - e1->c * e2->a) / d ;
		if ((e1->reg[1]->coord.y < e2->reg[1]->coord.y) ||
			(e1->reg[1]->coord.y == e2->reg[1]->coord.y &&
			e1->reg[1]->coord.x < e2->reg[1]->coord.x))
		{
			el = el1 ;
			e = e1 ;
		}
		else
		{
			el = el2 ;
			e = e2 ;
		}
		right_of_site = (xint >= e->reg[1]->coord.x) ;
		if ((right_of_site && (el->ELpm == LEDGE)) ||
			(!right_of_site && (el->ELpm == REDGE)))
		{
			return ((Site *)NULL) ;
		}
		v = (Site *)getfree(&sfl) ;
		v->refcnt = 0 ;
		v->coord.x = xint ;
		v->coord.y = yint ;
		return (v) ;
	}

	/*** returns 1 if p is to right of halfedge e ***/

	int
		right_of(Halfedge * el, Point * p)
	{
		Edge * e ;
		Site * topsite ;
		int right_of_site, above, fast ;
		float dxp, dyp, dxs, t1, t2, t3, yl ;

		e = el->ELedge ;
		topsite = e->reg[1] ;
		right_of_site = (p->x > topsite->coord.x) ;
		if (right_of_site && (el->ELpm == LEDGE))
		{
			return (1) ;
		}
		if(!right_of_site && (el->ELpm == REDGE))
		{
			return (0) ;
		}
		if (e->a == 1.0)
		{
			dyp = p->y - topsite->coord.y ;
			dxp = p->x - topsite->coord.x ;
			fast = 0 ;
			if ((!right_of_site & (e->b < 0.0)) ||
				(right_of_site & (e->b >= 0.0)))
			{
				fast = above = (dyp >= e->b*dxp) ;
			}
			else
			{
				above = ((p->x + p->y * e->b) > (e->c)) ;
				if (e->b < 0.0)
				{
					above = !above ;
				}
				if (!above)
				{
					fast = 1 ;
				}
			}
			if (!fast)
			{
				dxs = topsite->coord.x - (e->reg[0])->coord.x ;
				above = (e->b * (dxp*dxp - dyp*dyp))
					<
					(dxs * dyp * (1.0 + 2.0 * dxp /
					dxs + e->b * e->b)) ;
				if (e->b < 0.0)
				{
					above = !above ;
				}
			}
		}
		else  /*** e->b == 1.0 ***/
		{
			yl = e->c - e->a * p->x ;
			t1 = p->y - yl ;
			t2 = p->x - topsite->coord.x ;
			t3 = yl - topsite->coord.y ;
			above = ((t1*t1) > ((t2 * t2) + (t3 * t3))) ;
		}
		return (el->ELpm == LEDGE ? above : !above) ;
	}

	void endpoint(Edge * e, int lr, Site * s)
	{
		e->ep[lr] = s ;
		incref(s) ;
		if (e->ep[REDGE-lr] == (Site *)NULL)
		{
			return ;
		}
		out_ep(e) ;
		decref(e->reg[LEDGE]) ;
		decref(e->reg[REDGE]) ;
		makefree((Freenode *)e, (Freelist *) &efl) ;
	}

	float dist(Site * s, Site * t)
	{
		float dx,dy ;

		dx = s->coord.x - t->coord.x ;
		dy = s->coord.y - t->coord.y ;
		return (sqrt(dx*dx + dy*dy)) ;
	}

	void makevertex(Site * v)
	{
		v->sitenbr = nvertices++ ;
		out_vertex(v) ;
	}

	void decref(Site * v)
	{
		if (--(v->refcnt) == 0 )
		{
			makefree((Freenode *)v, (Freelist *)&sfl) ;
		}
	}

	void
		incref(Site * v)
	{
		++(v->refcnt) ;
	}



	void PQueue::insert(Halfedge * he, Site * v, float offset)
	{
		Halfedge * last, * next ;

		he->vertex = v ;
		incref(v) ;
		he->ystar = v->coord.y + offset ;
		last = &PQhash[ bucket(he)] ;
		while ((next = last->PQnext) != (Halfedge *)NULL &&
			(he->ystar  > next->ystar  ||
			(he->ystar == next->ystar &&
			v->coord.x > next->vertex->coord.x)))
		{
			last = next ;
		}
		he->PQnext = last->PQnext ;
		last->PQnext = he ;
		PQcount++ ;
	}

	void  PQueue::remove(Halfedge * he)
	{
		Halfedge * last;

		if(he ->  vertex != (Site *) NULL)
		{
			last = &PQhash[mQueue.bucket(he)] ;
			while (last -> PQnext != he)
			{
				last = last->PQnext ;
			}
			last->PQnext = he->PQnext;
			PQcount-- ;
			decref(he->vertex) ;
			he->vertex = (Site *)NULL ;
		}
	}

	int PQueue::bucket(Halfedge * he)
	{
		int result ;

		if	    (he->ystar < ymin)  result = 0;
		else if (he->ystar >= ymax) result = PQhashsize-1;
		else 			result = (he->ystar - ymin)/deltay * PQhashsize;
		if (result < 0)
		{
			result = 0 ;
		}
		if (result >= PQhashsize)
		{
			result = PQhashsize-1 ;
		}
		if (result < PQmin)
		{
			PQmin = result ;
		}
		return (result);
	}

	int PQueue::empty()
	{
		return (PQcount == 0) ;
	}


	Point PQueue::min()
	{
		Point answer ;

		while (PQhash[PQmin].PQnext == (Halfedge *)NULL)
		{
			++PQmin ;
		}
		answer.x = PQhash[PQmin].PQnext->vertex->coord.x ;
		answer.y = PQhash[PQmin].PQnext->ystar ;
		return (answer) ;
	}

	Halfedge* PQueue::extractmin()
	{
		Halfedge * curr ;

		curr = PQhash[PQmin].PQnext ;
		PQhash[PQmin].PQnext = curr->PQnext ;
		PQcount-- ;
		return (curr) ;
	}

	void  PQueue::initialize()
	{
		int i ;

		PQcount = PQmin = 0 ;
		PQhashsize = 4 * sqrt_nsites ;
		PQhash = (Halfedge *)myalloc(PQhashsize * sizeof *PQhash) ;
		for (i = 0 ; i < PQhashsize; i++)
		{
			PQhash[i].PQnext = (Halfedge *)NULL ;
		}
	}

	extern int sqrt_nsites, siteidx ;

	void freeinit(Freelist * fl, int size)
	{
		fl->head = (Freenode *)NULL ;
		fl->nodesize = size ;
	}

	char *
		getfree(Freelist * fl)
	{
		int i ;
		Freenode * t ;
		if (fl->head == (Freenode *)NULL)
		{
			t =  (Freenode *) myalloc(sqrt_nsites * fl->nodesize) ;
			for(i = 0 ; i < sqrt_nsites ; i++)
			{
				makefree((Freenode *)((char *)t+i*fl->nodesize), fl) ;
			}
		}
		t = fl->head ;
		fl->head = (fl->head)->nextfree ;
		return ((char *)t) ;
	}

	void
		makefree(Freenode * curr, Freelist * fl)
	{
		curr->nextfree = fl->head ;
		fl->head = curr ;
	}

	int total_alloc ;

	char *
		myalloc(unsigned n)
	{
		char * t ;
		if ((t=(char*)malloc(n)) == 0)
		{
			fprintf(stderr,"Insufficient memory processing site %d (%d bytes in use)\n",
				siteidx, total_alloc) ;
			exit(0) ;
		}
		total_alloc += n ;
		return (t) ;
	}

	extern int triangulate, plot, debug ;
	extern float ymax, ymin, xmax, xmin ;

	float pxmin, pxmax, pymin, pymax, cradius;

	void
		openpl(void)
	{
	}

#pragma argsused
	void
		line(float ax, float ay, float bx, float by)
	{
	}

#pragma argsused
	void
		circle(float ax, float ay, float radius)
	{
	}

#pragma argsused
	void
		range(float pxmin, float pxmax, float pymin, float pymax)
	{
	}

	void
		out_bisector(Edge * e)
	{
		if (triangulate && plot && !debug)
		{
			line(e->reg[0]->coord.x, e->reg[0]->coord.y,
				e->reg[1]->coord.x, e->reg[1]->coord.y) ;
		}
		if (!triangulate && !plot && !debug)
		{
			printf("l %f %f %f\n", e->a, e->b, e->c) ;
		}
		if (debug)
		{
			printf("line(%d) %gx+%gy=%g, bisecting %d %d\n", e->edgenbr,
				e->a, e->b, e->c, e->reg[LEDGE]->sitenbr, e->reg[REDGE]->sitenbr) ;
		}
	}

	void
		out_ep(Edge * e)
	{
		if (!triangulate && plot)
		{
			clip_line(e) ;
		}
		if (!triangulate && !plot)
		{
			printf("e %d", e->edgenbr);
			printf(" %d ", e->ep[LEDGE] != (Site *)NULL ? e->ep[LEDGE]->sitenbr : -1) ;
			printf("%d\n", e->ep[REDGE] != (Site *)NULL ? e->ep[REDGE]->sitenbr : -1) ;
		}
	}

	void
		out_vertex(Site * v)
	{
		if (!triangulate && !plot && !debug)
		{
			printf ("v %f %f\n", v->coord.x, v->coord.y) ;
		}
		if (debug)
		{
			printf("vertex(%d) at %f %f\n", v->sitenbr, v->coord.x, v->coord.y) ;
		}
	}

	void
		out_site(Site * s)
	{
		if (!triangulate && plot && !debug)
		{
			circle (s->coord.x, s->coord.y, cradius) ;
		}
		if (!triangulate && !plot && !debug)
		{
			printf("s %f %f\n", s->coord.x, s->coord.y) ;
		}
		if (debug)
		{
			printf("site (%d) at %f %f\n", s->sitenbr, s->coord.x, s->coord.y) ;
		}
	}

	void
		out_triple(Site * s1, Site * s2, Site * s3)
	{
		if (triangulate && !plot && !debug)
		{
			printf("%d %d %d\n", s1->sitenbr, s2->sitenbr, s3->sitenbr) ;
		}
		if (debug)
		{
			printf("circle through left=%d right=%d bottom=%d\n",
				s1->sitenbr, s2->sitenbr, s3->sitenbr) ;
		}
	}

	void
		plotinit(void)
	{
		float dx, dy, d ;

		dy = ymax - ymin ;
		dx = xmax - xmin ;
		d = ( dx > dy ? dx : dy) * 1.1 ;
		pxmin = xmin - (d-dx) / 2.0 ;
		pxmax = xmax + (d-dx) / 2.0 ;
		pymin = ymin - (d-dy) / 2.0 ;
		pymax = ymax + (d-dy) / 2.0 ;
		cradius = (pxmax - pxmin) / 350.0 ;
		openpl() ;
		range(pxmin, pymin, pxmax, pymax) ;
	}

	void
		clip_line(Edge * e)
	{
		Site * s1, * s2 ;
		float x1, x2, y1, y2 ;

		if (e->a == 1.0 && e->b >= 0.0)
		{
			s1 = e->ep[1] ;
			s2 = e->ep[0] ;
		}
		else
		{
			s1 = e->ep[0] ;
			s2 = e->ep[1] ;
		}
		if (e->a == 1.0)
		{
			y1 = pymin ;
			if (s1 != (Site *)NULL && s1->coord.y > pymin)
			{
				y1 = s1->coord.y ;
			}
			if (y1 > pymax)
			{
				return ;
			}
			x1 = e->c - e->b * y1 ;
			y2 = pymax ;
			if (s2 != (Site *)NULL && s2->coord.y < pymax)
			{
				y2 = s2->coord.y ;
			}
			if (y2 < pymin)
			{
				return ;
			}
			x2 = e->c - e->b * y2 ;
			if (((x1 > pxmax) && (x2 > pxmax)) || ((x1 < pxmin) && (x2 < pxmin)))
			{
				return ;
			}
			if (x1 > pxmax)
			{
				x1 = pxmax ;
				y1 = (e->c - x1) / e->b ;
			}
			if (x1 < pxmin)
			{
				x1 = pxmin ;
				y1 = (e->c - x1) / e->b ;
			}
			if (x2 > pxmax)
			{
				x2 = pxmax ;
				y2 = (e->c - x2) / e->b ;
			}
			if (x2 < pxmin)
			{
				x2 = pxmin ;
				y2 = (e->c - x2) / e->b ;
			}
		}
		else
		{
			x1 = pxmin ;
			if (s1 != (Site *)NULL && s1->coord.x > pxmin)
			{
				x1 = s1->coord.x ;
			}
			if (x1 > pxmax)
			{
				return ;
			}
			y1 = e->c - e->a * x1 ;
			x2 = pxmax ;
			if (s2 != (Site *)NULL && s2->coord.x < pxmax)
			{
				x2 = s2->coord.x ;
			}
			if (x2 < pxmin)
			{
				return ;
			}
			y2 = e->c - e->a * x2 ;
			if (((y1 > pymax) && (y2 > pymax)) || ((y1 < pymin) && (y2 <pymin)))
			{
				return ;
			}
			if (y1> pymax)
			{
				y1 = pymax ;
				x1 = (e->c - y1) / e->a ;
			}
			if (y1 < pymin)
			{
				y1 = pymin ;
				x1 = (e->c - y1) / e->a ;
			}
			if (y2 > pymax)
			{
				y2 = pymax ;
				x2 = (e->c - y2) / e->a ;
			}
			if (y2 < pymin)
			{
				y2 = pymin ;
				x2 = (e->c - y2) / e->a ;
			}
		}
		line(x1,y1,x2,y2);
	}


	extern Site * bottomsite ;

	/*** implicit parameters: nsites, sqrt_nsites, xmin, xmax, ymin, ymax,
	: deltax, deltay (can all be estimates).
	: Performance suffers if they are wrong; better to make nsites,
	: deltax, and deltay too big than too small.  (?)
	***/

	void voronoi(Site *(*nextsite)(void))
	{
		Site * newsite, * bot, * top, * temp, * p, * v ;
		Point newintstar ;
		int pm ;
		Halfedge * lbnd, * rbnd, * llbnd, * rrbnd, * bisector ;
		Edge * e ;

		mQueue.initialize() ;
		bottomsite = (*nextsite)() ;
		out_site(bottomsite) ;
		mEL.initialize() ;
		newsite = (*nextsite)() ;
		while (1)
		{
			if(!mQueue.empty())
			{
				newintstar = mQueue.min() ;
			}
			if (newsite != (Site *)NULL && (mQueue.empty()
				|| newsite -> coord.y < newintstar.y
				|| (newsite->coord.y == newintstar.y
				&& newsite->coord.x < newintstar.x))) {/* new site is
													   smallest */
					{
						out_site(newsite) ;
					}
					lbnd = mEL.leftbnd(&(newsite->coord)) ;
					rbnd = ELright(lbnd) ;
					bot = rightreg(lbnd) ;
					e = bisect(bot, newsite) ;
					bisector = mEL.create(e, LEDGE) ;
					mEL.insert(lbnd, bisector) ;
					p = intersect(lbnd, bisector) ;
					if (p != (Site *)NULL)
					{
						mQueue.remove(lbnd) ;
						mQueue.insert(lbnd, p, dist(p,newsite)) ;
					}
					lbnd = bisector ;
					bisector = mEL.create(e, REDGE) ;
					mEL.insert(lbnd, bisector) ;
					p = intersect(bisector, rbnd) ;
					if (p != (Site *)NULL)
					{
						mQueue.insert(bisector, p, dist(p,newsite)) ;
					}
					newsite = (*nextsite)() ;
			}
			else if (!mQueue.empty())   /* intersection is smallest */
			{
				lbnd = mQueue.extractmin() ;
				llbnd = ELleft(lbnd) ;
				rbnd = ELright(lbnd) ;
				rrbnd = ELright(rbnd) ;
				bot = leftreg(lbnd) ;
				top = rightreg(rbnd) ;
				out_triple(bot, top, rightreg(lbnd)) ;
				v = lbnd->vertex ;
				makevertex(v) ;
				endpoint(lbnd->ELedge, lbnd->ELpm, v);
				endpoint(rbnd->ELedge, rbnd->ELpm, v) ;
				mEL.remove(lbnd) ;
				mQueue.remove(rbnd) ;
				mEL.remove(rbnd) ;
				pm = LEDGE ;
				if (bot->coord.y > top->coord.y)
				{
					temp = bot ;
					bot = top ;
					top = temp ;
					pm = REDGE ;
				}
				e = bisect(bot, top) ;
				bisector = mEL.create(e, pm) ;
				mEL.insert(llbnd, bisector) ;
				endpoint(e, REDGE-pm, v) ;
				decref(v) ;
				p = intersect(llbnd, bisector) ;
				if (p  != (Site *) NULL)
				{
					mQueue.remove(llbnd) ;
					mQueue.insert(llbnd, p, dist(p,bot)) ;
				}
				p = intersect(bisector, rrbnd) ;
				if (p != (Site *) NULL)
				{
					mQueue.insert(bisector, p, dist(p,bot)) ;
				}
			}
			else
			{
				break ;
			}
		}

		for( lbnd = ELright( mEL.ELleftend ) ;
			lbnd != mEL.ELrightend ;
			lbnd = ELright(lbnd))
		{
			e = lbnd->ELedge ;
			out_ep(e) ;
		}
	}



	Site * readone(void), * nextone(void) ;
	void readsites(void) ;

	int sorted, triangulate, plot, debug, nsites, siteidx ;
	float xmin, xmax, ymin, ymax ;
	Site * sites ;
	Freelist sfl ;

	int main(int argc, char *argv[])
	{
		int c ;
		Site *(*next)() ;

		sorted = triangulate = plot = debug = 0 ;
#if 0
		while ((c = getopt(argc, argv, "dpst")) != EOF)
		{
			switch(c)
			{
			case 'd':
				debug = 1 ;
				break ;
			case 's':
				sorted = 1 ;
				break ;
			case 't':
				triangulate = 1 ;
				break ;
			case 'p':
				plot = 1 ;
				break ;
			}
		}
#endif
		
		freeinit(&sfl, sizeof(Site)) ;
		if (sorted)
		{
			scanf("%d %f %f %f %f", &nsites, &xmin, &xmax, &ymin, &ymax) ;
			next = readone ;
		}
		else
		{
			readsites() ;
			next = nextone ;
		}
		siteidx = 0 ;
		geominit() ;
		if (plot)
		{
			plotinit() ;
		}
		voronoi(next) ;
		return (0) ;
	}

	/*** sort sites on y, then x, coord ***/

	int scomp(const void * vs1, const void * vs2)
	{
		Point * s1 = (Point *)vs1 ;
		Point * s2 = (Point *)vs2 ;

		if (s1->y < s2->y)
		{
			return (-1) ;
		}
		if (s1->y > s2->y)
		{
			return (1) ;
		}
		if (s1->x < s2->x)
		{
			return (-1) ;
		}
		if (s1->x > s2->x)
		{
			return (1) ;
		}
		return (0) ;
	}

	/*** return a single in-storage site ***/

	Site * nextone(void)
	{
		Site * s ;

		if (siteidx < nsites)
		{
			s = &sites[siteidx++];
			return (s) ;
		}
		else
		{
			return ((Site *)NULL) ;
		}
	}

	/*** read all sites, sort, and compute xmin, xmax, ymin, ymax ***/

	void readsites(void)
	{
		int i ;

		nsites = 0 ;
		sites = (Site *) myalloc(4000 * sizeof(Site));
		while (scanf("%f %f", &sites[nsites].coord.x,
			&sites[nsites].coord.y) !=EOF)
		{
			sites[nsites].sitenbr = nsites ;
			sites[nsites++].refcnt = 0 ;
			if (nsites % 4000 == 0)
				sites = (Site *)
				realloc(sites,(nsites+4000)*sizeof(Site));
		}

		qsort((void *)sites, nsites, sizeof(Site), scomp) ;
		xmin = sites[0].coord.x ;
		xmax = sites[0].coord.x ;
		for (i = 1 ; i < nsites ; ++i)
		{
			if(sites[i].coord.x < xmin)
			{
				xmin = sites[i].coord.x ;
			}
			if (sites[i].coord.x > xmax)
			{
				xmax = sites[i].coord.x ;
			}
		}
		ymin = sites[0].coord.y ;
		ymax = sites[nsites-1].coord.y ;
	}

	/*** read one site ***/

	Site * readone(void)
	{
		Site * s ;

		s = (Site *)getfree(&sfl) ;
		s->refcnt = 0 ;
		s->sitenbr = siteidx++ ;
		if (scanf("%f %f", &(s->coord.x), &(s->coord.y)) == EOF)
		{
			return ((Site *)NULL ) ;
		}
		return (s) ;
	}


}//namespace Vor2