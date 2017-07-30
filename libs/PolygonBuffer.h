#pragma once

#include <memory>
#include <string>
#include "imodelsurface.h"

namespace model
{

/**
 * A helper class used for exporting map geometry to static meshes.
 * This class collects polygons (vertices + indices) and adds them
 * to the internal buffer, adjusting the indices along the way.
 *
 * It implements the IModelSurface interface such that it can be 
 * passed as argument to a IModelExporter implementation.
 */
class PolygonBuffer :
	public IModelSurface
{
private:
	std::string _materialName;

public:
	typedef std::shared_ptr<PolygonBuffer> Ptr;

	PolygonBuffer(const std::string& materialName) :
		_materialName(materialName)
	{

	}



	// IModelSurface implementation
	int getNumVertices() const override
	{
		return 0;
	}

	int getNumTriangles() const override
	{
		return 0;
	}

	const ArbitraryMeshVertex& getVertex(int vertexIndex) const override
	{
		throw std::runtime_error("Not implemented");
	}

	ModelPolygon getPolygon(int polygonIndex) const override
	{
		return ModelPolygon();
	}

	const std::string& getDefaultMaterial() const override
	{
		return _materialName;
	}
};

}
