#pragma once

#include <fstream>
#include <map>
#include <fmt/format.h>

#include "i18n.h"
#include "imodel.h"
#include "imodelsurface.h"

#include "render.h"
#include "math/Matrix4.h"
#include "os/fs.h"
#include "os/path.h"

#include "stream/ExportStream.h"

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
		std::vector<MeshVertex> vertices;

		// The indices connecting the vertices to triangles
		IndexBuffer indices;
	};

	typedef std::map<std::string, Surface> Surfaces;
	Surfaces _surfaces;

public:
	// Adds the given Surface to the exporter's queue
	void addSurface(const IModelSurface& incoming, const Matrix4& localToWorld) override
	{
		Surface& surface = ensureSurface(incoming.getActiveMaterial());

		Matrix4 invTranspTransform = localToWorld.getFullInverse().getTransposed();

		try
		{
			const IIndexedModelSurface& indexedSurf = dynamic_cast<const IIndexedModelSurface&>(incoming);

			// Cast succeeded, load the vertices and indices directly into here
			unsigned int indexStart = static_cast<unsigned int>(surface.vertices.size());
			
			const auto& vertices = indexedSurf.getVertexArray();
			const auto& indices = indexedSurf.getIndexArray();

			if (indices.size() < 3)
			{
				// Reject this index buffer
				rError() << "Rejecting model surface with less than 3 indices." << std::endl;
				return;
			}

			// Transform vertices before inserting them
			for (const auto& meshVertex : vertices)
			{
				// Copy-construct based on the incoming meshVertex, transform the vertex.
                // Transform the normal using the inverse transpose
                // We discard the tangent and bitangent vectors here, none of the exporters is using them.
                surface.vertices.emplace_back(
                    localToWorld.transformPoint(meshVertex.vertex),
                    invTranspTransform.transformPoint(meshVertex.normal).getNormalised(),
                    meshVertex.texcoord,
                    meshVertex.colour);
			}
			
			surface.indices.reserve(surface.indices.size() + indices.size());

			// Incoming polygons are defined in clockwise windings, so reverse the indices
			// as the exporter code expects them to be counter-clockwise.
			for (std::size_t i = 0; i < indices.size() - 2; i += 3)
			{
				surface.indices.push_back(indices[i + 2] + indexStart);
				surface.indices.push_back(indices[i + 1] + indexStart);
				surface.indices.push_back(indices[i + 0] + indexStart);
			}

			return;
		}
		catch (std::bad_cast&)
		{
			// Not an indexed surface, fall through
		}

		// Pull in all the triangles of that mesh
		for (int i = 0; i < incoming.getNumTriangles(); ++i)
		{
			ModelPolygon poly = incoming.getPolygon(i);

			unsigned int indexStart = static_cast<unsigned int>(surface.vertices.size());

			poly.a.vertex = localToWorld.transformPoint(poly.a.vertex);
			poly.b.vertex = localToWorld.transformPoint(poly.b.vertex);
			poly.c.vertex = localToWorld.transformPoint(poly.c.vertex);

			// Transform the normal using the inverse transpose
			poly.a.normal = invTranspTransform.transformPoint(poly.a.normal).getNormalised();
			poly.b.normal = invTranspTransform.transformPoint(poly.b.normal).getNormalised();
			poly.c.normal = invTranspTransform.transformPoint(poly.c.normal).getNormalised();

			surface.vertices.push_back(poly.a);
			surface.vertices.push_back(poly.b);
			surface.vertices.push_back(poly.c);

			surface.indices.push_back(indexStart);
			surface.indices.push_back(indexStart + 1);
			surface.indices.push_back(indexStart + 2);
		}
	}

	void addPolygons(const std::string& materialName,
		const std::vector<ModelPolygon>& polys, const Matrix4& localToWorld) override
	{
		Surface& surface = ensureSurface(materialName);

		for (const ModelPolygon& poly : polys)
		{
			unsigned int indexStart = static_cast<unsigned int>(surface.vertices.size());

			ModelPolygon transformed(poly); // copy to transform

			transformed.a.vertex = localToWorld.transformPoint(poly.a.vertex);
			transformed.b.vertex = localToWorld.transformPoint(poly.b.vertex);
			transformed.c.vertex = localToWorld.transformPoint(poly.c.vertex);

			surface.vertices.push_back(transformed.a);
			surface.vertices.push_back(transformed.b);
			surface.vertices.push_back(transformed.c);

			surface.indices.push_back(indexStart);
			surface.indices.push_back(indexStart + 1);
			surface.indices.push_back(indexStart + 2);
		}
	}

private:
	Surface& ensureSurface(const std::string& materialName)
	{
		Surfaces::iterator surf = _surfaces.find(materialName);

		if (surf == _surfaces.end())
		{
			surf = _surfaces.insert(std::make_pair(materialName, Surface())).first;
			surf->second.materialName = materialName;
		}

		return surf->second;
	}
};

}