#pragma once

#include "ProcFile.h"

namespace map
{

class OptIsland
{
private:
	ProcFilePtr _procFile;

	ProcOptimizeGroup& _group;

	OptVertex*	_verts;
	OptEdge*	_edges;

	typedef std::vector<OptTriPtr> Tris;
	Tris		_tris;

	struct EdgeLength
	{
		OptVertex*	v1;
		OptVertex*	v2;
		float		length;
	};

	std::vector<OptVertex>& _optVerts;
	std::vector<OptEdge>& _optEdges;

public:
	OptIsland(ProcOptimizeGroup& group, 
			  std::vector<OptVertex>& vertices, 
			  std::vector<OptEdge>& edges,
			  const ProcFilePtr& procFile);

	// At this point, all needed vertexes are already in the list, 
	// including any that were added at crossing points.
	// Interior and colinear vertexes will be removed, and 
	// a new triangulation will be created.
	void optimise();

private:
	void linkVerts();
	void linkEdges();

	void unlinkEdge(OptEdge& e);
	void removeEdgeFromVert(OptEdge& e1, OptVertex* vert);

	// Add all possible edges between the verts
	void addInteriorEdges();

	// Edges that have triangles of the same type (filled / empty)
	// on both sides will be removed
	void removeInteriorEdges();

	static int LengthSort(const void* a, const void* b);

	bool tryAddNewEdge(OptVertex* v1, OptVertex* v2);

	// Generate a new list of triangles from the optEdeges
	void buildOptTriangles();
	void createOptTri(OptVertex* first, OptEdge* e1, OptEdge* e2);

	// Tests if a 2D point is inside an original triangle
	bool pointInTri(const Vector3& p, const ProcTri& tri);

	void linkTriToEdge(OptTri& optTri, OptEdge& edge);

	void validateEdgeCounts();

	void combineCollinearEdges();
	void removeIfColinear(OptVertex* ov);

	// Unlink any verts with no edges, so they won't be used in the retriangulation
	void cullUnusedVerts();

	// Add new triangles to the group's regeneratedTris
	void regenerateTriangles();
};

} // namespace 
