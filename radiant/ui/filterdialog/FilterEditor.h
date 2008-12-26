#ifndef _FILTER_EDITOR_H_
#define _FILTER_EDITOR_H_

#include "gtkutil/window/BlockingTransientWindow.h"
#include <map>
#include "Filter.h"

typedef struct _GtkListStore GtkListStore;
typedef struct _GtkTreeView GtkTreeView;
typedef struct _GtkTreeSelection GtkTreeSelection;
typedef struct _GtkCellRendererText GtkCellRendererText;

namespace ui {

/**
 * greebo: UI for editing a single filter (name and criteria)
 */
class FilterEditor :
	public gtkutil::BlockingTransientWindow
{
	// The actual filter object to be edited
	Filter& _originalFilter;

	// The working copy of the Filter
	Filter _filter;

	std::map<int, GtkWidget*> _widgets;

	GtkTreeView* _criteriaView;

	GtkListStore* _criteriaStore;

public:
	// Constructor, pass the Filter object to be edited
	FilterEditor(Filter& filter, GtkWindow* parent);

private:
	void populateWindow();

	void save();

	void update();

	GtkWidget* createButtonPanel();
	GtkWidget* createCriteriaPanel();

	GtkListStore* createTypeStore();
	GtkListStore* createActionStore();

	// Converts the given string "entityclass", "object", etc. into an index in the typestore
	int getTypeIndexForString(const std::string& type);

	static void onSave(GtkWidget* widget, FilterEditor* self);
	static void onCancel(GtkWidget* widget, FilterEditor* self);

	static void onRegexEdited(GtkCellRendererText* renderer, gchar* path, gchar* new_text, FilterEditor* self);
	static void onTypeEdited(GtkCellRendererText* renderer, gchar* path, gchar* new_text, FilterEditor* self);
	static void onActionEdited(GtkCellRendererText* renderer, gchar* path, gchar* new_text, FilterEditor* self);

	static void onCriterionSelectionChanged(GtkTreeSelection* sel, FilterEditor* self);
};

} // namespace

#endif /* _FILTER_EDITOR_H_ */
