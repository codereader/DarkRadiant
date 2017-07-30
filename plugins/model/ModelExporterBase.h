#pragma once

#include "imodel.h"
#include "imodelsurface.h"
#include <map>
#include "render.h"

namespace model
{

class ModelExporterBase :
	public IModelExporter
{
protected:
	struct Surface
	{
		std::string materialName;

		// The vertices of this surface
		std::vector<ArbitraryMeshVertex> vertices;

		// The indices connecting the vertices to triangles
		IndexBuffer indices;
	};

	typedef std::map<std::string, Surface> Surfaces;
	Surfaces _surfaces;

public:
	// Adds the given Surface to the exporter's queue
	void addSurface(const IModelSurface& incoming) override
	{
		const std::string& materialName = incoming.getDefaultMaterial();

		Surfaces::iterator surf = _surfaces.find(materialName);

		if (surf == _surfaces.end())
		{
			surf = _surfaces.insert(std::make_pair(incoming.getDefaultMaterial(), Surface())).first;
			surf->second.materialName = materialName;
		}

		Surface& surface = surf->second;

		// Pull in all the triangles of that mesh
		for (int i = 0; i < incoming.getNumTriangles(); ++i)
		{
			ModelPolygon poly = incoming.getPolygon(i);

			unsigned int indexStart = static_cast<unsigned int>(surface.vertices.size());

			surface.vertices.push_back(poly.a);
			surface.vertices.push_back(poly.b);
			surface.vertices.push_back(poly.c);

			surface.indices.push_back(indexStart);
			surface.indices.push_back(indexStart + 1);
			surface.indices.push_back(indexStart + 2);
		}
	}
};

}