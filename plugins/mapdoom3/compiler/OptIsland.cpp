#include "OptIsland.h"

#include "OptUtils.h"

namespace map
{

OptIsland::OptIsland(ProcOptimizeGroup& group, 
					 std::vector<OptVertex>& vertices, 
					 std::vector<OptEdge>& edges) :
	_group(group),
	_verts(NULL),
	_edges(NULL),
	_tris(NULL),
	_optVerts(vertices),
	_optEdges(edges)
{
	linkVerts();
	linkEdges();
}

void OptIsland::linkVerts()
{
	// link everything together
	for (std::size_t i = 0; i < _optVerts.size(); ++i)
	{
		_optVerts[i].islandLink = _verts;
		_verts = &_optVerts[i];
	}
}

void OptIsland::linkEdges()
{
	for (std::size_t i = 0; i < _optEdges.size(); ++i)
	{
		_optEdges[i].islandLink = _edges;
		_edges = &_optEdges[i];
	}
}

void OptIsland::optimise()
{
	// add space-filling fake edges so we have a complete
	// triangulation of a convex hull before optimization
	addInteriorEdges();
	
	// determine all the possible triangles, and decide if
	// the are filled or empty
	/*BuildOptTriangles( island );

	// remove interior vertexes that have filled triangles
	// between all their edges
	RemoveInteriorEdges( island );
	DrawEdges( island );

	ValidateEdgeCounts( island );

	// remove vertexes that only have two colinear edges
	CombineColinearEdges( island );
	CullUnusedVerts( island );
	DrawEdges( island );

	// add new internal edges between the remaining exterior edges
	// to give us a full triangulation again
	AddInteriorEdges( island );
	DrawEdges( island );

	// determine all the possible triangles, and decide if
	// the are filled or empty
	BuildOptTriangles( island );

	// make mapTri_t out of the filled optTri_t
	RegenerateTriangles( island );*/
}

int OptIsland::LengthSort(const void *a, const void *b)
{
	const EdgeLength* ea = reinterpret_cast<const EdgeLength*>(a);
	const EdgeLength* eb = reinterpret_cast<const EdgeLength*>(b);

	if (ea->length < eb->length)
	{
		return -1;
	}

	if (ea->length > eb->length)
	{
		return 1;
	}

	return 0;
}

bool OptIsland::tryAddNewEdge(OptVertex* v1, OptVertex* v2)
{
	// if the new edge crosses any other edges, don't add it
	for (OptEdge* e = _edges; e; e = e->islandLink)
	{
		if (OptUtils::EdgesCross(e->v1, e->v2, v1, v2))
		{
			return false;
		}
	}

#if 0
	if ( dmapGlobals.drawflag ) {
		qglBegin( GL_LINES );
		qglColor3f( 0, ( 128 + orandom.RandomInt( 127 ) )/ 255.0, 0 );
		qglVertex3fv( v1->pv.ToFloatPtr() );
		qglVertex3fv( v2->pv.ToFloatPtr() );
		qglEnd();
		qglFlush();
	}
#endif

	// add it
	_optEdges.push_back(OptEdge());

	OptEdge& e = _optEdges.back();

	e.islandLink = _edges;
	_edges = &e;
	e.v1 = v1;
	e.v2 = v2;

	e.created = true;

	// link the edge to its verts
	e.linkToVertices();

	return true;
}

void OptIsland::addInteriorEdges()
{
	// count the verts
	std::size_t vertCount = 0;

	for (OptVertex* vert = _verts; vert; vert = vert->islandLink)
	{
		if (!vert->edges) continue;

		vertCount++;
	}

	// allocate space for all the lengths
	std::vector<EdgeLength> lengths(vertCount * vertCount / 2);

	std::size_t numLengths = 0;

	for (OptVertex* vert = _verts; vert; vert = vert->islandLink)
	{
		if (!vert->edges) continue;

		for (OptVertex* vert2 = vert->islandLink; vert2; vert2 = vert2->islandLink)
		{
			if (!vert2->edges) continue;

			lengths[numLengths].v1 = vert;
			lengths[numLengths].v2 = vert2;

			Vector3	dir = vert->pv - vert2->pv;

			lengths[numLengths].length = dir.getLength();
			numLengths++;
		}
	}

	// sort by length, shortest first
	qsort(&lengths[0], numLengths, sizeof(lengths[0]), LengthSort);

	// try to create them in that order
	std::size_t addedEdges = 0;

	for (std::size_t i = 0; i < numLengths; ++i)
	{
		if (tryAddNewEdge(lengths[i].v1, lengths[i].v2))
		{
			addedEdges++;
		}
	}

	if (false/* dmapGlobals.verbose */)
	{
		globalOutputStream() << (boost::format("%6i tested segments") % numLengths).str() << std::endl;
		globalOutputStream() << (boost::format("%6i added interior edges") % addedEdges).str() << std::endl;
	}
}

} // namespace
