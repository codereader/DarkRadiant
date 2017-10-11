#include "FixupMapDialog.h"

#include "i18n.h"
#include "imainframe.h"
#include "wxutil/dialog/MessageBox.h"

#include "string/string.h"
#include "FixupMap.h"

namespace ui
{

namespace
{
	const char* const WINDOW_TITLE = N_("Fixup Map");
	const char* const FIXUP_FILE_LABEL = N_("Fixup File");
}

FixupMapDialog::FixupMapDialog() :
	Dialog(_(WINDOW_TITLE))
{
	// Add the path entry
	_pathEntry = addPathEntry(FIXUP_FILE_LABEL, false);
}

std::string FixupMapDialog::getFixupFilePath()
{
	return getElementValue(_pathEntry);
}

void FixupMapDialog::RunDialog(const cmd::ArgumentList& args)
{
	FixupMapDialog dialog;

	if (dialog.run() == RESULT_OK)
	{
		std::string filename = dialog.getFixupFilePath();

		// Run fixup script
		FixupMap fixup(filename);

		FixupMap::Result result = fixup.perform();

		// Show popup with results
		std::string msg;

		msg += fmt::format(_("{0} shaders replaced."), result.replacedShaders) + "\n";
		msg += fmt::format(_("{0} entities replaced."), result.replacedEntities) + "\n";
		msg += fmt::format(_("{0} models replaced."), result.replacedModels) + "\n";
		msg += fmt::format(_("{0} spawnargs replaced."), result.replacedMisc) + "\n";

		if (!result.errors.empty())
		{
			msg += "\n\n";
			msg += _("Errors occurred:");
			msg += "\n";

			for (FixupMap::Result::ErrorMap::const_iterator i = result.errors.begin();
				 i != result.errors.end(); ++i)
			{
				msg += fmt::format(_("(Line {0}): {1}"), i->first, i->second);
				msg += "\n";
			}
		}

		wxutil::Messagebox::Show(_("Fixup Results"), msg, IDialog::MESSAGE_CONFIRM);
	}
}

} // namespace ui
