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

		msg += (boost::format(_("%d shaders replaced.")) % result.replacedShaders).str() + "\n";
		msg += (boost::format(_("%d entities replaced.")) % result.replacedEntities).str() + "\n";
		msg += (boost::format(_("%d models replaced.")) % result.replacedModels).str() + "\n";
		msg += (boost::format(_("%d spawnargs replaced.")) % result.replacedMisc).str() + "\n";

		if (!result.errors.empty())
		{
			msg += "\n\n";
			msg += _("Errors occurred:");
			msg += "\n";

			for (FixupMap::Result::ErrorMap::const_iterator i = result.errors.begin();
				 i != result.errors.end(); ++i)
			{
				msg += (boost::format(_("(Line %d): %s")) % i->first % i->second).str();
				msg += "\n";
			}
		}

		wxutil::Messagebox::Show(_("Fixup Results"), msg, IDialog::MESSAGE_CONFIRM);
	}
}

} // namespace ui
