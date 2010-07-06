#ifndef AI_VOCAL_SET_CHOOSER_DIALOG_H_
#define AI_VOCAL_SET_CHOOSER_DIALOG_H_

#include "imodelpreview.h"

#include "gtkutil/window/BlockingTransientWindow.h"
#include <set>
#include <map>

typedef struct _GtkWidget GtkWidget;
typedef struct _GtkListStore GtkListStore;
typedef struct _GtkTreeSelection GtkTreeSelection;

namespace ui
{

class AIVocalSetChooserDialog :
	public gtkutil::BlockingTransientWindow
{
public:
	typedef std::set<std::string> SetList;

	enum Result
	{
		RESULT_OK,
		RESULT_CANCEL,
		NUM_RESULTS,
	};

private:
	GtkListStore* _setStore;
	GtkTreeSelection* _setSelection;

	// Widgets, access via enum values
	std::map<int, GtkWidget*> _widgets;

	// The name of the currently selected set
	std::string _selectedSet;

	static SetList _availableSets;

	Result _result;

public:
	AIVocalSetChooserDialog();

	// Set the selection to a given entityDef
	void setSelectedVocalSet(const std::string& setName);

	// Get the currently selected set (is empty when nothing is selected)
	std::string getSelectedVocalSet();

	// Get the result (whether the user clicked OK or Cancel)
	Result getResult();

private:
	void populateSetStore();

	GtkWidget* createButtonPanel();
	GtkWidget* createDescriptionPanel();

	// Searches all entity classes for available sets
	static void findAvailableSets();

	static void onSetSelectionChanged(GtkTreeSelection* sel, AIVocalSetChooserDialog* self);
	static void onOK(GtkWidget* widget, AIVocalSetChooserDialog* self);
	static void onCancel(GtkWidget* widget, AIVocalSetChooserDialog* self);
};

} // namespace ui

#endif /* AI_VOCAL_SET_CHOOSER_DIALOG_H_ */
