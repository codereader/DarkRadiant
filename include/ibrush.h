#ifndef _IBRUSH_H_
#define _IBRUSH_H_

#include "inode.h"
#include "imodule.h"

const std::string RKEY_ENABLE_TEXTURE_LOCK("user/ui/brush/textureLock");

class BrushCreator :
	public RegisterableModule
{
public:
	virtual scene::INodePtr createBrush() = 0;
	
	// Call this when the clip plane colours should be updated.
	virtual void clipperColourChanged() = 0;
};

// Brush Interface
class IBrush
{
public:
    virtual ~IBrush() {}

	// Returns the number of faces for this brush
	virtual std::size_t getNumFaces() const = 0;

	// Returns true when this brush has no faces
	virtual bool empty() const = 0;

	// Returns true if any face of the brush contributes to the final B-Rep.
	virtual bool hasContributingFaces() const = 0;

	// Removes faces that do not contribute to the brush. 
	// This is useful for cleaning up after CSG operations on the brush.
	// Note: removal of empty faces is not performed during direct brush manipulations, 
	// because it would make a manipulation irreversible if it created an empty face.
	virtual void removeEmptyFaces() = 0;

	// Sets the shader of all faces to the given name
	virtual void setShader(const std::string& newShader) = 0;

	// Returns TRUE if any of the faces has the given shader
	virtual bool hasShader(const std::string& name) = 0;

	// Saves the current state to the undo stack.
	// Call this before manipulating the brush to make your action undo-able.
	virtual void undoSave() = 0;
};

// Forward-declare the Brush object, only accessible from main binary
class Brush;

class IBrushNode
{
public:
    virtual ~IBrushNode() {}
	/** greebo: Retrieves the contained Brush from the BrushNode
	 */
	virtual Brush& getBrush() = 0;

	// Returns the IBrush interface
	virtual IBrush& getIBrush() = 0;
};
typedef boost::shared_ptr<IBrushNode> IBrushNodePtr;

inline bool Node_isBrush(const scene::INodePtr& node)
{
	return boost::dynamic_pointer_cast<IBrushNode>(node) != NULL;
}

// Casts the node onto a BrushNode and returns the Brush pointer
inline Brush* Node_getBrush(const scene::INodePtr& node)
{
	IBrushNodePtr brushNode = boost::dynamic_pointer_cast<IBrushNode>(node);
	if (brushNode != NULL) {
		return &brushNode->getBrush();
	}
	return NULL;
}

// Casts the node onto a BrushNode and returns the IBrush pointer
inline IBrush* Node_getIBrush(const scene::INodePtr& node)
{
	IBrushNodePtr brushNode = boost::dynamic_pointer_cast<IBrushNode>(node);
	if (brushNode != NULL) {
		return &brushNode->getIBrush();
	}
	return NULL;
}

const std::string MODULE_BRUSHCREATOR("Doom3BrushCreator");

inline BrushCreator& GlobalBrushCreator()
{
	// Cache the reference locally
	static BrushCreator& _brushCreator(
		*boost::static_pointer_cast<BrushCreator>(
			module::GlobalModuleRegistry().getModule(MODULE_BRUSHCREATOR)
		)
	);
	return _brushCreator;
}

#endif /* _IBRUSH_H_ */
