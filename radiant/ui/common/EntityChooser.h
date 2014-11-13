#pragma once

#include "wxutil/dialog/DialogBase.h"
#include <memory>

#include "wxutil/TreeView.h"

namespace ui
{

class EntityChooser :
	public wxutil::DialogBase
{
private:
	struct EntityChooserColumns :
		public wxutil::TreeModel::ColumnRecord
	{
		EntityChooserColumns() :
			name(add(wxutil::TreeModel::Column::String))
		{}

		wxutil::TreeModel::Column name;
	};

	EntityChooserColumns _listColumns;
	wxutil::TreeModel::Ptr _entityStore;
	wxutil::TreeView* _entityView;

	std::string _selectedEntityName;

	EntityChooser();

public:
	std::string getSelectedEntity() const;
	void setSelectedEntity(const std::string& name);

	/**
	 * Static convenience method. Constructs a dialog with an EntityChooser
	 * and returns the selection.
	 *
	 * @preSelectedEntity: The entity name which should be selected by default.
	 * @returns: The name of the entity or an empty string if the user cancelled the dialog.
	 */
	static std::string ChooseEntity(const std::string& preSelectedEntity);

protected:
	void populateEntityList();
};

} // namespace ui
