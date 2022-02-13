#include "ClipboardModule.h"

#include "itextstream.h"
#include "math/Hash.h"
#include <wx/clipbrd.h>
#include <wx/app.h>

#include "module/StaticModule.h"

namespace ui
{

namespace
{
    inline std::string getContentHash(const std::string& content)
    {
        // Inspect the clipboard when the main window regains focus
        // and fire the event if the contents changed
        math::Hash hash;
        hash.addString(content);

        return hash;
    }
}

std::string ClipboardModule::getString()
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

void ClipboardModule::setString(const std::string& str)
{
	if (wxTheClipboard->Open())
	{
		// This data objects are held by the clipboard, so do not delete them in the app.
		wxTheClipboard->SetData(new wxTextDataObject(str));
		wxTheClipboard->Close();

        _contentHash = getContentHash(str);

        // Contents changed signal
        _sigContentsChanged.emit();
	}
}

sigc::signal<void>& ClipboardModule::signal_clipboardContentChanged()
{
    return _sigContentsChanged;
}

const std::string& ClipboardModule::getName() const
{
	static std::string _name(MODULE_CLIPBOARD);
	return _name;
}

const StringSet& ClipboardModule::getDependencies() const
{
    static StringSet _dependencies;
	return _dependencies;
}

void ClipboardModule::initialiseModule(const IApplicationContext& ctx)
{
	rMessage() << getName() << "::initialiseModule called." << std::endl;

    wxTheApp->Bind(wxEVT_ACTIVATE_APP, &ClipboardModule::onAppActivated, this);
}

void ClipboardModule::shutdownModule()
{
    wxTheApp->Unbind(wxEVT_ACTIVATE_APP, &ClipboardModule::onAppActivated, this);
}

void ClipboardModule::onAppActivated(wxActivateEvent& ev)
{
    if (ev.GetActive())
    {
        // Inspect the clipboard when the main window regains focus
        // and fire the event if the contents changed
        auto newHash = getContentHash(getString());

        if (newHash != _contentHash)
        {
            _contentHash.swap(newHash);
            _sigContentsChanged.emit();
        }
    }

    ev.Skip();
}

module::StaticModuleRegistration<ClipboardModule> clipboardModule;

}
