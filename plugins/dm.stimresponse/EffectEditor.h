#ifndef EFFECTEDITOR_H_
#define EFFECTEDITOR_H_

#include "gtkutil/DialogWindow.h"

#include "StimResponse.h"
#include "ResponseEffectTypes.h"
#include "EffectArgumentItem.h"
#include <boost/shared_ptr.hpp>

// Forward Declarations
typedef struct _GtkWindow GtkWindow;
typedef struct _GtkWidget GtkWidget;
typedef struct _GtkListStore GtkListStore;
typedef struct _GtkTooltips GtkTooltips;
typedef struct _GtkToggleButton GtkToggleButton;

namespace ui {

class ResponseEditor;

class EffectEditor :
	public gtkutil::DialogWindow
{
	// The overall vbox
	GtkWidget* _dialogVBox;
	
	// The container holding the argument widget table
	GtkWidget* _argAlignment;
	
	GtkWidget* _argTable;
	
	// The tooltips group to display the help text
	GtkTooltips* _tooltips;
	
	// The list containing the possible effect types
	ResponseEffectTypeMap _effectTypes;
	
	GtkWidget* _effectTypeCombo;
	GtkListStore* _effectStore;
	
	// The entity list store
	GtkListStore* _entityStore;
	
	// The list of argument items
	typedef boost::shared_ptr<EffectArgumentItem> ArgumentItemPtr;
	typedef std::vector<ArgumentItemPtr> ArgumentItemList;
	ArgumentItemList _argumentItems;
	
	GtkWidget* _stateToggle;
	
	// The references to the object we're editing here 
	StimResponse& _response;
	unsigned int _effectIndex;
	
	// For calling update() when finished editing
	ResponseEditor& _editor;
	
public:
	/** greebo: Constructor, needs information about parent and the edit target. 
	 * 
	 * @parent: The parent this window is child of.
	 * 
	 * @response: The Stim/Response object the effect is associated with
	 * 			  (this should be a response, although stims work as well).
	 * 
	 * @effectIndex: The response effect index within the given Response. 
	 */
	EffectEditor(GtkWindow* parent, 
				 StimResponse& response, 
				 const unsigned int effectIndex,
				 ResponseEditor& editor);
	
	/** greebo: Creates the widgets 
	 */
	void populateWindow();
	
private:
	/** greebo: Gets called on effect type changes to update the argument
	 * 			widgets accordingly.
	 */
	void effectTypeChanged();

	/** greebo: Populate the entity list store by traversing the 
	 * 			scene graph searching for entities. The names of 
	 * 			the entities are stored into the member _entityStore
	 */
	void populateEntityListStore();
	
	/** greebo: Saves the widget contents into the arguments.
	 */
	void save();

	/** greebo: Parses the response effect for necessary arguments
	 * 			and creates the according widgets.
	 */
	void createArgumentWidgets(ResponseEffect& effect);
	
	static void onEffectTypeChange(GtkWidget* combobox, EffectEditor* self);
	static void onStateToggle(GtkToggleButton* toggleButton, EffectEditor* self);
	static void onSave(GtkWidget* button, EffectEditor* self);
	static void onCancel(GtkWidget* button, EffectEditor* self);

}; // class EffectEditor

} // namespace ui

#endif /*EFFECTEDITOR_H_*/
