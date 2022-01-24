#include "AasControl.h"

#include "i18n.h"
#include "iarchive.h"
#include "ui/imainframe.h"
#include "ifilesystem.h"

#include <wx/event.h>
#include <wx/button.h>
#include <wx/bmpbuttn.h>
#include <wx/tglbtn.h>
#include <wx/sizer.h>
#include "wxutil/Bitmap.h"
#include <memory>

namespace ui
{

AasControl::AasControl(wxWindow* parent, const map::AasFileInfo& info) :
    _toggle(nullptr),
    _refreshButton(nullptr),
    _buttonHBox(nullptr),
    _updateActive(nullptr),
    _info(info)
{
    // Create the main toggle
	_toggle = new wxToggleButton(parent, wxID_ANY, info.type.fileExtension);
	_toggle->Connect(wxEVT_TOGGLEBUTTON, wxCommandEventHandler(AasControl::onToggle), NULL, this);

    _refreshButton = new wxBitmapButton(parent, wxID_ANY, 
		wxutil::GetLocalBitmap("refresh.png"));
	_refreshButton->Connect(wxEVT_BUTTON, wxCommandEventHandler(AasControl::onRefresh), NULL, this);
	_refreshButton->SetToolTip(_("Reload AAS File"));

	_buttonHBox = new wxBoxSizer(wxHORIZONTAL);
	_buttonHBox->Add(_refreshButton, 0, wxEXPAND);

    // Refresh the Control
	update();
}

AasControl::~AasControl()
{
    // Detach before destruction
    if (_toggle->GetValue())
    {
        _renderable.clear();
        GlobalRenderSystem().detachRenderable(_renderable);
    }
}

wxSizer* AasControl::getButtons()
{
    return _buttonHBox;
}

wxToggleButton* AasControl::getToggle()
{
    return _toggle;
}

void AasControl::update()
{

}

void AasControl::ensureAasFileLoaded()
{
    if (_aasFile) return;

    ArchiveTextFilePtr file = GlobalFileSystem().openTextFileInAbsolutePath(_info.absolutePath);

    if (file)
    {
        std::istream stream(&file->getInputStream());
        map::IAasFileLoaderPtr loader = GlobalAasFileManager().getLoaderForStream(stream);

        if (loader && loader->canLoad(stream))
        {
            stream.seekg(0, std::ios_base::beg);

            _aasFile = loader->loadFromStream(stream);

            // Construct a renderable to attach to the rendersystem
            _renderable.setAasFile(_aasFile);
        }
    }
}

void AasControl::onToggle(wxCommandEvent& ev)
{
    if (_toggle->GetValue())
    {
        ensureAasFileLoaded();

        _renderable.setAasFile(_aasFile);
        GlobalRenderSystem().attachRenderable(_renderable);
    }
    else
    {
        // Disable rendering
        _renderable.clear();
        GlobalRenderSystem().detachRenderable(_renderable);
    }

    GlobalMainFrame().updateAllWindows();
}

void AasControl::onRefresh(wxCommandEvent& ev)
{
    // Detach renderable
    _aasFile.reset();
    _renderable.clear();

    if (_toggle->GetValue())
    {
		GlobalRenderSystem().detachRenderable(_renderable);

        ensureAasFileLoaded();

        GlobalRenderSystem().attachRenderable(_renderable);
    }
}

} // namespace ui
