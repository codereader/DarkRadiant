#pragma once

#include "render.h"
#include "PatchControl.h"

struct FaceTangents;

/// Representation of a patch as mesh geometry
class PatchTesselation
{
public:
	// The vertex data, each vertex equipped with texcoord and ntb vectors
	std::vector<ArbitraryMeshVertex> vertices;

	// The indices, arranged in the way it's expected by GL_QUAD_STRIPS
	// The number of indices is (lenStrips*numStrips), which is the same as (width*height)
	std::vector<RenderIndex> indices;

	// Strip index layout
	std::size_t numStrips;
	std::size_t lenStrips;

	// Geometry of the tesselated mesh
	std::size_t width;
	std::size_t height;

private:
	// Used during the tesselation phase
	std::size_t _maxWidth;
	std::size_t _maxHeight;

public:

    /// Construct an uninitialised patch tesselation
	PatchTesselation() :
		numStrips(0),
		lenStrips(0),
		width(0),
		height(0),
		_maxWidth(0),
		_maxHeight(0)
	{}

    /// Clear all patch data
    void clear();

	// Generates the tesselated mesh based on the input parameters
	void generate(std::size_t width, std::size_t height, const PatchControlArray& controlPoints, 
		bool subdivionsFixed, const Subdivisions& subdivs, IRenderEntity* renderEntity);

private:
	// Private methods used for tesselation, modeled after the patch subdivision code found in idTech4
	void generateIndices();
	void generateNormals();
	void subdivideMesh();
	void subdivideMeshFixed(std::size_t subdivX, std::size_t subdivY);
	void collapseMesh();
	void expandMesh();
    void resizeExpandedMesh(std::size_t newHeight, std::size_t newWidth);
	void putOnCurve();
	void removeLinearColumnsRows();

	static void lerpVert(const ArbitraryMeshVertex& a, const ArbitraryMeshVertex& b, ArbitraryMeshVertex&out);
	static Vector3 projectPointOntoVector(const Vector3& point, const Vector3& vStart, const Vector3& vEnd);

	void sampleSinglePatch(const ArbitraryMeshVertex ctrl[3][3], std::size_t baseCol, std::size_t baseRow, 
		std::size_t width, std::size_t horzSub, std::size_t vertSub, 
		std::vector<ArbitraryMeshVertex>& outVerts) const;
	void sampleSinglePatchPoint(const ArbitraryMeshVertex ctrl[3][3], float u, float v, ArbitraryMeshVertex& out) const;
	void deriveTangents();
	void deriveFaceTangents(std::vector<FaceTangents>& faceTangents);
};
