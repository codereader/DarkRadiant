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

    enum EdgeLocation
    {
        Begin,
        End,
    };

    enum EdgeType
    {
        Row,
        Column,
    };

    // Construct edge iterators for the first patch
    // Iterator, Starting Point for copying data to new patch, Edge Length
    auto patch1FirstRow = std::make_tuple(SinglePatchRowIterator(patch1, 0), RowWisePatchIterator(patch1, patch1.getHeight() - 1, 1), Row, patch1.getWidth(), "patch1FirstRow");
    auto patch1FirstCol = std::make_tuple(SinglePatchColumnIterator(patch1, 0), ColumnWisePatchIterator(patch1, patch1.getWidth() - 1, 1), Column, patch1.getHeight(), "patch1FirstCol");
    auto patch1LastRow = std::make_tuple(SinglePatchRowIterator(patch1, patch1.getHeight() - 1), RowWisePatchIterator(patch1, 0, patch1.getHeight() - 2), Row, patch1.getWidth(), "patch1LastRow");
    auto patch1LastCol = std::make_tuple(SinglePatchColumnIterator(patch1, patch1.getWidth() - 1), ColumnWisePatchIterator(patch1, 0, patch1.getWidth() - 2), Column, patch1.getHeight(), "patch1LastCol");

    // We'll be comparing each of the above edges to all other four edges of the second patch
    // and we're doing it in forward and backward iteration, so we're trying to orient patch 2
    // such that the edge vertices are matching up with patch 1
    auto patch2FirstRow = std::make_tuple(RowWisePatchIterator(patch2), Begin, patch2.getWidth(), "patch2FirstRow");
    auto patch2FirstCol = std::make_tuple(ColumnWisePatchIterator(patch2), Begin, patch2.getHeight(), "patch2FirstCol");
    auto patch2LastRow = std::make_tuple(RowWisePatchIterator(patch2, patch2.getHeight() - 1, 0), End, patch2.getWidth(), "patch2LastRow");
    auto patch2LastCol = std::make_tuple(ColumnWisePatchIterator(patch2, patch2.getWidth() - 1, 0), End, patch2.getHeight(), "patch2LastCol");

    auto patch2FirstRowReverse = std::make_tuple(RowWisePatchReverseIterator(patch2), Begin, patch2.getWidth(), "patch2FirstRowReverse");
    auto patch2FirstColReverse = std::make_tuple(ColumnWisePatchReverseIterator(patch2), Begin, patch2.getHeight(), "patch2FirstColReverse");
    auto patch2LastRowReverse = std::make_tuple(RowWisePatchReverseIterator(patch2, patch2.getHeight() - 1, 0), End, patch2.getWidth(), "patch2LastRowReverse");
    auto patch2LastColReverse = std::make_tuple(ColumnWisePatchReverseIterator(patch2, patch2.getWidth() - 1, 0), End, patch2.getHeight(), "patch2LastColReverse");
    
    std::vector<std::tuple<PatchControlIterator, PatchControlIterator, EdgeType, std::size_t, const char*>> patch1Edges =
        { patch1FirstRow, patch1FirstCol, patch1LastRow, patch1LastCol };

    std::vector<std::tuple<PatchControlIterator, EdgeLocation, std::size_t, const char*>> patch2Edges =
    {
        patch2FirstRow, patch2FirstCol, patch2LastRow, patch2LastCol,
        patch2FirstRowReverse, patch2FirstColReverse, patch2LastRowReverse, patch2LastColReverse
    };

    for (const auto& patch1Edge : patch1Edges)
    {
        for (const auto& patch2Edge : patch2Edges)
        {
            if (std::get<3>(patch1Edge) != std::get<2>(patch2Edge))
            {
                continue; // length don't match
            }

            //rMessage() << "Comparing " << std::get<4>(patch1Edge) << " to " << std::get<3>(patch2Edge) << std::endl;

            if (!firstNItemsAreEqual(std::get<0>(patch1Edge), std::get<0>(patch2Edge), std::get<3>(patch1Edge), WELD_EPSILON))
            {
                continue;
            }

            // Expand the patch dimensions
            std::size_t numNewRows = patch1.getHeight();
            std::size_t numNewColumns = patch1.getWidth();
            std::size_t numNewElements = patch2.getWidth() * patch2.getHeight() / std::get<2>(patch2Edge) - 1;

            auto& dimensionToExpand = std::get<2>(patch1Edge) == Row ? numNewRows : numNewColumns;
            dimensionToExpand += numNewElements;

            auto newPatchNode = GlobalPatchModule().createPatch(patch1.subdivisionsFixed() ? PatchDefType::Def3 : PatchDefType::Def2);
            auto& newPatch = std::dynamic_pointer_cast<IPatchNode>(newPatchNode)->getPatch();
            newPatch.setDims(numNewColumns, numNewRows);

            if (std::get<2>(patch1Edge) == Row)
            {
                RowWisePatchIterator target(newPatch);

                assignPatchControls(std::get<1>(patch1Edge), target);
                assignPatchControls(std::get<0>(patch2Edge), target);
            }
            else
            {
                ColumnWisePatchIterator target(newPatch);

                assignPatchControls(std::get<1>(patch1Edge), target);
                assignPatchControls(std::get<0>(patch2Edge), target);
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
