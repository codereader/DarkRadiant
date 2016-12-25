#pragma once

#include "idialogmanager.h"
#include <list>

#include "wxutil/dialog/Dialog.h"

namespace ui
{

class DialogManager :
	public IDialogManager
{
private:
	typedef std::list<IDialogPtr> Dialogs;
	Dialogs _dialogs;

public:
	virtual ~DialogManager();

	// Create a new dialog
	IDialogPtr createDialog(const std::string& title,
							wxWindow* parent = nullptr) override;

	IDialogPtr createMessageBox(const std::string& title,
								const std::string& text,
								IDialog::MessageType type,
								wxWindow* parent = nullptr) override;

	IFileChooserPtr createFileChooser(const std::string& title, bool open, 
		const std::string& pattern, const std::string& defaultExt) override;

	IDirChooserPtr createDirChooser(const std::string& title) override;

	IResourceChooser* createSoundShaderChooser(wxWindow* parent) override;
	IAnimationChooser* createAnimationChooser(wxWindow* parent) override;

private:
	void cleanupOldDialogs();
};
typedef std::shared_ptr<DialogManager> DialogManagerPtr;

} // namespace ui
