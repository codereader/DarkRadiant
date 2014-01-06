#include "General.h"

#include "i18n.h"
#include "imainframe.h"
#include "itextstream.h"
#include "ipatch.h"
#include "patch/PatchNode.h"
#include "patch/Patch.h"
#include "gtkutil/dialog/MessageBox.h"
#include "ui/patch/BulgePatchDialog.h"
#include "ui/surfaceinspector/SurfaceInspector.h"
#include "selection/algorithm/Primitives.h"

namespace patch
{

namespace algorithm
{

void thicken(const PatchNodePtr& sourcePatch, float thickness, bool createSeams, int axis)
{
	// Get a shortcut to the patchcreator
	PatchCreator& patchCreator = GlobalPatchCreator(DEF2);

	// Create a new patch node
	scene::INodePtr node(patchCreator.createPatch());

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
			patchCreator.createPatch(),
			patchCreator.createPatch(),
			patchCreator.createPatch(),
			patchCreator.createPatch()
		};

		// Now create the four walls
		for (int i = 0; i < 4; i++)
		{
			// Insert each node into the same parent as the existing patch
			// It's vital to do this first, otherwise these patches won't have valid shaders
			parent->addChildNode(nodes[i]);

			// Retrieve the contained patch from the node
			Patch* wallPatch = Node_getPatch(nodes[i]);

			// Create the wall patch by passing i as wallIndex
			wallPatch->createThickenedWall(sourcePatch->getPatchInternal(), *targetPatch, i);

			if (!wallPatch->isDegenerate())
			{
				// Now select the newly created patch
				Node_setSelected(nodes[i], true);
			}
			else
			{
				rMessage() << "Thicken: Discarding degenerate patch." << std::endl;

				// Remove again
				parent->removeChildNode(nodes[i]);
			}
		}
	}

	// Invert the target patch so that it faces the opposite direction
	targetPatch->InvertMatrix();
}

void stitchTextures(const cmd::ArgumentList& args)
{
	// Get all the selected patches
	PatchPtrVector patchList = selection::algorithm::getSelectedPatches();

	if (patchList.size() == 2)
	{
		UndoableCommand undo("patchStitchTexture");

		// Fetch the instances from the selectionsystem
		const scene::INodePtr& targetNode =
			GlobalSelectionSystem().ultimateSelected();

		const scene::INodePtr& sourceNode =
			GlobalSelectionSystem().penultimateSelected();

		// Cast the instances onto a patch
		Patch* source = Node_getPatch(sourceNode);
		Patch* target = Node_getPatch(targetNode);

		if (source != NULL && target != NULL) {
			// Stitch the texture leaving the source patch intact
			target->stitchTextureFrom(*source);
		}
		else {
			gtkutil::MessageBox::ShowError(_("Cannot stitch textures. \nCould not cast nodes to patches."),
							 GlobalMainFrame().getTopLevelWindow());
		}

		SceneChangeNotify();
		// Update the Texture Tools
		ui::SurfaceInspector::update();
	}
	else
	{
		gtkutil::MessageBox::ShowError(_("Cannot stitch patch textures. \nExactly 2 patches must be selected."),
							 GlobalMainFrame().getTopLevelWindow());
	}
}

void bulge(const cmd::ArgumentList& args)
{
	// Get the list of selected patches
	PatchPtrVector patches = selection::algorithm::getSelectedPatches();

	if (!patches.empty())
	{
		int maxValue = 16;

		// Ask the user to enter a noise value
		if (ui::BulgePatchDialog::queryPatchNoise(maxValue))
		{
			UndoableCommand cmd("BulgePatch");

			// Cycle through all patches and apply the bulge algorithm
			for (PatchPtrVector::iterator p = patches.begin(); p != patches.end(); ++p)
			{
				Patch& patch = (*p)->getPatchInternal();

				patch.undoSave();

				for (PatchControlIter i = patch.begin(); i != patch.end(); ++i)
				{
					PatchControl& control = *i;
					int randomNumber = int(maxValue * (float(std::rand()) / float(RAND_MAX)));
					control.vertex.set(control.vertex.x(), control.vertex.y(), control.vertex.z() + randomNumber);
				}

				patch.controlPointsChanged();
			}
		}
	}
	else
	{
		gtkutil::MessageBox::ShowError(_("Cannot bulge patch. No patches selected."),
			GlobalMainFrame().getTopLevelWindow());
	}
}

} // namespace

} // namespace
