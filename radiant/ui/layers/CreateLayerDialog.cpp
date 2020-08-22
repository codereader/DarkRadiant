#include "CreateLayerDialog.h"

#include "imap.h"
#include "i18n.h"
#include "wxutil/EntryAbortedException.h"
#include "wxutil/dialog/Dialog.h"
#include "wxutil/dialog/MessageBox.h"

namespace ui
{

void CreateLayerDialog::CreateNewLayer(const cmd::ArgumentList& args)
{
	if (!GlobalMapModule().getRoot())
	{
		rError() << "No map loaded, cannot create a layer." << std::endl;
		return;
	}

	std::string initialName = !args.empty() ? args[0].getString() : "";

	while (true)
	{
		// Query the name of the new layer from the user
		std::string layerName;

		if (!initialName.empty())
		{
			// If we got a layer name passed through the arguments,
			// we use this one, but only the first time
			layerName = initialName;
			initialName.clear();
		}

		if (layerName.empty())
		{
			try
			{
				layerName = wxutil::Dialog::TextEntryDialog(
					_("Enter Name"),
					_("Enter Layer Name"),
					"",
					GlobalMainFrame().getWxTopLevelWindow()
				);
			}
			catch (wxutil::EntryAbortedException&)
			{
				break;
			}
		}

		if (layerName.empty())
		{
			// Wrong name, let the user try again
			wxutil::Messagebox::ShowError(_("Cannot create layer with empty name."));
			continue;
		}

		// Attempt to create the layer, this will return -1 if the operation fails
		int layerID = GlobalMapModule().getRoot()->getLayerManager().createLayer(layerName);

		if (layerID != -1)
		{
			// Success, break the loop
			break;
		}
		else
		{
			// Wrong name, let the user try again
			wxutil::Messagebox::ShowError(_("This name already exists."));
			continue;
		}
	}
}

}
