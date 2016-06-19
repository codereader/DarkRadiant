#pragma once

#include "iradiant.h"
#include "icommandsystem.h"

#include "PrefPage.h"

class wxTreebook;

namespace wxutil { class DialogBase; }

namespace ui
{

class PrefDialog;
typedef std::shared_ptr<PrefDialog> PrefDialogPtr;

class PrefDialog
{
private:
	// The actual dialog instance
	wxutil::DialogBase* _dialog;

	wxTreebook* _notebook;
	wxBoxSizer* _mainVbox;

	// The root page
	PrefPagePtr _root;

public:
	PrefDialog();

	// Retrieve a reference to the static instance of this dialog
	static PrefDialog& Instance();

	/** greebo: Runs the modal dialog
	 */
	static void ShowDialog(const cmd::ArgumentList& args);

	/** greebo: Makes sure that the dialog is visible.
	 * 			(does nothing if the dialog is already on screen)
	 */
	static void ShowModal(const std::string& path = "");

	/** greebo: The command target to show the Game settings preferences.
	 */
	static void ShowProjectSettings(const cmd::ArgumentList& args);

	/** greebo: Looks up the page for the path and creates it
	 * 			if necessary.
	 */
	PrefPagePtr createOrFindPage(const std::string& path);

	/** greebo: A safe shutdown request that saves the window information
	 * 			to the registry.
	 */
	void onRadiantShutdown();
	
	/** greebo: Displays the page with the specified path.
	 *
	 * @path: a string like "Settings/Patches"
	 */
	void showPage(const std::string& path);

private:
	void doShowModal(const std::string& requestedPage);

	// This is where the static shared_ptr of the singleton instance is held.
	static PrefDialogPtr& InstancePtr();

	void createDialog(wxWindow* parent);
	void destroyDialog();

	void createTreebook();
};

} // namespace ui
