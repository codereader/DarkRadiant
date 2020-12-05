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
#include "command/ExecutionFailure.h"

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

PatchNodePtr createdMergedPatch(const PatchNodePtr& patchNode1, const PatchNodePtr& patchNode2,
    int sourceCol1, int sourceCol2, int sourceRow1, int sourceRow2, bool invert1, bool invert2)
{
    float		sScale, tScale;
    float		s, t;
    int			col1, col2;
    int			row1, row2;
    int			adj1, adj2;
    int			in, out;
    int			w, h;

    auto& patch1 = patchNode1->getPatch();
    auto& patch2 = patchNode2->getPatch();

    auto node = GlobalPatchModule().createPatch(patch1.subdivisionsFixed() ? PatchDefType::Def3 : PatchDefType::Def2);
    auto newPatchNode = std::dynamic_pointer_cast<PatchNode>(node);
    auto& newPatch = newPatchNode->getPatch();

    newPatch.setShader(patch1.getShader());

    // Merge the patches
    if (sourceCol1 >= 0)
    {
        // Adding width
        if (sourceCol2 >= 0)
        {
            // From width
            newPatch.setDims(patch1.getWidth() + patch2.getWidth() - 1, patch1.getHeight());

            col1 = 0;
            col2 = 1;

            adj1 = 1;
            adj2 = 1;

            if (sourceCol1 != 0)
            {
                adj1 = -1;
                col1 = patch1.getWidth() - 1;
            }

            if (sourceCol2 != 0)
            {
                adj2 = -1;
                col2 = patch2.getWidth() - 2;
            }

            out = 0;

            for (w = 0; w < patch1.getWidth(); w++, col1 += adj1)
            {
                in = invert1 ? patch1.getHeight() - 1 : 0;

                for (h = 0; h < patch1.getHeight(); h++)
                {
                    newPatch.ctrlAt(out, h) = patch1.ctrlAt(col1, in);

                    in += invert1 ? -1 : 1;
                }

                out++;
            }

            for (w = 1; w < patch2.getWidth(); w++, col2 += adj2)
            {
                in = invert2 ? patch2.getHeight() - 1 : 0;

                for (h = 0; h < patch2.getHeight(); h++)
                {
                    newPatch.ctrlAt(out, h) = patch2.ctrlAt(col2, in);

                    in += invert2 ? -1 : 1;
                }

                out++;
            }
        }
        else
        {
            // From height
            newPatch.setDims(patch1.getWidth() + patch2.getHeight() - 1, patch1.getHeight());

            col1 = 0;
            row2 = 1;

            adj1 = 1;
            adj2 = 1;

            if (sourceCol1 != 0)
            {
                adj1 = -1;
                col1 = patch1.getWidth() - 1;
            }

            if (sourceRow2 != 0)
            {
                adj2 = -1;
                row2 = patch2.getHeight() - 2;
            }

            out = 0;

            for (w = 0; w < patch1.getWidth(); w++, col1 += adj1)
            {
                in = invert1 ? patch1.getHeight() - 1 : 0;

                for (h = 0; h < patch1.getHeight(); h++)
                {
                    newPatch.ctrlAt(out, h) = patch1.ctrlAt(col1, in);

                    in += invert1 ? -1 : 1;
                }

                out++;
            }

            for (h = 1; h < patch2.getHeight(); h++, row2 += adj2)
            {
                in = invert2 ? patch2.getWidth() - 1 : 0;

                for (w = 0; w < patch2.getWidth(); w++)
                {
                    newPatch.ctrlAt(out, w) = patch2.ctrlAt(in, row2);

                    in += invert2 ? -1 : 1;
                }

                out++;
            }
        }
    }
    else
    {
        // Adding height
        if (sourceRow1 >= 0)
        {
            // From height
            newPatch.setDims(patch1.getWidth(), patch1.getHeight() + patch2.getHeight() - 1);

            row1 = 0;
            row2 = 0;

            adj1 = 1;
            adj2 = 1;

            if (sourceRow1 != 0)
            {
                adj1 = -1;
                row1 = patch1.getHeight() - 1;
            }

            if (sourceRow2 != 0)
            {
                adj2 = -1;
                row2 = patch2.getHeight() - 2;
            }

            out = 0;

            for (h = 0; h < patch1.getHeight(); h++, row1 += adj1)
            {
                in = invert1 ? patch1.getWidth() - 1 : 0;

                for (w = 0; w < patch1.getWidth(); w++)
                {
                    newPatch.ctrlAt(w, out) = patch1.ctrlAt(in, row1);

                    in += invert1 ? -1 : 1;
                }

                out++;
            }

            for (h = 1; h < patch2.getHeight(); h++, row2 += adj2)
            {
                in = invert2 ? patch1.getWidth() - 1 : 0;

                for (w = 0; w < patch2.getWidth(); w++)
                {
                    newPatch.ctrlAt(w, out) = patch2.ctrlAt(in, row2);

                    in += invert1 ? -1 : 1;
                }

                out++;
            }
        }
        else
        {
            // From width
            newPatch.setDims(patch1.getWidth() + patch2.getHeight() - 1, patch1.getHeight());

            row1 = 0;
            col2 = 0;

            adj1 = 1;
            adj2 = 1;

            if (sourceRow1 != 0)
            {
                adj1 = -1;
                row1 = patch1.getHeight() - 1;
            }

            if (sourceCol2 != 0)
            {
                adj2 = -1;
                col2 = patch2.getWidth() - 2;
            }

            out = 0;

            for (h = 0; h < patch1.getHeight(); h++, row1 += adj1) 
            {
                in = invert1 ? patch1.getWidth() - 1 : 0;

                for (w = 0; w < patch1.getWidth(); w++)
                {
                    newPatch.ctrlAt(w, out) = patch1.ctrlAt(in, row1);

                    in += invert1 ? -1 : 1;
                }

                out++;
            }

            for (w = 1; w < patch2.getWidth(); w++, col2 += adj2)
            {
                in = invert2 ? patch1.getHeight() - 1 : 0;

                for (h = 0; h < patch2.getHeight(); h++)
                {
                    newPatch.ctrlAt(h, out) = patch2.ctrlAt(col2, in);

                    in += invert1 ? -1 : 1;
                }

                out++;
            }
        }
    }

#if 0
    // Generate the texture coords
    GetTextureMappingScales(newPatch->material->editorImage, &sScale, &tScale);

    for (w = 0; w < newPatch->width; w++) {
        s = Patch_WidthDistanceTo(newPatch, w);

        for (h = 0; h < newPatch->height; h++) {
            t = Patch_HeightDistanceTo(newPatch, h);

            newPatch.ctrlAt(w, h).st[0] = s * sScale;
            newPatch.ctrlAt(w, h).st[1] = 1.0f - t * tScale;
        }
    }
#endif

    return newPatchNode;
}

