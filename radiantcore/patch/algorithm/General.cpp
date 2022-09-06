#include "General.h"

#include "i18n.h"
#include "itextstream.h"
#include "ipatch.h"
#include "patch/PatchNode.h"
#include "patch/Patch.h"
#include "messages/TextureChanged.h"
#include "selection/algorithm/Primitives.h"

#include "string/convert.h"
#include "scenelib.h"
#include "selectionlib.h"
#include "command/ExecutionFailure.h"
#include "patch/PatchIterators.h"

#include <map>

namespace patch
{

namespace algorithm
{

void thicken(const PatchNodePtr& sourcePatch, float thickness, bool createSeams, int axis)
{
	if (axis < 0 || axis > 3)  throw cmd::ExecutionFailure(fmt::format(_("Invalid axis value: {0}"), string::to_string(axis)));

	// Get a shortcut to the patchcreator
	auto& patchCreator = GlobalPatchModule();

	// Create a new patch node
	scene::INodePtr node(patchCreator.createPatch(patch::PatchDefType::Def2));

	scene::INodePtr parent = sourcePatch->getParent();
	assert(parent != NULL);

	// Insert the node into the same parent as the existing patch
	parent->addChildNode(node);

	// Retrieve the contained patch from the node
	Patch* targetPatch = Node_getPatch(node);

	// Create the opposite patch with the given thickness = distance
	targetPatch->createThickenedOpposite(sourcePatch->getPatchInternal(), thickness, axis);

	// Select the newly created patch
	Node_setSelected(node, true);

	if (createSeams && thickness > 0.0f)
	{
		// Allocate four new patches
		scene::INodePtr nodes[4] = {
			patchCreator.createPatch(patch::PatchDefType::Def2),
			patchCreator.createPatch(patch::PatchDefType::Def2),
			patchCreator.createPatch(patch::PatchDefType::Def2),
			patchCreator.createPatch(patch::PatchDefType::Def2)
		};

		// Now create the four walls
		for (int i = 0; i < 4; i++)
		{
			// Retrieve the contained patch from the node
			Patch* wallPatch = Node_getPatch(nodes[i]);

			// Create the wall patch by passing i as wallIndex
			wallPatch->createThickenedWall(sourcePatch->getPatchInternal(), *targetPatch, i);

			if (!wallPatch->isDegenerate())
			{
				// Insert each node into the same parent as the existing patch
				// It's vital to do this first, otherwise these patches won't have valid shaders
				parent->addChildNode(nodes[i]);

				// Now the shader is realised, apply natural scale
				wallPatch->scaleTextureNaturally();

				// Now select the newly created patch
				Node_setSelected(nodes[i], true);
			}
			else
			{
				rMessage() << "Thicken: Discarding degenerate patch." << std::endl;
			}
		}
	}

	// Invert the target patch so that it faces the opposite direction
	targetPatch->invertMatrix();
}

void stitchTextures()
{
	// Get all the selected patches
	PatchPtrVector patchList = selection::algorithm::getSelectedPatches();

	if (patchList.size() == 2)
	{
		UndoableCommand undo("patchStitchTexture");

		// Fetch the instances from the selectionsystem
		auto targetNode = GlobalSelectionSystem().ultimateSelected();
		auto sourceNode = GlobalSelectionSystem().penultimateSelected();

		// Cast the instances onto a patch
		Patch* source = Node_getPatch(sourceNode);
		Patch* target = Node_getPatch(targetNode);

		if (source && target)
		{
			// Stitch the texture leaving the source patch intact
			target->stitchTextureFrom(*source);
		}
		else
		{
			throw cmd::ExecutionFailure(_("Cannot stitch textures. \nCould not cast nodes to patches."));
		}

		SceneChangeNotify();

		// Update the Texture Tools
		radiant::TextureChangedMessage::Send();
	}
	else
	{
		throw cmd::ExecutionFailure(_("Cannot stitch patch textures. \nExactly 2 patches must be selected."));
	}
}

void bulge(const cmd::ArgumentList& args)
{
	if (args.size() != 1)
	{
		rWarning() << "Usage: BulgePatch <maxNoiseAmplitude>" << std::endl;
		return;
	}

	// Get the list of selected patches
	PatchPtrVector patches = selection::algorithm::getSelectedPatches();

	if (patches.empty())
	{
		throw cmd::ExecutionFailure(_("Cannot bulge patch. No patches selected."));
	}

	double maxValue = args[0].getDouble();

	UndoableCommand cmd("BulgePatch");

	// Cycle through all patches and apply the bulge algorithm
	for (const PatchNodePtr& p : patches)
	{
		Patch& patch = p->getPatchInternal();

		patch.undoSave();

		for (PatchControl& control : patch)
		{
			int randomNumber = int(maxValue * (float(std::rand()) / float(RAND_MAX)));
			control.vertex.set(control.vertex.x(), control.vertex.y(), control.vertex.z() + randomNumber);
		}

		patch.controlPointsChanged();
	}
}

namespace
{

constexpr double WELD_EPSILON = 0.001;

// Returns true if <num> elements in the given sequences are equal
inline bool firstNItemsAreEqual(const PatchControlIterator& sequence1,
    const PatchControlIterator& sequence2, std::size_t num, double epsilon)
{
    // If the iterators are invalid from the start, return false
    if (!sequence1.isValid() || !sequence2.isValid())
    {
        return false;
    }

    auto p1 = sequence1;
    auto p2 = sequence2;

    for (std::size_t n = 0; n < num && p1.isValid() && p2.isValid(); ++n, ++p1, ++p2)
    {
        if (!math::isNear(p1->vertex, p2->vertex, epsilon))
        {
            return false;
        }
    }

    return true;
}

// Copies all values from source to target, until the source sequence is depleted
// Returns the new position of the target iterator
inline void assignPatchControls(const PatchControlIterator& source, PatchControlIterator& target)
{
    auto& t = target;
    for (auto s = source; s.isValid() && t.isValid(); ++s, ++t)
    {
        t->vertex = s->vertex;
        t->texcoord = s->texcoord;
    }
}

enum class EdgeType
{
    Row,
    Column,
};

struct PatchEdge
{
    PatchControlIterator iterator; // The edge to compare against the other patch
    std::size_t edgeLength;    // Edge length, the iterator doesn't know about its length
    EdgeType edgeType;
};

inline bool meshesAreFacingOppositeDirections(const PatchMesh& mesh1, const PatchMesh& mesh2)
{
    // Find the first matching 3D vertex and return it
    for (const auto& v1 : mesh1.vertices)
    {
        for (const auto& v2 : mesh2.vertices)
        {
            if (math::isNear(v1.vertex, v2.vertex, 0.01))
            {
                return std::abs(v1.normal.angle(v2.normal)) > c_half_pi;
            }
        }
    }

    return false;
}

void correctPatchOrientation(const IPatch& originalPatch, IPatch& mergedPatch)
{
    if (meshesAreFacingOppositeDirections(originalPatch.getTesselatedPatchMesh(), mergedPatch.getTesselatedPatchMesh()))
    {
        rMessage() << "Reversing patch matrix" << std::endl;
        mergedPatch.invertMatrix();
    }
}

}

scene::INodePtr createdMergedPatch(const PatchNodePtr& patchNode1, const PatchNodePtr& patchNode2)
{
    auto& patch1 = patchNode1->getPatch();
    auto& patch2 = patchNode2->getPatch();

    // Start with the first patch, construct some edges to compare: first row, last row, first col, last col
    // Associate Edge to the Starting Point for copying data to new patch
    std::vector<std::pair<PatchEdge, PatchControlIterator>> patch1Edges =
    {
        { PatchEdge{ SinglePatchRowIterator(patch1, 0), patch1.getWidth(), EdgeType::Row }, RowWisePatchIterator(patch1, patch1.getHeight() - 1, 1) },
        { PatchEdge{ SinglePatchColumnIterator(patch1, 0), patch1.getHeight(), EdgeType::Column }, ColumnWisePatchIterator(patch1, patch1.getWidth() - 1, 1) },
        { PatchEdge{ SinglePatchRowIterator(patch1, patch1.getHeight() - 1), patch1.getWidth(), EdgeType::Row }, RowWisePatchIterator(patch1, 0, patch1.getHeight() - 2) },
        { PatchEdge{ SinglePatchColumnIterator(patch1, patch1.getWidth() - 1), patch1.getHeight(), EdgeType::Column }, ColumnWisePatchIterator(patch1, 0, patch1.getWidth() - 2) }
    };

    // We'll be comparing each of the above edges to all other four edges of the second patch
    // and we're doing it in forward and backwards direction: we're trying to orient patch 2
    // such that the edge vertices are matching up with patch 1
    std::vector<PatchEdge> patch2Edges =
    {
        { RowWisePatchIterator(patch2), patch2.getWidth(), EdgeType::Row },
        { ColumnWisePatchIterator(patch2), patch2.getHeight(), EdgeType::Column },
        { RowWisePatchIterator(patch2, patch2.getHeight() - 1, 0), patch2.getWidth(), EdgeType::Row },
        { ColumnWisePatchIterator(patch2, patch2.getWidth() - 1, 0), patch2.getHeight(), EdgeType::Column },

        { RowWisePatchReverseIterator(patch2), patch2.getWidth(), EdgeType::Row },
        { ColumnWisePatchReverseIterator(patch2), patch2.getHeight(), EdgeType::Column },
        { RowWisePatchReverseIterator(patch2, patch2.getHeight() - 1, 0), patch2.getWidth(), EdgeType::Row },
        { ColumnWisePatchReverseIterator(patch2, patch2.getWidth() - 1, 0), patch2.getHeight(), EdgeType::Column }
    };

    // As soon as we've found a matching edge, we know exactly how the resulting merged patch
    // should look like. We know the dimensions and we know whether we need to expand the
    // patch row-wise or column-wise.
    for (const auto& p1Iter : patch1Edges)
    {
        const auto& patch1Edge = p1Iter.first;
        const auto& patch1FillData = p1Iter.second;

        for (const auto& patch2Edge : patch2Edges)
        {
            if (patch1Edge.edgeLength != patch2Edge.edgeLength)
            {
                continue; // length doesn't match
            }

            if (!firstNItemsAreEqual(patch1Edge.iterator, patch2Edge.iterator, patch1Edge.edgeLength, WELD_EPSILON))
            {
                continue;
            }

            // Calculate the patch dimensions
            std::size_t numNewRows = patch1.getHeight();
            std::size_t numNewColumns = patch1.getWidth();
            std::size_t numNewEdges = patch2.getWidth() * patch2.getHeight() / patch2Edge.edgeLength - 1;

            auto& dimensionToExpand = patch1Edge.edgeType == EdgeType::Row ? numNewRows : numNewColumns;
            dimensionToExpand += numNewEdges;

            // Check if the new dimensions are out of bounds
            if (numNewColumns > MAX_PATCH_WIDTH || numNewRows > MAX_PATCH_HEIGHT)
            {
                throw cmd::ExecutionFailure(_("Merged patch would exceed maximum dimensions, cannot proceed."));
            }

            // Create the new patch
            auto newPatchNode = GlobalPatchModule().createPatch(patch1.subdivisionsFixed() ? PatchDefType::Def3 : PatchDefType::Def2);
            auto& newPatch = std::dynamic_pointer_cast<IPatchNode>(newPatchNode)->getPatch();
            newPatch.setDims(numNewColumns, numNewRows);

            // Load the control points into the new patch, row-wise or column-wise
            PatchControlIterator target = patch1Edge.edgeType == EdgeType::Row ?
                static_cast<PatchControlIterator>(RowWisePatchIterator(newPatch)) :
                static_cast<PatchControlIterator>(ColumnWisePatchIterator(newPatch));

            // Load all data of the first patch (minus the matching edge) plus all data of the second patch
            assignPatchControls(patch1FillData, target);
            assignPatchControls(patch2Edge.iterator, target);

            newPatch.controlPointsChanged();

            correctPatchOrientation(patch1, newPatch);

            return newPatchNode; // success
        }
    }

    // Failure
    throw cmd::ExecutionFailure(_("Unable to weld patches, no suitable edges of same length found"));
}

void weldPatches(const PatchNodePtr& patchNode1, const PatchNodePtr& patchNode2)
{
    if (patchNode1->getParent() != patchNode2->getParent())
    {
        throw cmd::ExecutionFailure(_("Patches have different parent entities, cannot weld."));
    }

    auto mergedPatchNode = createdMergedPatch(patchNode1, patchNode2);

    patchNode1->getParent()->addChildNode(mergedPatchNode);

    mergedPatchNode->assignToLayers(patchNode1->getLayers());

    auto patch1GroupSelectable = std::dynamic_pointer_cast<IGroupSelectable>(patchNode1);

    if (patch1GroupSelectable)
    {
        selection::assignNodeToSelectionGroups(mergedPatchNode, patch1GroupSelectable->getGroupIds());
    }

    auto& mergedPatch = std::dynamic_pointer_cast<IPatchNode>(mergedPatchNode)->getPatch();

    // Preserve the materials
    mergedPatch.setShader(patchNode1->getPatch().getShader());

    // Preserve fixed subdivision setting of the first patch
    if (patchNode1->getPatch().subdivisionsFixed())
    {
        mergedPatch.setFixedSubdivisions(true, patchNode1->getPatch().getSubdivisions());
    }

    mergedPatch.scaleTextureNaturally();

    Node_setSelected(mergedPatchNode, true);

    scene::removeNodeFromParent(patchNode1);
    scene::removeNodeFromParent(patchNode2);
}

void weldPatchPool()
{
    auto selectedPatches = selection::algorithm::getSelectedPatches();

    // Sort these patches into buckets according to their parents
    // This step may not be strictly necessary as weldPatches() checks the parents
    // but we might save a bit of unnecessary comparison work
    std::map<scene::INodePtr, std::vector<PatchNodePtr>> patchesByEntity;

    for (const auto& patch : selectedPatches)
    {
        patchesByEntity[patch->getParent()].push_back(patch);
    }

    std::size_t numPatchesCreated = 0;

    for (const auto& pair : patchesByEntity)
    {
        // Try to combine each patch of this list with the rest of them O(2n)
        for (auto p1 = pair.second.begin(); p1 != pair.second.end(); ++p1)
        {
            if (!(*p1)->getParent()) continue; // patch has been merged already

            for (auto p2 = p1 + 1; p2 != pair.second.end(); ++p2)
            {
                if (!(*p2)->getParent()) continue;// patch has been merged already

                try
                {
                    weldPatches(*p1, *p2);
                    ++numPatchesCreated;
                }
                catch (const cmd::ExecutionFailure&)
                {
                    continue; // failed to merge these two
                }
            }
        }
    }

    if (numPatchesCreated == 0)
    {
        throw cmd::ExecutionFailure(_("Cannot weld, patches need have the same parent entity."));
    }
}

void weldSelectedPatches(const cmd::ArgumentList& args)
{
    if (args.size() > 0)
    {
        rWarning() << "Usage: WeldSelectedPatches" << std::endl;
        return;
    }

    auto& selectionInfo = GlobalSelectionSystem().getSelectionInfo();

    if (selectionInfo.totalCount == 2 || selectionInfo.patchCount == 2)
    {
        // Special handling for exactly two selected patches: the selection order is important
        // Merge them and preserve the properties of the first selected patch
        auto patch1 = std::dynamic_pointer_cast<PatchNode>(GlobalSelectionSystem().penultimateSelected());
        auto patch2 = std::dynamic_pointer_cast<PatchNode>(GlobalSelectionSystem().ultimateSelected());

        UndoableCommand cmd("WeldSelectedPatches");
        weldPatches(patch1, patch2);
    }
    else if (selectionInfo.patchCount >= 2)
    {
        // We have many things with at least two patches selected, try to find a matching combo
        UndoableCommand cmd("WeldSelectedPatches");
        weldPatchPool();
    }
    else
    {
        throw cmd::ExecutionFailure(_("Cannot weld patches, select (at least) two patches with the same parent entity."));
    }
}

} // namespace

} // namespace
