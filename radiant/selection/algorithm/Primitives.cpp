#include "Primitives.h"

#include "brush/FaceInstance.h"
#include "patch/PatchSceneWalk.h"
#include "string/string.h"

// greebo: Nasty global that contains all the selected face instances
extern FaceInstanceSet g_SelectedFaceInstances;

namespace selection {
	namespace algorithm {

int selectedFaceCount() {
	return static_cast<int>(g_SelectedFaceInstances.size());
}

Patch& getLastSelectedPatch() {
	if (GlobalSelectionSystem().getSelectionInfo().totalCount > 0 &&
		GlobalSelectionSystem().getSelectionInfo().patchCount > 0)
	{
		// Retrieve the last selected instance
		scene::Instance& instance = GlobalSelectionSystem().ultimateSelected();
		// Try to cast it onto a patch
		PatchInstance* patchInstance = Instance_getPatch(instance);
		
		// Return or throw
		if (patchInstance != NULL) {
			return patchInstance->getPatch();
		}
		else {
			throw selection::InvalidSelectionException("No patches selected.");
		}
	}
	else {
		throw selection::InvalidSelectionException("No patches selected.");
	}
}

Face& getLastSelectedFace() {
	if (selectedFaceCount() == 1) {
		return g_SelectedFaceInstances.last().getFace();
	}
	else {
		throw selection::InvalidSelectionException(intToStr(selectedFaceCount()));
	}
}

	} // namespace algorithm
} // namespace selection
