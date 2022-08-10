#pragma once

#include <stdexcept>
#include "iselection.h"
#include "ibrush.h"
#include "igroupnode.h"
#include "iselectiongroup.h"
#include "iscenegraph.h"
#include "ientity.h"
#include "ipatch.h"
#include "math/Vector3.h"
#include "math/AABB.h"

/** 
 * @brief A structure containing information about the current Selection.
 *
 * An instance of this is maintained by the RadiantSelectionSystem, and a const reference can be
 * retrieved via the according getSelectionInfo() method.
 */
class SelectionInfo
{
public:
    int totalCount = 0;     // number of selected items
    int patchCount = 0;     // number of selected patches
    int brushCount = 0;     // -- " -- brushes
    int entityCount = 0;    // -- " -- entities
    int componentCount = 0; // -- " -- components (faces, edges, vertices)

    // Zeroes all the counters
    void clear() { *this = SelectionInfo(); }
};

namespace selection
{

/**
 * The selection "WorkZone" defines the bounds of the most
 * recent selection. On each selection, the workzone is
 * recalculated, nothing happens on deselection.
 */
struct WorkZone
{
	// The corner points defining the selection workzone
	Vector3 min;
	Vector3 max;

	// The bounds of the selection workzone (equivalent to min/max)
	AABB bounds;

	WorkZone() :
		min(-64,-64,-64),
		max(64,64,64),
		bounds(AABB::createFromMinMax(min, max))
	{}
};

namespace detail
{

class AmbiguousShaderException :
	public std::runtime_error
{
public:
	// Constructor
	AmbiguousShaderException(const std::string& what) :
		std::runtime_error(what)
	{}
};

/**
 * greebo: Helper objects that compares each object passed
 * into it, throwing an AmbiguousShaderException as soon as
 * at least two different non-empty shader names are found.
 */
class UniqueShaderFinder
{
	// The string containing the result
	std::string _shader;

public:
	const std::string& getFoundShader() const
	{
		return _shader;
	}

	void checkFace(IFace& face)
	{
		checkShader(face.getShader());
	}

	void checkBrush(IBrush& brush)
	{
		for (std::size_t i = 0; i < brush.getNumFaces(); ++i)
		{
			checkFace(brush.getFace(i));
		}
	}

	void checkPatch(IPatch& patch)
	{
		checkShader(patch.getShader());
	}

private:
	void checkShader(const std::string& foundShader)
	{
		if (foundShader.empty())
		{
			return;
		}

		if (_shader.empty())
		{
			// No shader encountered yet, take this one
			_shader = foundShader;
		}
		else if (_shader != foundShader)
		{
			throw AmbiguousShaderException(foundShader);
		}

		// found shader is the same as _shader, all is well
	}
};

}

/**
 * greebo: Retrieves the shader name from the current selection.
 * @returns: the name of the shader that is shared by every selected instance or
 * the empty string "" otherwise.
 */
inline std::string getShaderFromSelection()
{
	try
	{
		detail::UniqueShaderFinder finder;

		if (GlobalSelectionSystem().countSelectedComponents() > 0)
		{
			// Check each selected face
			GlobalSelectionSystem().foreachFace([&](IFace& face)
			{
				finder.checkFace(face);
			});
		}
		else
		{
			GlobalSelectionSystem().foreachSelected([&](const scene::INodePtr& node)
			{
				if (Node_isBrush(node))
				{
					finder.checkBrush(*Node_getIBrush(node));
				}
				else if (Node_isPatch(node))
				{
					finder.checkPatch(*Node_getIPatch(node));
				}
			});
		}

		return finder.getFoundShader();
	}
	catch (const detail::AmbiguousShaderException&)
	{
		return std::string(); // return an empty string
	}
}

/**
 * Applies the given shader to the current selection.
 * This doesn't create an UndoableCommand, this has to be done by the caller.
 */
inline void applyShaderToSelection(const std::string& shaderName)
{
    GlobalSelectionSystem().foreachFace([&](IFace& face) { face.setShader(shaderName); });
    GlobalSelectionSystem().foreachPatch([&](IPatch& patch) { patch.setShader(shaderName); });

    SceneChangeNotify();
}

/**
 * Tests the current selection and returns true if the selection is suitable
 * for reparenting the selected primitives to the (last) selected entity.
 */
inline bool curSelectionIsSuitableForReparent()
{
	// Retrieve the selection information structure
	const SelectionInfo& info = GlobalSelectionSystem().getSelectionInfo();

	if (info.totalCount <= 1 || info.entityCount != 1)
	{
		return false;
	}

	scene::INodePtr lastSelected = GlobalSelectionSystem().ultimateSelected();
	Entity* entity = Node_getEntity(lastSelected);

	// Reject non-entities or models
	if (entity == nullptr || entity->isModel())
	{
		return false;
	}

	// Accept only group nodes as parent
	if (!Node_getGroupNode(lastSelected))
	{
		return false;
	}

	return true;
}

// Replaces the group assignments of the given node with the given groups
inline void assignNodeToSelectionGroups(const scene::INodePtr& node, const IGroupSelectable::GroupIds& groups)
{
    auto groupSelectable = std::dynamic_pointer_cast<IGroupSelectable>(node);

    if (!groupSelectable)
    {
        return;
    }

    const auto& groupIds = groupSelectable->getGroupIds();

    auto previousGroups = groupSelectable->getGroupIds();

    for (auto id : previousGroups)
    {
        groupSelectable->removeFromGroup(id);
    }

    // Add one by one, keeping the order intact
    for (auto id : groupIds)
    {
        groupSelectable->addToGroup(id);
    }
}

/// Selection predicates (e.g. for testing if a command should be runnable)
namespace pred
{
    /// Return true if there is at least one selected brush
    inline bool haveBrush()
    {
        return GlobalSelectionSystem().getSelectionInfo().brushCount > 0;
    }

    /// Return true if exactly the given number of entities are selected (and nothing else)
    inline bool haveEntitiesExact(int n)
    {
        const auto& info = GlobalSelectionSystem().getSelectionInfo();
        return info.totalCount == n && info.entityCount == n;
    }

    /// Return true if the exact given number of patches are selected (and nothing else)
    inline bool havePatchesExact(int n)
    {
        const auto& info = GlobalSelectionSystem().getSelectionInfo();
        return info.totalCount == n && info.patchCount == n;
    }

    /// Return true if at least the given number of patches are selected
    inline bool havePatchesAtLeast(int n)
    {
        return GlobalSelectionSystem().getSelectionInfo().patchCount >= n;
    }

    /// Return true if at least one patch is selected
    inline bool havePatch()
    {
        return havePatchesAtLeast(1);
    }
}

} // namespace selection
