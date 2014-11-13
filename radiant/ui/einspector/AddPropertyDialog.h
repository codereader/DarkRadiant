#pragma once

#include <wx/dialog.h>

#include "wxutil/dialog/DialogBase.h"
#include "wxutil/TreeView.h"
#include "wxutil/XmlResourceBasedWidget.h"

#include <string>
#include "ientity.h"

class wxDataViewCtrl;

namespace ui
{

/** Modal dialog to display a list of known properties and allow the user
 * to choose one. The dialog is displayed via a single static method which
 * creates the dialog, blocks in a recursive main loop until the choice is
 * made, and then returns the string property that was selected.
 */
class AddPropertyDialog :
	public wxutil::DialogBase,
    private wxutil::XmlResourceBasedWidget
{
public:
	typedef std::vector<std::string> PropertyList;

	// Treemodel definition
	struct TreeColumns :
		public wxutil::TreeModel::ColumnRecord
	{
		TreeColumns() :
			displayName(add(wxutil::TreeModel::Column::IconText)),
			propertyName(add(wxutil::TreeModel::Column::String)),
			description(add(wxutil::TreeModel::Column::String))
		{}

		wxutil::TreeModel::Column displayName;
		wxutil::TreeModel::Column propertyName;
		wxutil::TreeModel::Column description;
	};

private:
	// Tree view, selection and model
	TreeColumns _columns;
	wxutil::TreeModel::Ptr _treeStore;
	wxutil::TreeView* _treeView;

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
	void _onSelectionChanged(wxDataViewEvent& ev);
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
