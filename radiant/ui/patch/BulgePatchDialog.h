#pragma once

#include "wxutil/dialog/Dialog.h"

/**
 * Jesps: Dialog to query the user for the maxValue
 */
namespace ui
{

class BulgePatchDialog :
	public wxutil::Dialog
{
private:
	// The handle for the noise entry field
	Handle _noiseHandle;

public:
	// Constructor
	BulgePatchDialog();

	// Shows the dialog, returns TRUE if the user selected OK.
	// the given float reference is then filled with the chosen value
	static bool QueryPatchNoise(float& noise);

	// Command target for BulgePatchDialog
	static void BulgePatchCmd(const cmd::ArgumentList& args);
};

} // namespace ui
