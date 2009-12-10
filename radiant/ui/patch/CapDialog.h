#ifndef _UI_PATCH_CAP_DIALOG_H_
#define _UI_PATCH_CAP_DIALOG_H_

#include "gtkutil/dialog/Dialog.h"
#include <map>
#include "patch/PatchConstants.h"

typedef struct _GtkTable GtkTable;
typedef struct _GSList GSList;

/** 
 * Query the user which type of cap should be created
 */
namespace ui
{

class PatchCapDialog :
	public gtkutil::Dialog
{
private:
	EPatchCap _selectedCapType;

	// Linked list of radio buttons
	GSList* _radioButtonGroup;

	typedef std::map<EPatchCap, GtkWidget*> RadioButtons;
	RadioButtons _radioButtons;

public:
	// Constructor 
	PatchCapDialog();
	
	// Returns the selected cap type (only valid if dialog result == OK)
	EPatchCap getSelectedCapType();

private:
	void addItemToTable(GtkTable* table, const std::string& image, guint row, EPatchCap type);
};

} // namespace ui

#endif /* _UI_PATCH_CAP_DIALOG_H_ */
