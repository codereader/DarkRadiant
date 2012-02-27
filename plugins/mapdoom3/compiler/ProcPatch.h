#pragma once

#include "ipatch.h"
#include "render/ArbitraryMeshVertex.h"

namespace map
{

// greebo: This is a port of the idSurface_Patch class found in D3's idlib
class ProcPatch
{
private:
	const IPatch& _mapPatch;

	std::size_t			_width;			// width of patch
	std::size_t			_height;			// height of patch
	bool				_expanded;		// true if vertices are spaced out

	int					_horzSubdivisions;
	int					_vertSubdivisions;
	bool				_explicitSubdivisions;

	std::size_t			_maxWidth;			// maximum width allocated for
	std::size_t			_maxHeight;			// maximum height allocated for

	typedef std::vector<ArbitraryMeshVertex> Vertices;
	Vertices			_vertices;		// vertices

	typedef std::vector<int> Indices;
	Indices				_indices;		// 3 references to vertices for each triangle

public:
	ProcPatch(const IPatch& mapPatch);

	void subdivide(bool generateNormals);
	void subdivideExplicit(const Subdivisions& subdivisions, bool generateNormals);

	const ArbitraryMeshVertex& getVertex(std::size_t index) const
	{
		return _vertices[index];
	}

	std::size_t getNumIndices() const
	{
		return _indices.size();
	}

	int getIndex(std::size_t num) const
	{
		return _indices[num];
	}

private:
	void collapse();
	void expand();

	void resizeExpanded(std::size_t newWidth, std::size_t newHeight);

	void putOnCurve();
	void removeLinearColumnsRows();

	void generateNormals();
	void generateIndices();

	void sampleSinglePatch(const ArbitraryMeshVertex ctrl[3][3], 
							std::size_t baseCol, std::size_t baseRow, 
							std::size_t width, std::size_t horzSub, std::size_t vertSub, 
							std::vector<ArbitraryMeshVertex>& outVerts) const;

	void sampleSinglePatchPoint(const ArbitraryMeshVertex ctrl[3][3], 
								float u, float v, ArbitraryMeshVertex& out) const;
};

} // namespace
