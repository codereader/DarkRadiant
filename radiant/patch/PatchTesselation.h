#pragma once

#include "render.h"
#include "PatchBezier.h"

/// Representation of a patch as mesh geometry
struct PatchTesselation
{
public:

	std::vector<ArbitraryMeshVertex> vertices;
	std::vector<RenderIndex> indices;

	std::size_t m_numStrips;
	std::size_t m_lenStrips;

	std::vector<std::size_t> arrayWidth;
	std::size_t m_nArrayWidth;
	std::vector<std::size_t> arrayHeight;
	std::size_t m_nArrayHeight;

	std::vector<BezierCurveTree*> curveTreeU;
	std::vector<BezierCurveTree*> curveTreeV;

public:

    /// Construct an uninitialised patch tesselation
	PatchTesselation() :
		m_numStrips(0),
		m_lenStrips(0),
		m_nArrayWidth(0),
		m_nArrayHeight(0)
	{}

    /// Clear all patch data
    void clear();
};
