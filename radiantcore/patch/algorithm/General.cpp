#include "General.h"

#include "i18n.h"
#include "imainframe.h"
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

void stitchTextures(const cmd::ArgumentList& args)
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
        if (!p1->vertex.isEqual(p2->vertex, epsilon))
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

}

scene::INodePtr createdMergedPatch(const PatchNodePtr& patchNode1, const PatchNodePtr& patchNode2)
{
    constexpr double WELD_EPSILON = 0.001;

    if (patchNode1->getParent() != patchNode2->getParent())
    {
        throw cmd::ExecutionFailure(_("Patches have different parent entities, cannot weld."));
    }

    auto& patch1 = patchNode1->getPatch();
    auto& patch2 = patchNode2->getPatch();

    // Construct edge iterators for the first patch
    // Iterator, Starting Point for copying data to new patch, Edge Length
    auto patch1FirstRow = std::make_pair(PatchEdge{ SinglePatchRowIterator(patch1, 0), patch1.getWidth(), EdgeType::Row }, RowWisePatchIterator(patch1, patch1.getHeight() - 1, 1));
    auto patch1FirstCol = std::make_pair(PatchEdge{ SinglePatchColumnIterator(patch1, 0), patch1.getHeight(), EdgeType::Column }, ColumnWisePatchIterator(patch1, patch1.getWidth() - 1, 1));
    auto patch1LastRow = std::make_pair(PatchEdge{ SinglePatchRowIterator(patch1, patch1.getHeight() - 1), patch1.getWidth(), EdgeType::Row }, RowWisePatchIterator(patch1, 0, patch1.getHeight() - 2));
    auto patch1LastCol = std::make_pair(PatchEdge{ SinglePatchColumnIterator(patch1, patch1.getWidth() - 1), patch1.getHeight(), EdgeType::Column }, ColumnWisePatchIterator(patch1, 0, patch1.getWidth() - 2));

    // We'll be comparing each of the above edges to all other four edges of the second patch
    // and we're doing it in forward and backward iteration, so we're trying to orient patch 2
    // such that the edge vertices are matching up with patch 1
    auto patch2FirstRow = PatchEdge{ RowWisePatchIterator(patch2), patch2.getWidth(), EdgeType::Row };
    auto patch2FirstCol = PatchEdge{ ColumnWisePatchIterator(patch2), patch2.getHeight(), EdgeType::Column };
    auto patch2LastRow = PatchEdge{ RowWisePatchIterator(patch2, patch2.getHeight() - 1, 0), patch2.getWidth(), EdgeType::Row };
    auto patch2LastCol = PatchEdge{ ColumnWisePatchIterator(patch2, patch2.getWidth() - 1, 0), patch2.getHeight(), EdgeType::Column };

    auto patch2FirstRowReverse = PatchEdge{ RowWisePatchReverseIterator(patch2), patch2.getWidth(), EdgeType::Row };
    auto patch2FirstColReverse = PatchEdge{ ColumnWisePatchReverseIterator(patch2), patch2.getHeight(), EdgeType::Column };
    auto patch2LastRowReverse = PatchEdge{ RowWisePatchReverseIterator(patch2, patch2.getHeight() - 1, 0), patch2.getWidth(), EdgeType::Row };
    auto patch2LastColReverse = PatchEdge{ ColumnWisePatchReverseIterator(patch2, patch2.getWidth() - 1, 0), patch2.getHeight(), EdgeType::Column };
    
    std::vector<std::pair<PatchEdge, PatchControlIterator>> patch1Edges =
        { patch1FirstRow, patch1FirstCol, patch1LastRow, patch1LastCol };

    std::vector<PatchEdge> patch2Edges =
    {
        patch2FirstRow, patch2FirstCol, patch2LastRow, patch2LastCol,
        patch2FirstRowReverse, patch2FirstColReverse, patch2LastRowReverse, patch2LastColReverse
    };

    for (const auto& patch1Edge : patch1Edges)
    {
        for (const auto& patch2Edge : patch2Edges)
        {
            if (patch1Edge.first.edgeLength != patch2Edge.edgeLength)
            {
                continue; // length doesn't match
            }

            if (!firstNItemsAreEqual(patch1Edge.first.iterator, patch2Edge.iterator, patch1Edge.first.edgeLength, WELD_EPSILON))
            {
                continue;
            }

            // Expand the patch dimensions
            std::size_t numNewRows = patch1.getHeight();
            std::size_t numNewColumns = patch1.getWidth();
            std::size_t numNewElements = patch2.getWidth() * patch2.getHeight() / patch2Edge.edgeLength - 1;

            auto& dimensionToExpand = patch1Edge.first.edgeType == EdgeType::Row ? numNewRows : numNewColumns;
            dimensionToExpand += numNewElements;

            auto newPatchNode = GlobalPatchModule().createPatch(patch1.subdivisionsFixed() ? PatchDefType::Def3 : PatchDefType::Def2);
            auto& newPatch = std::dynamic_pointer_cast<IPatchNode>(newPatchNode)->getPatch();
            newPatch.setDims(numNewColumns, numNewRows);

            if (patch1Edge.first.edgeType == EdgeType::Row)
            {
                // Load the control points row-wise into the new patch
                RowWisePatchIterator target(newPatch);

                assignPatchControls(patch1Edge.second, target);
                assignPatchControls(patch2Edge.iterator, target);
            }
            else
            {
                // Load the control points column-wise into the new patch
                ColumnWisePatchIterator target(newPatch);

                assignPatchControls(patch1Edge.second, target);
                assignPatchControls(patch2Edge.iterator, target);
            }

            newPatch.controlPointsChanged();

            return newPatchNode;
        }
    }

    throw cmd::ExecutionFailure(_("Unable to weld patches, no suitable edges of same length found"));
}

void weldPatches(const PatchNodePtr& patchNode1, const PatchNodePtr& patchNode2)
{
    auto mergedPatch = createdMergedPatch(patchNode1, patchNode2);

    patchNode1->getParent()->addChildNode(mergedPatch);

    mergedPatch->assignToLayers(patchNode1->getLayers());
    // TODO: selection grouping

    std::dynamic_pointer_cast<IPatchNode>(mergedPatch)->getPatch().scaleTextureNaturally();

    Node_setSelected(mergedPatch, true);

    scene::removeNodeFromParent(patchNode1);
    scene::removeNodeFromParent(patchNode2);
}

void weldSelectedPatches(const cmd::ArgumentList& args)
{
    if (args.size() > 0)
    {
        rWarning() << "Usage: WeldSelectedPatches" << std::endl;
        return;
    }

    auto& selectionInfo = GlobalSelectionSystem().getSelectionInfo();

    // We need to have two patches selected
    if (selectionInfo.totalCount != 2 || selectionInfo.patchCount != 2)
    {
        throw cmd::ExecutionFailure(_("Cannot weld patches, select two patches to weld them."));
    }

    auto patch1 = std::dynamic_pointer_cast<PatchNode>(GlobalSelectionSystem().penultimateSelected());
    auto patch2 = std::dynamic_pointer_cast<PatchNode>(GlobalSelectionSystem().ultimateSelected());

    UndoableCommand cmd("WeldSelectedPatches");
    weldPatches(patch1, patch2);
}

} // namespace

} // namespace
