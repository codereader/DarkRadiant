#include "CreateTrimDialog.h"

#include "i18n.h"
#include "icommandsystem.h"
#include "iselection.h"
#include "string/convert.h"
#include "selectionlib.h"
#include "wxutil/dialog/MessageBox.h"

namespace
{
	const char* WINDOW_TITLE = N_("Create Trim");
	const char* LABEL_HEIGHT = N_("Trim Height:");
	const char* LABEL_DEPTH = N_("Trim Depth:");
	const char* LABEL_FIT_TO = N_("Fit To:");
	const char* LABEL_MITERED = N_("45-degree mitered ends");

	const double DEFAULT_HEIGHT = 16;
	const double DEFAULT_DEPTH = 1;

	const char* FIT_BOTTOM = N_("Bottom");
	const char* FIT_TOP = N_("Top");
	const char* FIT_LEFT = N_("Left");
	const char* FIT_RIGHT = N_("Right");
}

namespace ui {

CreateTrimDialog::CreateTrimDialog() :
	wxutil::Dialog(_(WINDOW_TITLE))
{
	_heightHandle = addSpinButton(_(LABEL_HEIGHT), 1, 4096, 1, 0);
	_depthHandle = addSpinButton(_(LABEL_DEPTH), 1, 4096, 1, 0);

	ui::IDialog::ComboBoxOptions fitOptions;
	fitOptions.push_back(_(FIT_BOTTOM));
	fitOptions.push_back(_(FIT_TOP));
	fitOptions.push_back(_(FIT_LEFT));
	fitOptions.push_back(_(FIT_RIGHT));
	_fitToHandle = addComboBox(_(LABEL_FIT_TO), fitOptions);

	_miteredHandle = addCheckbox(_(LABEL_MITERED));

	setElementValue(_heightHandle, string::to_string(DEFAULT_HEIGHT));
	setElementValue(_depthHandle, string::to_string(DEFAULT_DEPTH));
	setElementValue(_fitToHandle, _(FIT_BOTTOM));
	setElementValue(_miteredHandle, "0");
}

bool CreateTrimDialog::QueryTrimParams(TrimParams& params)
{
	auto* dialog = new CreateTrimDialog;

	IDialog::Result result = dialog->run();

	if (result == IDialog::RESULT_OK)
	{
		params.height = string::convert<double>(dialog->getElementValue(dialog->_heightHandle));
		params.depth = string::convert<double>(dialog->getElementValue(dialog->_depthHandle));

		std::string fitToStr = dialog->getElementValue(dialog->_fitToHandle);

		if (fitToStr == _(FIT_TOP))
			params.fitTo = FitTo::Top;
		else if (fitToStr == _(FIT_LEFT))
			params.fitTo = FitTo::Left;
		else if (fitToStr == _(FIT_RIGHT))
			params.fitTo = FitTo::Right;
		else
			params.fitTo = FitTo::Bottom;

		params.mitered = (dialog->getElementValue(dialog->_miteredHandle) == "1");

		return true;
	}

	return false;
}

void CreateTrimDialog::CreateTrimCmd(const cmd::ArgumentList& args)
{
	if (GlobalSelectionSystem().getSelectionInfo().componentCount == 0)
	{
		wxutil::Messagebox::ShowError(_("Cannot create trim. No faces selected."));
		return;
	}

	TrimParams params;

	if (QueryTrimParams(params))
	{
		cmd::ArgumentList trimArgs;
		trimArgs.push_back(params.height);
		trimArgs.push_back(params.depth);
		trimArgs.push_back(static_cast<int>(params.fitTo));
		trimArgs.push_back(params.mitered ? 1 : 0);

		GlobalCommandSystem().executeCommand("CreateTrimForFaces", trimArgs);
	}
}

} // namespace
