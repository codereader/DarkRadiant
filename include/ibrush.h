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
