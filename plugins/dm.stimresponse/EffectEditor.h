#pragma once

#include "wxutil/dialog/DialogBase.h"

#include "StimResponse.h"
#include "ResponseEffectTypes.h"
#include "EffectArgumentItem.h"
#include <boost/shared_ptr.hpp>
#include <wx/arrstr.h>

class wxCheckBox;
class wxFlexGridSizer;

namespace ui
{

class ResponseEditor;

class EffectEditor :
	public wxutil::DialogBase
{
private:
	wxFlexGridSizer* _argTable;

	// The list containing the possible effect types
	ResponseEffectTypeMap _effectTypes;

	wxChoice* _effectTypeCombo;

	// The list of argument items
	typedef boost::shared_ptr<EffectArgumentItem> ArgumentItemPtr;
	typedef std::vector<ArgumentItemPtr> ArgumentItemList;
	ArgumentItemList _argumentItems;

	wxCheckBox* _stateToggle;

	// The references to the object we're editing here
	StimResponse& _response;
	unsigned int _effectIndex;

	// The saved StimResponse to revert the changes on cancel
	ResponseEffect _backup;

	// For calling update() when finished editing
	ResponseEditor& _editor;

	StimTypes& _stimTypes;

	wxArrayString _entityChoices;

public:
	/** greebo: Constructor, needs information about parent and the edit target.
	 *
	 * @parent: The parent this window is child of.
	 *
	 * @response: The Stim/Response object the effect is associated with
	 * 			  (this should be a response, although stims work as well).
	 *
	 * @effectIndex: The response effect index within the given Response.
	 *
	 * @stimTypes: The StimTypes helper class
	 *
	 * @editor: The ResponseEditor for calling update() on exit.
	 */
	EffectEditor(wxWindow* parent,
				 StimResponse& response,
				 const unsigned int effectIndex,
				 StimTypes& stimTypes,
				 ResponseEditor& editor);

	/** greebo: Creates the widgets
	 */
	void populateWindow();

	int ShowModal();

private:
	/** greebo: Reverts the changes and loads the values from the
	 * 			backup effect object into the edited one.
	 */
	void revert();

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

	void onEffectTypeChange(wxCommandEvent& ev);
	void onStateToggle(wxCommandEvent& ev);
}; 

} // namespace ui
