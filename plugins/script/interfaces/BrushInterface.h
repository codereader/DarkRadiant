#pragma once

#include "iscript.h"
#include "ibrush.h"

#include "SceneGraphInterface.h"

namespace script 
{

class ScriptFace
{
private:
	IFace* _face;
	static std::string _emptyShader;
	static IWinding _emptyWinding;

public:
	ScriptFace();

	ScriptFace(IFace& face);

	void undoSave();

	const std::string& getShader() const;

	void setShader(const std::string& name);

	void shiftTexdef(float s, float t);

	void scaleTexdef(float s, float t);

	void rotateTexdef(float angle);

	void fitTexture(float s_repeat, float t_repeat);

	void flipTexture(unsigned int flipAxis);

	void normaliseTexture();

	IWinding& getWinding();
};

class ScriptBrushNode :
	public ScriptSceneNode
{
public:
	ScriptBrushNode(const scene::INodePtr& node);

	std::size_t getNumFaces();

	// Get a reference to the face by index in [0..getNumFaces).
	ScriptFace getFace(std::size_t index);

	// Returns true when this brush has no faces
	bool empty() const;

	// Returns true if any face of the brush contributes to the final B-Rep.
	bool hasContributingFaces() const;

	// Removes faces that do not contribute to the brush.
	// This is useful for cleaning up after CSG operations on the brush.
	// Note: removal of empty faces is not performed during direct brush manipulations,
	// because it would make a manipulation irreversible if it created an empty face.
	void removeEmptyFaces();

	// Sets the shader of all faces to the given name
	void setShader(const std::string& newShader);

	// Returns TRUE if any of the faces has the given shader
	bool hasShader(const std::string& name);

	bool hasVisibleMaterial();

	enum DetailFlag
	{
		Structural = 0,
		Detail = 1 << 27,
	};

	DetailFlag getDetailFlag();

	void setDetailFlag(DetailFlag detailFlag);

	// Saves the current state to the undo stack.
	// Call this before manipulating the brush to make your action undo-able.
	void undoSave();

	// Checks if the given SceneNode structure is a BrushNode
	static bool isBrush(const ScriptSceneNode& node);

	// "Cast" service for Python, returns a ScriptBrushNode.
	// The returned node is non-NULL if the cast succeeded
	static ScriptBrushNode getBrush(const ScriptSceneNode& node);
};

class BrushInterface :
	public IScriptInterface
{
public:
	ScriptSceneNode createBrush();

	// IScriptInterface implementation
	void registerInterface(py::module& scope, py::dict& globals) override;
};

} // namespace script