PatchNodePtr createdMergedPatch(const PatchNodePtr& patchNode1, const PatchNodePtr& patchNode2)
{
    constexpr double WELD_EPSILON = 0.001;

    // Algorithm ported from ODRadiant https://svn.code.sf.net/p/odblur/code/code/OverDose%20Tools/ODRadiant/Patch.cpp) rev8348
    int		col, col1, col2;
    int		row, row1, row2;
    int		adj1, adj2;
    bool	match;

    if (patchNode1->getParent() != patchNode2->getParent())
    {
        throw cmd::ExecutionFailure(_("Patches have different parent entities, cannot weld."));
    }

    auto& patch1 = patchNode1->getPatch();
    auto& patch2 = patchNode2->getPatch();

    if (patch1.getWidth() == patch2.getWidth())
    {
        row1 = 0;
        row2 = 0;

        while (true)
        {
            match = false;

            col1 = 0;
            col2 = 0;

            adj1 = 0;
            adj2 = 0;

            if (patch1.ctrlAt(0, row1).vertex.isEqual(patch2.ctrlAt(0, row2).vertex, WELD_EPSILON))
            {}
            else if (patch1.ctrlAt(0, row1).vertex.isEqual(patch2.ctrlAt(patch2.getWidth() - 1, row2).vertex, WELD_EPSILON))
            {
                col2 = patch2.getWidth() - 1;
                adj2 = -1;
            }
            else if (patch1.ctrlAt(patch1.getWidth() - 1, row1).vertex.isEqual(patch2.ctrlAt(patch2.getWidth() - 1, row2).vertex, WELD_EPSILON))
            {
                col2 = patch2.getWidth() - 1;
                adj2 = -1;

                col1 = patch1.getWidth() - 1;
                adj1 = -1;
            }
            else if (patch1.ctrlAt(patch1.getWidth() - 1, row1).vertex.isEqual(patch2.ctrlAt(0, row2).vertex, WELD_EPSILON))
            {
                col1 = patch1.getWidth() - 1;
                adj1 = -1;
            }
            else
            {
                adj1 = 0;
            }

            if (adj1)
            {
                match = true;

                for (col = 0; col < patch1.getWidth(); col++, col2 += adj2, col1 += adj1)
                {
                    if (!patch1.ctrlAt(col1, row1).vertex.isEqual(patch2.ctrlAt(col2, row2).vertex, WELD_EPSILON))
                    {
                        match = false;
                        break;
                    }
                }
            }

            if (match)
            {
                rMessage() << "Welding row " << row1 << " with row " << row2 << std::endl;

                row1 = (row1 == 0) ? patch1.getHeight() - 1 : 0;

                return createdMergedPatch(patchNode1, patchNode2, -1, -1, row1, row2, (adj1 == -1), (adj2 == -1));
            }

            if (row2 == 0)
            {
                row2 = patch2.getHeight() - 1;
            }
            else if (row1 == 0)
            {
                row1 = patch1.getHeight() - 1;
                row2 = 0;
            }
            else
            {
                break;
            }
        }
    }

    if (patch1.getWidth() == patch2.getHeight())
    {
        row1 = 0;
        col2 = 0;

        while (1)
        {
            match = false;

            col1 = 0;
            adj1 = 1;

            row2 = 0;
            adj2 = 1;

            if (patch1.ctrlAt(0, row1).vertex.isEqual(patch2.ctrlAt(col2, 0).vertex, WELD_EPSILON))
            {}
            else if (patch1.ctrlAt(0, row1).vertex.isEqual(patch2.ctrlAt(col2, patch2.getHeight() - 1).vertex, WELD_EPSILON))
            {
                row2 = patch2.getHeight() - 1;
                adj2 = -1;
            }
            else if (patch1.ctrlAt(patch1.getWidth() - 1, row1).vertex.isEqual(patch2.ctrlAt(col2, patch2.getHeight() - 1).vertex, WELD_EPSILON))
            {
                row2 = patch2.getHeight() - 1;
                adj2 = -1;

                col1 = patch1.getWidth() - 1;
                adj1 = -1;
            }
            else if (patch1.ctrlAt(patch1.getWidth() - 1, row1).vertex.isEqual(patch2.ctrlAt(col2, 0).vertex, WELD_EPSILON))
            {
                col1 = patch1.getWidth() - 1;
                adj1 = -1;
            }
            else
            {
                adj1 = 0;
            }

            if (adj1)
            {
                match = true;

                for (col = 0; col < patch1.getWidth(); col++, col1 += adj1, row2 += adj2)
                {
                    if (!patch1.ctrlAt(col1, row1).vertex.isEqual(patch2.ctrlAt(col2, row2).vertex, WELD_EPSILON))
                    {
                        match = false;
                        break;
                    }
                }
            }

            if (match)
            {
                rMessage() << "Welding row " << row1 << " with column " << col2 << std::endl;

                row1 = (row1 == 0) ? patch1.getHeight() - 1 : 0;

                return createdMergedPatch(patchNode1, patchNode2, -1, col2, row1, -1, (adj1 == -1), (adj2 == -1));
            }

            if (col2 == 0)
            {
                col2 = patch2.getWidth() - 1;
            }
            else if (row1 == 0) 
            {
                row1 = patch1.getHeight() - 1;
                col2 = 0;
            }
            else
            {
                break;
            }
        }
    }

    if (patch1.getHeight() == patch2.getWidth())
    {
        col1 = 0;
        row2 = 0;

        while (1)
        {
            match = false;

            row1 = 0;
            adj1 = 1;

            col2 = 0;
            adj2 = 1;

            if (patch1.ctrlAt(col1, 0).vertex.isEqual(patch2.ctrlAt(0, row2).vertex, WELD_EPSILON))
            {}
            else if (patch1.ctrlAt(col1, 0).vertex.isEqual(patch2.ctrlAt(patch2.getWidth() - 1, row2).vertex, WELD_EPSILON))
            {
                col2 = patch2.getWidth() - 1;
                adj2 = -1;
            }
            else if (patch1.ctrlAt(col1, patch1.getHeight() - 1).vertex.isEqual(patch2.ctrlAt(patch2.getWidth() - 1, row2).vertex, WELD_EPSILON))
            {
                col2 = patch2.getWidth() - 1;
                adj2 = -1;

                row1 = patch2.getHeight() - 1;
                adj2 = -1;
            }
            else if (patch1.ctrlAt(col1, patch1.getHeight() - 1).vertex.isEqual(patch2.ctrlAt(0, row2).vertex, WELD_EPSILON))
            {
                row1 = patch2.getHeight() - 1;
                adj2 = -1;
            }
            else
            {
                adj1 = 0;
            }

            if (adj1)
            {
                match = true;

                for (row = 0; row < patch1.getHeight(); row++, row1 += adj1, col2 += adj2)
                {
                    if (!patch1.ctrlAt(col1, row1).vertex.isEqual(patch2.ctrlAt(col2, row2).vertex, WELD_EPSILON))
                    {
                        match = false;
                        break;
                    }
                }
            }

            if (match)
            {
                rMessage() << "Welding column " << col1 << " with row " << row2 << std::endl;

                col1 = (col1 == 0) ? patch1.getWidth() - 1 : 0;

                return createdMergedPatch(patchNode1, patchNode2, col1, -1, -1, row2, (adj1 == -1), (adj2 == -1));
            }

            if (row2 == 0)
            {
                row2 = patch2.getHeight() - 1;
            }
            else if (col1 == 0)
            {
                col1 = patch1.getWidth() - 1;
                row2 = 0;
            }
            else
            {
                break;
            }
        }
    }

    if (patch1.getHeight() == patch2.getHeight())
    {
        col1 = 0;
        col2 = 0;

        while (1) 
        {
            match = false;

            row1 = 0;
            adj1 = 1;

            row2 = 0;
            adj2 = 1;

            if (patch1.ctrlAt(col1, 0).vertex.isEqual(patch2.ctrlAt(col2, 0).vertex, WELD_EPSILON))
            {}
            else if (patch1.ctrlAt(col1, 0).vertex.isEqual(patch2.ctrlAt(col2, patch2.getHeight() - 1).vertex, WELD_EPSILON))
            {
                row2 = patch2.getHeight() - 1;
                adj2 = -1;
            }
            else if (patch1.ctrlAt(col1, patch1.getHeight() - 1).vertex.isEqual(patch2.ctrlAt(col2, patch2.getHeight() - 1).vertex, WELD_EPSILON))
            {
                row2 = patch2.getHeight() - 1;
                adj2 = -1;

                row1 = patch1.getHeight() - 1;
                adj1 = -1;
            }
            else if (patch1.ctrlAt(col1, patch1.getHeight() - 1).vertex.isEqual(patch2.ctrlAt(col2, 0).vertex, WELD_EPSILON))
            {
                row1 = patch1.getHeight() - 1;
                adj1 = -1;
            }
            else
            {
                adj1 = 0;
            }

            if (adj1)
            {
                match = true;

                for (row = 0; row < patch1.getHeight(); row++, row1 += adj1, row2 += adj2)
                {
                    if (!patch1.ctrlAt(col1, row1).vertex.isEqual(patch2.ctrlAt(col2, row2).vertex, WELD_EPSILON))
                    {
                        match = false;
                        break;
                    }
                }
            }

            if (match)
            {
                rMessage() << "Welding column " << col1 << " with column " << col2 << std::endl;

                col1 = (col1 == 0) ? patch1.getWidth() - 1 : 0;

                return createdMergedPatch(patchNode1, patchNode2, col1, col2, -1, -1, (adj1 == -1), (adj2 == -1));
            }

            if (col2 == 0)
            {
                col2 = patch2.getWidth() - 1;
            }
            else if (col1 == 0)
            {
                col1 = patch1.getWidth() - 1;
                col2 = 0;
            }
            else
            {
                break;
            }
        }
    }

    throw cmd::ExecutionFailure(_("Unable to weld patches, no suitable edges of same length found"));
}

void weldPatches(const PatchNodePtr& patchNode1, const PatchNodePtr& patchNode2)
{
    auto mergedPatch = createdMergedPatch(patchNode1, patchNode2);
    assert(mergedPatch);

    patchNode1->getParent()->addChildNode(mergedPatch);

    mergedPatch->assignToLayers(patchNode1->getLayers());
    // TODO: selection grouping

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

    // Get the list of selected patches
    auto patches = selection::algorithm::getSelectedPatches();

    if (patches.size() != 2)
    {
        throw cmd::ExecutionFailure(_("Cannot weld patches, select two patches to weld them."));
    }

    UndoableCommand cmd("WeldSelectedPatches");
    weldPatches(patches[0], patches[1]);
}

} // namespace

} // namespace
