#include "AboutDialog.h"

#include "i18n.h"
#include "igl.h"
#include "imainframe.h"
#include "iuimanager.h"
#include "version.h"
#include "registry/registry.h"
#include "string/string.h"

#include <wx/panel.h>
#include <wx/stattext.h>
#include <wx/button.h>
#include <wx/textctrl.h>

#include "modulesystem/ModuleRegistry.h"
#include <fmt/format.h>

namespace ui {

	namespace
	{
#if WIN32
		const char* const RKEY_SHOW_BUILD_TIME = "user/showBuildTime";
#endif
		const char* const WINDOW_TITLE = N_("About DarkRadiant");
	}

AboutDialog::AboutDialog() :
	DialogBase(_(WINDOW_TITLE), GlobalMainFrame().getWxTopLevelWindow())
{
	SetWindowStyleFlag(GetWindowStyleFlag() & ~wxRESIZE_BORDER);

	// Create all the widgets
	populateWindow();

	Connect(wxEVT_CLOSE_WINDOW, wxCloseEventHandler(AboutDialog::_onDeleteEvent), NULL, this);

	Fit();
	CenterOnScreen();
}

void AboutDialog::populateWindow()
{
	loadNamedPanel(this, "AboutDialogPanel");

	wxStaticText* appTitle = findNamedObject<wxStaticText>(this, "AboutDialogAppTitle");
	wxFont appTitleFont = appTitle->GetFont().Bold();
	appTitleFont.SetPointSize(appTitleFont.GetPointSize() + 4);
	appTitle->SetFont(appTitleFont);
	appTitle->SetLabel(RADIANT_APPNAME_FULL());

	wxStaticText* buildDateText = findNamedObject<wxStaticText>(this, "AboutDialogBuildDate");

#if WIN32
	std::string date = __DATE__;
	std::string time = __TIME__;

	bool showBuildTime = registry::getValue<bool>(RKEY_SHOW_BUILD_TIME);
	std::string buildDate = (showBuildTime) ? date + " " + time : date;
	std::string buildDateStr = fmt::format(_("Build date: {0}"), buildDate);

	buildDateText->SetLabel(buildDateStr);
#else
	wxSizer* sizer = buildDateText->GetContainingSizer();
	buildDateText->Destroy();
	sizer->Layout();
#endif

	std::string wxVersion = fmt::format(_("Version: {0:d}.{1:d}.{2:d}"),
		wxMAJOR_VERSION, wxMINOR_VERSION, wxRELEASE_NUMBER);
	
	findNamedObject<wxStaticText>(this, "AboutDialogWxWidgetsVersion")->SetLabel(wxVersion);

	// If anybody knows a better method to convert glubyte* to char*, please tell me...
	std::string vendorStr = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
	std::string versionStr = reinterpret_cast<const char*>(glGetString(GL_VERSION));
	std::string rendererStr = reinterpret_cast<const char*>(glGetString(GL_RENDERER));

	std::string openGLVendor = fmt::format(_("Vendor: {0}"), vendorStr);
	std::string openGLVersion = fmt::format(_("Version: {0}"), versionStr);
	std::string openGLRenderer = fmt::format(_("Renderer: {0}"), rendererStr);

	findNamedObject<wxStaticText>(this, "AboutDialogOpenGLVendor")->SetLabel(openGLVendor);
	findNamedObject<wxStaticText>(this, "AboutDialogOpenGLVersion")->SetLabel(openGLVersion);
	findNamedObject<wxStaticText>(this, "AboutDialogOpenGLRenderer")->SetLabel(openGLRenderer);
	
	std::string openGLExtensions = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));

	findNamedObject<wxTextCtrl>(this, "AboutDialogOpenGLExtensions")->SetValue(openGLExtensions);
	
	// DarkRadiant modules

	std::string modules = module::ModuleRegistry::Instance().getModuleList(", ");
	findNamedObject<wxTextCtrl>(this, "AboutDialogDarkRadiantModules")->SetValue(modules);

	findNamedObject<wxButton>(this, "AboutDialogOkButton")->Connect(
		wxEVT_BUTTON, wxCommandEventHandler(AboutDialog::_onClose), NULL, this);

	// Make all headers bold
	wxFont bold = findNamedObject<wxStaticText>(this, "AboutDialogHeader1")->GetFont().Bold();
	findNamedObject<wxStaticText>(this, "AboutDialogHeader1")->SetFont(bold);
	findNamedObject<wxStaticText>(this, "AboutDialogHeader2")->SetFont(bold);
	findNamedObject<wxStaticText>(this, "AboutDialogHeader3")->SetFont(bold);
	findNamedObject<wxStaticText>(this, "AboutDialogHeader4")->SetFont(bold);
}

void AboutDialog::_onClose(wxCommandEvent& ev)
{
	EndModal(wxID_OK);
}

void AboutDialog::_onDeleteEvent(wxCloseEvent& ev)
{
	EndModal(wxID_OK);
}

void AboutDialog::showDialog(const cmd::ArgumentList& args)
{
	AboutDialog* dialog = new AboutDialog;
	dialog->ShowModal();
	dialog->Destroy();
}

} // namespace ui
