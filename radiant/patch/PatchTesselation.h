#ifndef PATCHTESSELATION_H_
#define PATCHTESSELATION_H_

#include "render.h"
#include "PatchBezier.h"

/**
 * greebo: This is the structure that represents the tesselation of a patch.
 */
struct PatchTesselation
{
public:
	PatchTesselation() :
		m_numStrips(0),
		m_lenStrips(0),
		m_nArrayWidth(0),
		m_nArrayHeight(0)
	{}

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
};

#endif /*PATCHTESSELATION_H_*/
