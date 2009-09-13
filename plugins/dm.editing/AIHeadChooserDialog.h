#ifndef AI_HEAD_CHOOSER_DIALOG_H_
#define AI_HEAD_CHOOSER_DIALOG_H_

#include "imodelpreview.h"

#include "gtkutil/window/BlockingTransientWindow.h"
#include <set>
#include <map>

typedef struct _GtkWidget GtkWidget;
typedef struct _GtkListStore GtkListStore;
typedef struct _GtkTreeSelection GtkTreeSelection;

namespace ui
{

class AIHeadChooserDialog :
	public gtkutil::BlockingTransientWindow
{
public:
	typedef std::set<std::string> HeadList;

	enum Result
	{
		RESULT_OK,
		RESULT_CANCEL,
		NUM_RESULTS,
	};

private:
	GtkListStore* _headStore;
	GtkTreeSelection* _headSelection;

	// The model preview
	IModelPreviewPtr _preview;

	// Widgets, access via enum values
	std::map<int, GtkWidget*> _widgets;

	// The name of the currently selected head
	std::string _selectedHead;

	static HeadList _availableHeads;

	Result _result;

public:
	AIHeadChooserDialog();

	// Set the selection to a given entityDef
	void setSelectedHead(const std::string& headDef);

	// Get the currently selected head (is empty when nothing is selected)
	std::string getSelectedHead();

	// Get the result (whether the user clicked OK or Cancel)
	Result getResult();

private:
	// Override base class _preDestroy
	void _preDestroy();
	void _postShow();
	
	void populateHeadStore();

	GtkWidget* createButtonPanel();

	// Searches all entity classes for available heads
	static void findAvailableHeads();

	static void onHeadSelectionChanged(GtkTreeSelection* sel, AIHeadChooserDialog* self);
	static void onOK(GtkWidget* widget, AIHeadChooserDialog* self);
	static void onCancel(GtkWidget* widget, AIHeadChooserDialog* self);
};

} // namespace ui

#endif /* AI_HEAD_CHOOSER_DIALOG_H_ */
