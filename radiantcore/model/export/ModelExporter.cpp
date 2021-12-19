#include "ModelExporter.h"

#include "i18n.h"
#include "ibrush.h"
#include "iclipper.h" // for caulk shader registry key
#include "ilightnode.h"
#include "ipatch.h"
#include "itextstream.h"
#include "imodel.h"
#include "os/fs.h"
#include "entitylib.h"
#include "registry/registry.h"
#include <stdexcept>
#include <fstream>

#include "PatchSurface.h"

namespace model
{

namespace
{

// Adapter methods to convert brush vertices to ArbitraryMeshVertex type
ArbitraryMeshVertex convertWindingVertex(const WindingVertex& in)
{
	ArbitraryMeshVertex out;

	out.vertex = in.vertex;
	out.normal = in.normal;
	out.texcoord = in.texcoord;
	out.bitangent = in.bitangent;
	out.tangent = in.tangent;
	out.colour = Vector4(1.0, 1.0, 1.0, 1.0);

	return out;
}

// Create a polygon out of 3 vertices defined in counter-clockwise winding
// Only the normal will be calculated, texcoord, tangent and bitangents will be zero
model::ModelPolygon createPolyCCW(const Vertex3f& a, const Vertex3f& b, const Vertex3f& c)
{
	model::ModelPolygon poly;

	poly.a.vertex = a;
	poly.b.vertex = b;
	poly.c.vertex = c;

	// Calc normals for all three vertices
	poly.a.normal = poly.b.normal = poly.c.normal = (b-a).cross(c-a).getNormalised();

	return poly;
}

}

ModelExporter::ModelExporter(const model::IModelExporterPtr& exporter) :
	_exporter(exporter),
	_skipCaulk(false),
	_caulkMaterial(registry::getValue<std::string>(RKEY_CLIPPER_CAULK_SHADER)),
	_centerObjects(false),
	_origin(0,0,0),
	_useOriginAsCenter(false),
	_exportLightsAsObjects(false),
	_centerTransform(Matrix4::getIdentity())
{
	if (!_exporter)
	{
		rError() << "Cannot save out scaled models, no exporter found." << std::endl;
		return;
	}
}

void ModelExporter::setSkipCaulkMaterial(bool skipCaulk)
{
	_skipCaulk = skipCaulk;
}

void ModelExporter::setCenterObjects(bool centerObjects)
{
	_centerObjects = centerObjects;
}

void ModelExporter::setOrigin(const Vector3& origin)
{
	_origin = origin;
	_useOriginAsCenter = true;
}

void ModelExporter::setExportLightsAsObjects(bool enabled)
{
	_exportLightsAsObjects = enabled;
}

bool ModelExporter::pre(const scene::INodePtr& node)
{
	// Skip worldspawn
	if (Node_isWorldspawn(node)) return true;

	_nodes.push_back(node);

	return true;
}

const Matrix4& ModelExporter::getCenterTransform()
{
	return _centerTransform;
}

void ModelExporter::processNodes()
{
	AABB bounds = calculateModelBounds();

	if (_centerObjects)
	{
		// Depending on the center point, we need to use the object bounds
		// or just the translation towards the user-defined origin, ignoring bounds
		_centerTransform = _useOriginAsCenter ?
			Matrix4::getTranslation(-_origin) :
			Matrix4::getTranslation(-bounds.origin);
	}

	for (const scene::INodePtr& node : _nodes)
	{
		if (Node_isModel(node))
		{
			model::ModelNodePtr modelNode = Node_getModel(node);

			// Push the geometry into the exporter
			model::IModel& model = modelNode->getIModel();

			Matrix4 exportTransform = node->localToWorld().getPremultipliedBy(_centerTransform);

			for (int s = 0; s < model.getSurfaceCount(); ++s)
			{
				const model::IModelSurface& surface = model.getSurface(s);

				if (isExportableMaterial(surface.getActiveMaterial()))
				{
					_exporter->addSurface(surface, exportTransform);
				}
			}
		}
		else if (Node_isBrush(node))
		{
			processBrush(node);
		}
		else if (Node_isPatch(node))
		{
			processPatch(node);
		}
		else if (_exportLightsAsObjects && Node_getLightNode(node))
		{
			processLight(node);
		}
	}
}

AABB ModelExporter::calculateModelBounds()
{
	AABB bounds;

	for (const scene::INodePtr& node : _nodes)
	{
		// Only consider the node types supported by processNodes()
		if (!Node_isModel(node) && !Node_isBrush(node) && !Node_isPatch(node))
		{
			continue;
		}

		bounds.includeAABB(node->worldAABB());
	}

	return bounds;
}

void ModelExporter::processPatch(const scene::INodePtr& node)
{
	IPatch* patch = Node_getIPatch(node);

	if (patch == nullptr) return;

	const std::string& materialName = patch->getShader();

	if (!isExportableMaterial(materialName)) return;

	PatchMesh mesh = patch->getTesselatedPatchMesh();
    Matrix4 exportTransform = node->localToWorld().getPremultipliedBy(_centerTransform);

    // Convert the patch mesh to an indexed surface
    PatchSurface surface(materialName, mesh);
    _exporter->addSurface(surface, exportTransform);
}

void ModelExporter::processBrush(const scene::INodePtr& node)
{
	IBrush* brush = Node_getIBrush(node);

	if (brush == nullptr) return;

	Matrix4 exportTransform = node->localToWorld().getPremultipliedBy(_centerTransform);

	for (std::size_t b = 0; b < brush->getNumFaces(); ++b)
	{
		const IFace& face = brush->getFace(b);

		const std::string& materialName = face.getShader();

		if (!isExportableMaterial(materialName)) continue;

		const IWinding& winding = face.getWinding();

		std::vector<model::ModelPolygon> polys;

		if (winding.size() < 3)
		{
			rWarning() << "Skipping face with less than 3 winding verts" << std::endl;
			continue;
		}

		// Create triangles for this winding
		for (std::size_t i = 1; i < winding.size() - 1; ++i)
		{
			model::ModelPolygon poly;

			poly.a = convertWindingVertex(winding[i + 1]);
			poly.b = convertWindingVertex(winding[i]);
			poly.c = convertWindingVertex(winding[0]);

			polys.push_back(poly);
		}

		_exporter->addPolygons(materialName, polys, exportTransform);
	}
}

void ModelExporter::processLight(const scene::INodePtr& node)
{
	// Export lights as small polyhedron
	static const double EXTENTS = 8.0;
	std::vector<model::ModelPolygon> polys;

	Vertex3f up(0, 0, EXTENTS);
	Vertex3f down(0, 0, -EXTENTS);
	Vertex3f north(0, EXTENTS, 0);
	Vertex3f south(0, -EXTENTS, 0);
	Vertex3f east(EXTENTS, 0, 0);
	Vertex3f west(-EXTENTS, 0, 0);

	// Upper semi-diamond
	polys.push_back(createPolyCCW(up, south, east));
	polys.push_back(createPolyCCW(up, east, north));
	polys.push_back(createPolyCCW(up, north, west));
	polys.push_back(createPolyCCW(up, west, south));

	// Lower semi-diamond
	polys.push_back(createPolyCCW(down, south, west));
	polys.push_back(createPolyCCW(down, west, north));
	polys.push_back(createPolyCCW(down, north, east));
	polys.push_back(createPolyCCW(down, east, south));

	Matrix4 exportTransform = node->localToWorld().getPremultipliedBy(_centerTransform);

	_exporter->addPolygons("lights/default", polys, exportTransform);
}

bool ModelExporter::isExportableMaterial(const std::string& materialName)
{
	return !_skipCaulk || materialName != _caulkMaterial;
}

}
