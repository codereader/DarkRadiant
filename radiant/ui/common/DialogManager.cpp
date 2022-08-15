#include "DialogManager.h"

#include "itextstream.h"
#include "ui/imainframe.h"

#include "wxutil/dialog/MessageBox.h"
#include "wxutil/FileChooser.h"
#include "wxutil/DirChooser.h"
#include "ui/common/SoundChooser.h"
#include "ui/animationpreview/MD5AnimationChooser.h"
#include "module/StaticModule.h"

namespace ui
{

DialogManager::~DialogManager()
{
	if (!_dialogs.empty())
	{
		rMessage() << "DialogManager: " << _dialogs.size()
			<< " dialogs still in memory at shutdown." << std::endl;

		clear();
	}
}

const std::string& DialogManager::getName() const
{
    static std::string _name(MODULE_DIALOGMANAGER);
    return _name;
}

const StringSet& DialogManager::getDependencies() const
{
    static StringSet _dependencies
    {
        MODULE_MAINFRAME
    };

    return _dependencies;
}

void DialogManager::initialiseModule(const IApplicationContext& ctx)
{
    GlobalMainFrame().signal_MainFrameShuttingDown().connect(
        sigc::mem_fun(this, &DialogManager::clear));
}

void DialogManager::clear()
{
    _dialogs.clear();
}

IDialogPtr DialogManager::createDialog(const std::string& title, wxWindow* parent)
{
	cleanupOldDialogs();

	// Allocate a new dialog
	auto dialog = std::make_shared<wxutil::Dialog>(title, parent);

	_dialogs.push_back(dialog);

	return dialog;
}

IDialogPtr DialogManager::createMessageBox(const std::string& title,
										   const std::string& text,
										   IDialog::MessageType type,
										   wxWindow* parent)
{
	cleanupOldDialogs();

	// Allocate a new dialog, use the main window if no parent specified
	auto box = std::make_shared<wxutil::Messagebox>(title, text, type, parent);

	// Store it in the local map so that references are held
	_dialogs.push_back(box);

	return box;
}

IFileChooserPtr DialogManager::createFileChooser(const std::string& title,
	bool open, const std::string& pattern, const std::string& defaultExt)
{
	return IFileChooserPtr(new wxutil::FileChooser(
		GlobalMainFrame().getWxTopLevelWindow(),
		title, open, pattern, defaultExt));
}

IDirChooserPtr DialogManager::createDirChooser(const std::string& title)
{
	return IDirChooserPtr(new wxutil::DirChooser(GlobalMainFrame().getWxTopLevelWindow(), title));
}

void DialogManager::cleanupOldDialogs()
{
	for (auto i = _dialogs.begin(); i != _dialogs.end(); /* in-loop increment */)
	{
		if (i->use_count() <= 1)
		{
			_dialogs.erase(i++);
		}
		else
		{
			++i;
		}
	}
}

IResourceChooser* DialogManager::createSoundShaderChooser(wxWindow* parent)
{
	return new SoundChooser(parent);
}

IAnimationChooser* DialogManager::createAnimationChooser(wxWindow* parent)
{
	return new MD5AnimationChooser(parent);
}

module::StaticModuleRegistration<DialogManager> dialogManagerModule;

} // namespace ui
