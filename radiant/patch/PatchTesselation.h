#pragma once

#include "render.h"
#include "PatchBezier.h"
#include "PatchControl.h"

struct FaceTangents;

/// Representation of a patch as mesh geometry
class PatchTesselation
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

private:
	std::size_t _maxWidth;
	std::size_t _maxHeight;

public:

    /// Construct an uninitialised patch tesselation
	PatchTesselation() :
		m_numStrips(0),
		m_lenStrips(0),
		m_nArrayWidth(0),
		m_nArrayHeight(0),
		_maxWidth(0),
		_maxHeight(0)
	{}

    /// Clear all patch data
    void clear();

	// Generates the tesselated mesh based on the input parameters
	void generate(std::size_t width, std::size_t height, const PatchControlArray& controlPoints, 
		bool subdivionsFixed, const Subdivisions& subdivs);

private:
	void generateIndices();
	void generateNormals();
	void subdivideMesh();
	void subdivideMeshFixed(std::size_t subdivX, std::size_t subdivY);
	void collapseMesh();
	void expandMesh();
	void resizeExpandedMesh(int newHeight, int newWidth);
	void lerpVert(const ArbitraryMeshVertex& a, const ArbitraryMeshVertex& b, ArbitraryMeshVertex&out) const;
	void putOnCurve();
	void removeLinearColumnsRows();
	Vector3 projectPointOntoVector(const Vector3& point, const Vector3& vStart, const Vector3& vEnd);

	void sampleSinglePatch(const ArbitraryMeshVertex ctrl[3][3], std::size_t baseCol, std::size_t baseRow, 
		std::size_t width, std::size_t horzSub, std::size_t vertSub, 
		std::vector<ArbitraryMeshVertex>& outVerts) const;
	void sampleSinglePatchPoint(const ArbitraryMeshVertex ctrl[3][3], float u, float v, ArbitraryMeshVertex& out) const;
	void deriveTangents();
	void deriveFaceTangents(std::vector<FaceTangents>& faceTangents);
};
