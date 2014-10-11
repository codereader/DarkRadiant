#include "clipboard.h"

#include <wx/clipbrd.h>

namespace wxutil
{

void copyToClipboard(const std::string& contents)
{
	if (wxTheClipboard->Open())
	{
		// This data objects are held by the clipboard, so do not delete them in the app.
		wxTheClipboard->SetData(new wxTextDataObject(contents));
		wxTheClipboard->Close();
	}
}

std::string pasteFromClipboard()
{
	std::string returnValue;

	if (wxTheClipboard->Open())
	{
		if (wxTheClipboard->IsSupported(wxDF_TEXT))
		{
			wxTextDataObject data;
			wxTheClipboard->GetData(data);
			returnValue = data.GetText().ToStdString();
		}

		wxTheClipboard->Close();
	}

	return returnValue;
}

} // namespace gtkutil
