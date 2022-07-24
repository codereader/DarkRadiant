#pragma once

#include "icommandsystem.h"

#include "wxutil/dialog/DialogBase.h"
#include "wxutil/dataview/TreeView.h"
#include "wxutil/dataview/DeclarationTreeView.h"

class EntityClassAttribute;

namespace ui
{

class EClassTree :
	public wxutil::DialogBase
{
private:
	// The EClass treeview widget and underlying liststore
    wxutil::DeclarationTreeView::Columns _eclassColumns;
	wxutil::DeclarationTreeView* _eclassView;

	struct PropertyListColumns :
		public wxutil::TreeModel::ColumnRecord
	{
		PropertyListColumns() :
			name(add(wxutil::TreeModel::Column::String)),
			value(add(wxutil::TreeModel::Column::String)),
			inherited(add(wxutil::TreeModel::Column::Boolean))
		{}

		wxutil::TreeModel::Column name;
		wxutil::TreeModel::Column value;
		wxutil::TreeModel::Column inherited;
	};

	// The treeview and liststore for the property pane
	PropertyListColumns _propertyColumns;
	wxutil::TreeModel::Ptr _propertyStore;
	wxutil::TreeView* _propertyView;

	// Private constructor, traverses the entity classes
	EClassTree();

public:
	// Shows the singleton class (static command target)
	static void ShowDialog(const cmd::ArgumentList& args);

private:
	// Constructs and adds all the dialog widgets
	void populateWindow();

	wxWindow* createEClassTreeView(wxWindow* parent); // EClass Tree
	void createPropertyTreeView(wxWindow* parent); // Property Tree

	// Loads the spawnargs into the right treeview
    void addToListStore(const EntityClassAttribute& attr, bool inherited);
	void updatePropertyView(const std::string& eclassName);

	void handleSelectionChange();

	// callbacks
	void onSelectionChanged(wxDataViewEvent& ev);
    void onTreeViewPopulationFinished(wxutil::ResourceTreeView::PopulationFinishedEvent& ev);
};

} // namespace ui
