#pragma once

#include "iradiant.h"
#include "icommandsystem.h"
#include "wxutil/dialog/DialogBase.h"
#include <memory>
#include "wxutil/TreeView.h"
#include <memory>

class EntityClassAttribute;

namespace ui
{

class EClassTreeBuilder;

class EClassTree;
typedef std::shared_ptr<EClassTree> EClassTreePtr;

struct EClassTreeColumns :
	public wxutil::TreeModel::ColumnRecord
{
	EClassTreeColumns() :
		name(add(wxutil::TreeModel::Column::IconText))
	{}

	wxutil::TreeModel::Column name;	// name
};

class EClassTree :
	public wxutil::DialogBase
{
private:
	// The EClass treeview widget and underlying liststore
	EClassTreeColumns _eclassColumns;
	wxutil::TreeModel::Ptr _eclassStore;

	wxutil::TreeView* _eclassView;

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

	std::unique_ptr<EClassTreeBuilder> _treeBuilder;

	// Private constructor, traverses the entity classes
	EClassTree();

public:
	// Shows the singleton class (static command target)
	static void ShowDialog(const cmd::ArgumentList& args);

private:
	// Constructs and adds all the dialog widgets
	void populateWindow();

	void createEClassTreeView(wxWindow* parent); // EClass Tree
	void createPropertyTreeView(wxWindow* parent); // Property Tree

	// Loads the spawnargs into the right treeview
    void addToListStore(const EntityClassAttribute& attr);
	void updatePropertyView(const std::string& eclassName);

	void handleSelectionChange();

	// callbacks
	void onSelectionChanged(wxDataViewEvent& ev);
	void onTreeStorePopulationFinished(wxutil::TreeModel::PopulationFinishedEvent& ev);
};

} // namespace ui
