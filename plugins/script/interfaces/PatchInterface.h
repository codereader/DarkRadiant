#pragma once

#include "iscript.h"

#include "SceneGraphInterface.h"

namespace script
{

class ScriptPatchNode :
	public ScriptSceneNode
{
private:
	static const std::string _emptyShader;
	static PatchControl _emptyPatchControl;
public:
	ScriptPatchNode(const scene::INodePtr& node);

	// Checks if the given SceneNode structure is a PatchNode
	static bool isPatch(const ScriptSceneNode& node);

	// "Cast" service for Python, returns a ScriptPatchNode.
	// The returned node is non-NULL if the cast succeeded
	static ScriptPatchNode getPatch(const ScriptSceneNode& node);

	// Resizes the patch to the given dimensions
	void setDims(std::size_t width, std::size_t height);

	// Get the patch dimensions
	std::size_t getWidth() const;
	std::size_t getHeight() const;

	PatchMesh getTesselatedPatchMesh() const;

	// Return a defined patch control vertex at <row>,<col>
	PatchControl& ctrlAt(std::size_t row, std::size_t col);

	void insertColumns(std::size_t colIndex);
	void insertRows(std::size_t rowIndex);

	void removePoints(int columns, std::size_t index);
	void appendPoints(int columns, int beginning);

	// Check if the patch has invalid control points or width/height are zero
	bool isValid() const;

	// Check whether all control vertices are in the same 3D spot (with minimal tolerance)
	bool isDegenerate() const;

	void controlPointsChanged();

	// Shader handling
	const std::string& getShader() const;
	void setShader(const std::string& name);

	bool hasVisibleMaterial();

	bool subdivisionsFixed() const;
	Subdivisions getSubdivisions() const;
	void setFixedSubdivisions(int isFixed, const Subdivisions& divisions);
};

class PatchInterface :
	public IScriptInterface
{
public:
	ScriptSceneNode createPatchDef2();
	ScriptSceneNode createPatchDef3();

	// IScriptInterface implementation
	void registerInterface(py::module& scope, py::dict& globals) override;
};

} // namespace script
