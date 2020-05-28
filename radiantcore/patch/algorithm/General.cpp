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
				wallPatch->NaturalTexture();

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

} // namespace

} // namespace
