#ifndef _ENTITY_CHOOSER_H_
#define _ENTITY_CHOOSER_H_

#include "gtkutil/ifc/Widget.h"
#include "gtkutil/dialog/DialogElements.h"
#include <map>

typedef struct _GtkListStore GtkListStore;
typedef struct _GtkTreeSelection GtkTreeSelection;

namespace ui
{

class EntityChooser :
	public gtkutil::DialogElement
{
private:
	GtkListStore* _entityStore;
	GtkTreeSelection* _selection;

	std::map<int, GtkWidget*> _widgets;

	std::string _selectedEntityName;

public:
	EntityChooser();

	std::string getSelectedEntity() const;
	void setSelectedEntity(const std::string& name);

	// Implementation of StringSerialisable
	virtual std::string exportToString() const;
	virtual void importFromString(const std::string& str);

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

	static void onSelectionChanged(GtkTreeSelection* sel, EntityChooser* self);
};
typedef boost::shared_ptr<EntityChooser> EntityChooserPtr;

} // namespace ui

#endif /* _ENTITY_CHOOSER_H_ */
