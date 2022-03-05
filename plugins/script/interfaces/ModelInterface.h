#pragma once

#include "iscript.h"
#include "iscriptinterface.h"

#include "imodel.h"
#include "SceneGraphInterface.h"

class MeshVertex;
namespace model { struct ModelPolygon; }

namespace script
{

// Wrapper around a IModelSurface reference
class ScriptModelSurface
{
	const model::IModelSurface& _surface;
public:
	ScriptModelSurface(const model::IModelSurface& surface) :
		_surface(surface)
	{}

	int getNumVertices() const;
	int getNumTriangles() const;
	const MeshVertex& getVertex(int vertexIndex) const;
	model::ModelPolygon getPolygon(int polygonIndex) const;
	std::string getDefaultMaterial() const;
	std::string getActiveMaterial() const;
};

class ScriptModelNode :
	public ScriptSceneNode
{
public:
	// Constructor, checks if the passed node is actually a model
	ScriptModelNode(const scene::INodePtr& node);

	// Proxy methods
	std::string getFilename();
	std::string getModelPath();
	//void applySkin(const ModelSkin& skin);
	int getSurfaceCount();
	int getVertexCount();
	int getPolyCount();
	model::StringList getActiveMaterials();
	ScriptModelSurface getSurface(int surfaceNum);

	// Checks if the given SceneNode structure is a ModelNode
	static bool isModel(const ScriptSceneNode& node);

	// "Cast" service for Python, returns a ScriptModelNode.
	// The returned node is non-NULL if the cast succeeded
	static ScriptModelNode getModel(const ScriptSceneNode& node);
};

/**
 * greebo: This class registers the model interface with the
 * scripting system.
 */
class ModelInterface :
	public IScriptInterface
{
public:
	// IScriptInterface implementation
	void registerInterface(py::module& scope, py::dict& globals) override;
};

} // namespace script
