#pragma once

#include <stdexcept>
#include "iselection.h"
#include "ibrush.h"
#include "igroupnode.h"
#include "ientity.h"
#include "ipatch.h"
#include "math/Vector3.h"
#include "math/AABB.h"

/** greebo: A structure containing information about the current
 * Selection. An instance of this is maintained by the
 * RadiantSelectionSystem, and a const reference can be
 * retrieved via the according getSelectionInfo() method.
 */
class SelectionInfo {
public:
	int totalCount; 	// number of selected items
	int patchCount; 	// number of selected patches
	int brushCount; 	// -- " -- brushes
	int entityCount; 	// -- " -- entities
	int componentCount;	// -- " -- components (faces, edges, vertices)

	SelectionInfo() :
		totalCount(0),
		patchCount(0),
		brushCount(0),
		entityCount(0),
		componentCount(0)
	{}

	// Zeroes all the counters
	void clear() {
		totalCount = 0;
		patchCount = 0;
		brushCount = 0;
		entityCount = 0;
		componentCount = 0;
	}
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

} // namespace selection
