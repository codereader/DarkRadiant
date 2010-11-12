#ifndef BULGEPATCHDIALOG_H_
#define BULGEPATCHDIALOG_H_

#include "gtkutil/dialog/Dialog.h"

/**
 * Jesps: Dialog to query the user for the maxValue
 */
namespace ui
{

class BulgePatchDialog :
	public gtkutil::Dialog
{
private:
	// The handle for the noise entry field
	Handle _noiseHandle;

public:
	// Constructor
	BulgePatchDialog();

	// Shows the dialog, returns TRUE if the user selected OK.
	// the given integer reference is then filled with the chosen value
	static bool queryPatchNoise(int& noise);
};

} // namespace ui

#endif /*PATCHCREATEDIALOG_H_*/
