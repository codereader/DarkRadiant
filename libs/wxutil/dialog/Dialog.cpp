#include "Dialog.h"

#include "ui/imainframe.h"
#include "itextstream.h"

#include "DialogElements.h"
#include "../EntryAbortedException.h"

#include <wx/frame.h>
#include <wx/sizer.h>

namespace wxutil
{

Dialog::Dialog(const std::string& title, wxWindow* parent) :
	_dialog(new wxutil::DialogBase(title, parent != nullptr ? parent : GlobalMainFrame().getWxTopLevelWindow())),
	_result(RESULT_CANCELLED),
	_elementsTable(new wxFlexGridSizer(1, 2, 6, 12)), // Nx2 table
	_constructed(false),
	_highestUsedHandle(0),
    _focusWidget(0)
{
	_elementsTable->AddGrowableCol(1);
	_dialog->SetSizer(new wxBoxSizer(wxVERTICAL));

	_dialog->GetSizer()->Add(_elementsTable, 1, wxEXPAND | wxALL, 12);
}

Dialog::~Dialog()
{
	// wxWidgets is responsible of deleting the dialog from this point
	_dialog->Destroy();
}

void Dialog::setTitle(const std::string& title)
{
	// Dispatch this call to the base class
	_dialog->SetTitle(title);
}

void Dialog::setFocus(Handle handle)
{
    _focusWidget = handle;
}

void Dialog::setDefaultSize(int width, int height)
{
	_dialog->SetSize(width, height);
}

wxWindow* Dialog::getElementParent()
{
	return _dialog;
}

void Dialog::createButtons()
{
	// Buttons are added to the dialog last
	_dialog->GetSizer()->Add(_dialog->CreateStdDialogButtonSizer(wxOK | wxCANCEL), 0, 
		wxALIGN_RIGHT | wxBOTTOM | wxLEFT | wxRIGHT, 12);
}

void Dialog::construct()
{
	// Call the virtual method
	createButtons();
}

ui::IDialog::Handle Dialog::addElement(const DialogElementPtr& element)
{
	wxStaticText* first = element->getLabel();
	wxWindow* second = element->getValueWidget();

	if (first == NULL && second == NULL) return ui::INVALID_HANDLE;

	// At least one of the widgets is non-NULL

	// Acquire a new handle
	Handle handle = ++_highestUsedHandle;

	// Store this element in the map
	_elements[handle] = element;

	int numRows = static_cast<int>(_elements.size());

	// Push the widgets into the dialog, resize the table to fit
	_elementsTable->SetRows(numRows);

	if (first != second)
	{
		// Widgets are not equal, check for NULL-ness
		if (second == NULL)
		{
			// One single widget
			_elementsTable->Add(first, 1, wxEXPAND);
			_elementsTable->Add(new wxStaticText(_dialog, wxID_ANY, ""));
		}
		else if (first == NULL)
		{
			// One single widget
			_elementsTable->Add(new wxStaticText(_dialog, wxID_ANY, ""));
			_elementsTable->Add(second, 1, wxEXPAND);
		}
		else // Both are non-NULL
		{
			// The label (first column)
			_elementsTable->Add(first, 0, wxALIGN_CENTER_VERTICAL);

			// The edit widgets (second column)
			_elementsTable->Add(second, 1, wxEXPAND);
		}
	}
	else // The widgets are the same, non-NULL
	{
		_elementsTable->Add(first, 1);
		_elementsTable->Add(new wxStaticText(_dialog, wxID_ANY, ""));
	}

	return handle;
}

ui::IDialog::Handle Dialog::addLabel(const std::string& text)
{
	return addElement(DialogElementPtr(new DialogLabel(_dialog, text)));
}

ui::IDialog::Handle Dialog::addComboBox(const std::string& label, const ComboBoxOptions& options)
{
	return addElement(DialogComboBoxPtr(new DialogComboBox(_dialog, label, options)));
}

ui::IDialog::Handle Dialog::addEntryBox(const std::string& label)
{
	return addElement(DialogElementPtr(new DialogEntryBox(_dialog, label)));
}

ui::IDialog::Handle Dialog::addPathEntry(const std::string& label, bool foldersOnly)
{
	return addElement(DialogElementPtr(new DialogPathEntry(_dialog, label, foldersOnly)));
}

ui::IDialog::Handle Dialog::addSpinButton(const std::string& label, double min, double max, double step, unsigned int digits)
{
	return addElement(DialogElementPtr(new DialogSpinButton(_dialog, label, min, max, step, digits)));
}

ui::IDialog::Handle Dialog::addCheckbox(const std::string& label)
{
	return addElement(DialogElementPtr(new DialogCheckBox(_dialog, label)));
}

void Dialog::setElementValue(const ui::IDialog::Handle& handle, const std::string& value)
{
	ElementMap::iterator i = _elements.find(handle);

	if (i == _elements.end())
	{
		rError() << "Dialog: cannot find element with handle " << handle << std::endl;
		return;
	}

	// Import the data from the string into the widget
	i->second->importFromString(value);
}

std::string Dialog::getElementValue(const ui::IDialog::Handle& handle)
{
	ElementMap::iterator i = _elements.find(handle);

	if (i == _elements.end())
	{
		rError() << "Dialog: cannot find element with handle " << handle << std::endl;
		return "";
	}

	// Export the widget's contents to a string
	return i->second->exportToString();
}

ui::IDialog::Result Dialog::run()
{
	if (!_constructed)
	{
		_constructed = true;

		// Call the virtual method, gives subclasses a chance to do their stuff
		construct();
	}

	_dialog->Fit();
	_dialog->CenterOnParent();

    // Set the focus widget
    ElementMap::const_iterator found = _elements.find(_focusWidget);

    if (found != _elements.end() && found->second->getValueWidget())
    {
        found->second->getValueWidget()->SetFocus();
    }

	// Show the dialog (enters main loop and blocks)
	int result = _dialog->ShowModal();

	switch (result)
	{
		case wxID_OK: 
			_result = RESULT_OK;
			break;
		case wxID_CANCEL:
		default:
			_result = RESULT_CANCELLED;
			break;
	};

	return _result;
}

std::string Dialog::TextEntryDialog(const std::string& title,
								    const std::string& prompt,
								    const std::string& defaultText,
								    wxWindow* mainFrame)
{
	Dialog dialog(title, mainFrame);

	Dialog::Handle entryHandle = dialog.addEntryBox(prompt);

    // Set the default value and focus on the entry widget
	dialog.setElementValue(entryHandle, defaultText);
    dialog.setFocus(entryHandle);

	Dialog::Result result = dialog.run();

	if (result == Dialog::RESULT_OK)
	{
		std::string returnValue = dialog.getElementValue(entryHandle);
		return returnValue;
	}
    else
	{
        throw EntryAbortedException("textEntryDialog(): dialog cancelled");
	}
}

} // namespace wxutil
