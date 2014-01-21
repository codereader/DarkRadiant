#pragma once

#include <wx/xrc/xmlres.h>

namespace wxutil
{

/**
 * Base class for wxWindows that load all (or parts of their) controls
 * from an XRC file in the application's ui/ directory.
 */
class XmlResourceBasedWidget
{
protected:
	// Loads a named Panel from the XRC resources
	wxPanel* getNamedPanel(wxWindow* parent, const std::string& name)
	{
		wxPanel* panel = wxXmlResource::Get()->LoadPanel(parent, name);

		assert(panel != NULL);

		return panel;
	}
};

} // namespace
