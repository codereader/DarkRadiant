#ifndef _FILTER_DIALOG_H_
#define _FILTER_DIALOG_H_

#include "gtkutil/window/BlockingTransientWindow.h"
#include <map>

typedef struct _GtkTreeView GtkTreeView;
typedef struct _GtkListStore GtkListStore;
typedef struct _GtkTreeSelection GtkTreeSelection;

namespace ui {

class FilterDialog :
	public gtkutil::BlockingTransientWindow
{
	// The treeview listing all the filters
	GtkTreeView* _filterView;
	GtkListStore* _filterStore;

	std::map<int, GtkWidget*> _widgets;

	// Holds the name of the currently selected filter (or "" if none selected)
	std::string _selectedFilter;

	// Private constructor
	FilterDialog();

public:
	/** 
	 * greebo: Shows the dialog (command target)
	 */
	static void showDialog();

private:
	// Saves filter settings and exits
	void save();

	// Reload filter settings and fill widges
	void update();

	// This is called to create the widgets
	void populateWindow();

	GtkWidget* createButtonPanel();
	GtkWidget* createFiltersPanel();

	// Update buttons
	void updateWidgetSensitivity();

	static void onSave(GtkWidget* widget, FilterDialog* self);
	static void onCancel(GtkWidget* widget, FilterDialog* self);

	static void onAddFilter(GtkWidget* w, FilterDialog* self);
	static void onDeleteFilter(GtkWidget* w, FilterDialog* self);

	static void onFilterSelectionChanged(GtkTreeSelection* sel, FilterDialog* self);
};

} // namespace ui

#endif /* _FILTER_DIALOG_H_ */
