#include "General.h"

#include "ipatch.h"
#include "patch/PatchNode.h"
#include "patch/Patch.h"
#include "scenelib.h"

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

} // namespace

} // namespace
