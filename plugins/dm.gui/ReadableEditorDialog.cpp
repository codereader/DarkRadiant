#include "ReadableEditorDialog.h"

#include "imainframe.h"
#include "gtkutil/MultiMonitor.h"
#include "gtkutil/dialog/MessageBox.h"

#include <gtk/gtktextview.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkstock.h>
#include <gtk/gtkbutton.h>

#include "string/string.h"


namespace ui
{

namespace 
{
	const std::string WINDOW_TITLE("Readable Editor");
	const std::string FIXUP_FILE_LABEL("blah blah");
}

ReadableEditorDialog::ReadableEditorDialog() :
	gtkutil::Dialog(WINDOW_TITLE, GlobalMainFrame().getTopLevelWindow())
{
	// Set size of the window, default size is too narrow for path entries
	GdkRectangle rect = gtkutil::MultiMonitor::getMonitorForWindow(
		GlobalMainFrame().getTopLevelWindow()
	);

	gtk_window_set_default_size(GTK_WINDOW(getWindow()), static_cast<gint>(rect.width*0.4f), -1);

	// Add the path entry
	_pathEntry = addPathEntry(FIXUP_FILE_LABEL, false);
}

void ReadableEditorDialog::RunDialog(const cmd::ArgumentList& args)
{
	ReadableEditorDialog dialog;

	if (dialog.run() == RESULT_OK)
	{
		/* ???
		std::string filename = "";

		// Run fixup script
		FixupMap fixup(filename);

		FixupMap::Result result = fixup.perform();

		// Show popup with results
		std::string msg;

		msg += "<b>" + sizetToStr(result.replacedShaders) + "</b> shaders replaced.\n";
		msg += "<b>" + sizetToStr(result.replacedEntities) + "</b> entities replaced.\n";
		msg += "<b>" + sizetToStr(result.replacedModels) + "</b> models replaced.\n";
		msg += "<b>" + sizetToStr(result.replacedMisc) + "</b> spawnargs replaced.\n";
		
		if (!result.errors.empty())
		{
			msg += "\n\nErrors occurred:\n";

			for (FixupMap::Result::ErrorMap::const_iterator i = result.errors.begin();
				 i != result.errors.end(); ++i)
			{
				msg += "(Line " + sizetToStr(i->first) + "): " + i->second + "\n";
			}
		}

		gtkutil::MessageBox resultBox("Fixup Results", msg, IDialog::MESSAGE_CONFIRM);
		resultBox.run();//*/
	}
}

} // namespace ui
