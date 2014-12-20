#pragma once

#include <wx/dialog.h>
#include <wx/treelist.h>
#include <wx/windowptr.h>

#include "wxutil/dialog/DialogBase.h"
#include "wxutil/XmlResourceBasedWidget.h"

#include <string>
#include "ientity.h"

class wxDataViewCtrl;

namespace ui
{

/**
 * \brief
 * Modal dialog to display a list of known properties and allow the user to
 * choose one.
 *
 * The dialog is displayed via a single static method which creates the dialog,
 * blocks in a recursive main loop until the choice is made, and then returns
 * the string property that was selected.
 */
class AddPropertyDialog :
	public wxutil::DialogBase,
    private wxutil::XmlResourceBasedWidget
{
public:
	typedef std::vector<std::string> PropertyList;

private:
	// Tree view
	wxWindowPtr<wxTreeListCtrl> _treeView;

	// The selected properties
	PropertyList _selectedProperties;

	// Target entity to query for existing spawnargs
	Entity* _entity;

private:

    // Set up tree model and columns
    void setupTreeView();

	// Populate tree view with properties
	void populateTreeView();

	void updateUsagePanel();

	void _onOK(wxCommandEvent& ev);
	void _onCancel(wxCommandEvent& ev);
	void _onSelectionChanged(wxTreeListEvent& ev);
	void _onDeleteEvent(wxCloseEvent& ev);

	/* Private constructor creates the dialog widgets. Accepts an Entity
	 * to use for populating class-specific keys.
	 */
	AddPropertyDialog(Entity* entity);

public:

	/**
	 * Static method to display an AddPropertyDialog and return the chosen
	 * property.
	 *
	 * @param entity
	 * The Entity to be queried for spawnargs.
	 *
	 * @returns
	 * String list of the chosen properties (e.g. "light_radius").
	 */
	static PropertyList chooseProperty(Entity* entity);
};

} // namespace
