#include "BulgePatchDialog.h"

#include "i18n.h"
#include "imainframe.h"
#include "string/convert.h"

namespace
{
	const char* WINDOW_TITLE = N_("Bulge Patch");
	const char* LABEL_NOISE = N_("Noise:");

	const int NOISE = 16;
}

namespace ui {

BulgePatchDialog::BulgePatchDialog() :
	wxutil::Dialog(_(WINDOW_TITLE))
{
	_noiseHandle = addEntryBox(_(LABEL_NOISE));

	setElementValue(_noiseHandle, string::to_string(NOISE));
}

bool BulgePatchDialog::QueryPatchNoise(int& noise)
{
	// Instantiate a dialog and run the dialog routine
	BulgePatchDialog* dialog = new BulgePatchDialog;

	IDialog::Result result = dialog->run();

	if (result == IDialog::RESULT_OK)
	{
		// Retrieve the maxValue
		noise = string::convert<float>(dialog->getElementValue(dialog->_noiseHandle));

		return true;
	}

	return false; // cancelled
}

} // namespace
