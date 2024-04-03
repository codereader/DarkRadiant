#include "Prefab.h"

#include "i18n.h"
#include "iselection.h"
#include "ipatch.h"
#include "igrid.h"
#include "itextstream.h"
#include "iorthoview.h"
#include "ishaderclipboard.h"

#include "scenelib.h"
#include "shaderlib.h"
#include "patch/Patch.h"

#include "selection/algorithm/General.h"
#include "selectionlib.h"
#include "command/ExecutionFailure.h"

#include "string/case_conv.h"

namespace patch
{

namespace algorithm
{

namespace
{
	// Gets the active/selected shader or the default fallback value
	inline std::string getSelectedShader()
	{
		auto selectedShader = GlobalShaderClipboard().getShaderName();

		if (selectedShader.empty())
		{
			selectedShader = texdef_name_default();
		}

		return selectedShader;
	}
}

void constructPrefab(const AABB& aabb, const std::string& shader, EPatchPrefab eType,
					 OrthoOrientation viewType, std::size_t width, std::size_t height)
{
	GlobalSelectionSystem().setSelectedAll(false);

	scene::INodePtr node(GlobalPatchModule().createPatch(patch::PatchDefType::Def2));
	GlobalMapModule().findOrInsertWorldspawn()->addChildNode(node);

	Patch* patch = Node_getPatch(node);
	patch->setShader(shader);

	patch->ConstructPrefab(aabb, eType, viewType, width, height);
	patch->controlPointsChanged();

	Node_setSelected(node, true);
}

AABB getDefaultBoundsFromSelection()
{
	AABB aabb = GlobalSelectionSystem().getWorkZone().bounds;

	float gridSize = GlobalGrid().getGridSize();

	if (aabb.extents[0] == 0)
	{
		aabb.extents[0] = gridSize;
	}

	if (aabb.extents[1] == 0)
	{
		aabb.extents[1] = gridSize;
	}

	if (aabb.extents[2] == 0)
	{
		aabb.extents[2] = gridSize;
	}

	if (aabb.isValid())
	{
		return aabb;
	}

	return AABB(Vector3(0, 0, 0), Vector3(64, 64, 64));
}

void createPrefabInternal(EPatchPrefab prefabType, const std::string& undoCmdName)
{
	UndoableCommand undo(undoCmdName);

	constructPrefab(getDefaultBoundsFromSelection(),
					getSelectedShader(),
					prefabType,
					GlobalOrthoViewManager().getActiveViewType());
}

void createPrefab(const cmd::ArgumentList& args)
{
	if (args.size() != 1)
	{
		rError() << "Usage: createPatchPrefab <type>" << std::endl
			<< " with <type> being one of the following: " << std::endl
			<< "cylinder, densecylinder, verydensecylinder, squarecylinder," << std::endl
			<< "sphere, endcap, bevel, cone" << std::endl;
		return;
	}

	std::string typeStr = string::to_lower_copy(args[0].getString());

	if (typeStr == "cylinder")
	{
		createPrefabInternal(eCylinder, "patchCreateCylinder");
	}
	else if (typeStr == "densecylinder")
	{
		createPrefabInternal(eDenseCylinder, "patchCreateDenseCylinder");
	}
	else if (typeStr == "verydensecylinder")
	{
		createPrefabInternal(eVeryDenseCylinder, "patchCreateVeryDenseCylinder");
	}
	else if (typeStr == "squarecylinder")
	{
		createPrefabInternal(eSqCylinder, "patchCreateSquareCylinder");
	}
	else if (typeStr == "sphere")
	{
		createPrefabInternal(eSphere, "patchCreateSphere");
	}
	else if (typeStr == "endcap")
	{
		createPrefabInternal(eEndCap, "patchCreateCaps");
	}
	else if (typeStr == "bevel")
	{
		createPrefabInternal(eBevel, "patchCreateBevel");
	}
	else if (typeStr == "cone")
	{
		createPrefabInternal(eCone, "patchCreateCone");
	}
}

void createCylinder(const cmd::ArgumentList& args)
{
	createPrefabInternal(eCylinder, "patchCreateCylinder");
}

void createDenseCylinder(const cmd::ArgumentList& args)
{
	createPrefabInternal(eDenseCylinder, "patchCreateDenseCylinder");
}

void createVeryDenseCylinder(const cmd::ArgumentList& args)
{
	createPrefabInternal(eVeryDenseCylinder, "patchCreateVeryDenseCylinder");
}

void createSquareCylinder(const cmd::ArgumentList& args)
{
	createPrefabInternal(eSqCylinder, "patchCreateSquareCylinder");
}

void createSphere(const cmd::ArgumentList& args)
{
	createPrefabInternal(eSphere, "patchCreateSphere");
}

void createEndcap(const cmd::ArgumentList& args)
{
	createPrefabInternal(eEndCap, "patchCreateCaps");
}

void createBevel(const cmd::ArgumentList& args)
{
	createPrefabInternal(eBevel, "patchCreateBevel");
}

void createCone(const cmd::ArgumentList& args)
{
	createPrefabInternal(eCone, "patchCreateCone");
}

// Sanitise the integer to specify a valid patch dimension
// will return 0 if the input is invalid
std::size_t checkPatchDimension(int input)
{
	// Must be an odd number in [3..15]
	if (input < 3 || input > 15 || input % 2 == 0)
	{
		return 0;
	}

	// all good
	return static_cast<std::size_t>(input);
}

void createSimplePatch(const cmd::ArgumentList& args)
{
	std::size_t width = 0;
	std::size_t height = 0;
	bool removeSelectedBrush = false;

	if (args.empty() || args.size() > 3)
	{
		throw cmd::ExecutionFailure(_("Invalid number of arguments"));
	}
	if (args.size() == 1)
	{
		// Try to convert the arguments to actual integers and do the range checks
		width = height = checkPatchDimension(args[0].getInt());
	}
	else if (args.size() == 2)
	{
		width = checkPatchDimension(args[0].getInt());
		height = checkPatchDimension(args[1].getInt());
	}
	else if (args.size() == 3)
	{
		width = checkPatchDimension(args[0].getInt());
		height = checkPatchDimension(args[1].getInt());
		removeSelectedBrush = args[2].getBoolean();
	}

	// Only fire the dialog if no or invalid command arguments are given
	if (width != 0 && height != 0)
	{
		UndoableCommand undo("patchCreatePlane");

		// Retrieve the boundaries before any delete operation
		AABB bounds = getDefaultBoundsFromSelection();

		if (removeSelectedBrush)
		{
			// Delete the selection, the should be only one brush selected
			selection::algorithm::deleteSelection();
		}

		// Call the PatchConstruct routine (GtkRadiant legacy)
		constructPrefab(bounds,
			getSelectedShader(),
			ePlane, GlobalOrthoViewManager().getActiveViewType(),
			width, height);
	}
}

scene::INodePtr constructCap(const IPatch& sourcePatch, CapType capType, bool front, const std::string& material)
{
    auto cap = GlobalPatchModule().createPatch(PatchDefType::Def2);

    auto& capPatch = *Node_getPatch(cap);

    auto width = sourcePatch.getWidth();
    auto height = sourcePatch.getHeight();

    std::vector<Vector3> points(sourcePatch.getWidth());

    auto row = front ? 0 : height - 1;

    for (auto i = 0; i < width; i++)
    {
        const auto& ctrl = sourcePatch.ctrlAt(row, i);
        points[front ? i : width - 1 - i] = ctrl.vertex;
    }

    // Inherit the same fixed tesselation as the source patch
    if (sourcePatch.subdivisionsFixed())
    {
        const auto& subdivisions = sourcePatch.getSubdivisions();

        if (capType == CapType::InvertedEndCap)
        {
            capPatch.setFixedSubdivisions(true, subdivisions);
        }
        else
        {
            // Flip the subdivision X/Y values for all other cap types
            capPatch.setFixedSubdivisions(true, { subdivisions.y(), subdivisions.x() });
        }
    }

    capPatch.constructSeam(capType, points, width);

    // greebo: Avoid creating "degenerate" patches (all vertices merged in one 3D point)
    if (capPatch.isDegenerate())
    {
        return {};
    }

    // greebo: Apply natural texture to that patch, to fix the texcoord==1.#INF bug.
    capPatch.setShader(material);
    capPatch.scaleTextureNaturally();

    return cap;
}

void createCaps(const IPatch& patch, const scene::INodePtr& parent, CapType type, const std::string& shader)
{
    if ((type == CapType::EndCap || type == CapType::InvertedEndCap) && patch.getWidth() != 5)
    {
        throw cmd::ExecutionFailure(_("Cannot create end-cap, patch must have a width of 5."));
    }

    if ((type == CapType::Bevel || type == CapType::InvertedBevel) && patch.getWidth() != 3)
    {
        throw cmd::ExecutionFailure(_("Cannot create bevel-cap, patch must have a width of 3."));
    }

    if (type == CapType::Cylinder && patch.getWidth() != 9)
    {
        throw cmd::ExecutionFailure(_("Cannot create cylinder-cap, patch must have a width of 9."));
    }

    assert(parent);

    // We do this once for the front and once for the back patch
    for (auto front : { true, false })
    {
        auto cap = constructCap(patch, type, front, shader);

        if (cap)
        {
            parent->addChildNode(cap);
            Node_setSelected(cap, true);
        }
    }
}

}

} // namespace
