#pragma once

#include "idialogmanager.h"
#include "MessageBox.h"

namespace wxutil
{

// An adapter class which provides the IDialog interface for a Messagebox 
class MessageBoxDialogAdapter :
	public ui::IDialog
{
private:
	Messagebox* _msgBox;

public:
	MessageBoxDialogAdapter(const std::string& title,
			   const std::string& text,
			   ui::IDialog::MessageType type,
			   wxWindow* parent = NULL);

	virtual void setTitle(const std::string& title);

	virtual Handle addLabel(const std::string& text);
	virtual Handle addComboBox(const std::string& label, const ComboBoxOptions& options);
	virtual Handle addEntryBox(const std::string& label);
	virtual Handle addPathEntry(const std::string& label, bool foldersOnly = false);
	virtual Handle addSpinButton(const std::string& label, double min, double max, double step, unsigned int digits);
	virtual Handle addCheckbox(const std::string& label);

	virtual void setElementValue(const Handle& handle, const std::string& value);
	virtual std::string getElementValue(const Handle& handle);

	// Enter the main loop
	virtual ui::IDialog::Result run();
};

} // namespace
