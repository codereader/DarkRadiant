#pragma once

#include "ProcFile.h"

namespace map
{

class OptIsland
{
private:
	ProcOptimizeGroup& _group;

	OptVertex*	_verts;
	OptEdge*	_edges;
	OptTri*		_tris;

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
			  std::vector<OptEdge>& edges);

	// At this point, all needed vertexes are already in the list, 
	// including any that were added at crossing points.
	// Interior and colinear vertexes will be removed, and 
	// a new triangulation will be created.
	void optimise();

private:
	void linkVerts();
	void linkEdges();

	// Add all possible edges between the verts
	void addInteriorEdges();

	static int LengthSort(const void* a, const void* b);

	bool tryAddNewEdge(OptVertex* v1, OptVertex* v2);
};

} // namespace 
