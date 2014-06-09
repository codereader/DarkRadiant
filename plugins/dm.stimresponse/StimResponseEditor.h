#pragma once

#include "ientity.h"
#include "iradiant.h"
#include "icommandsystem.h"
#include "iscenegraph.h"
#include "gtkutil/WindowPosition.h"
#include "gtkutil/dialog/DialogBase.h"

#include "StimTypes.h"
#include "SREntity.h"
#include "CustomStimEditor.h"

#include <wx/notebook.h>

namespace ui
{

class StimEditor;
class ResponseEditor;

class StimResponseEditor;
typedef boost::shared_ptr<StimResponseEditor> StimResponseEditorPtr;

class StimResponseEditor :
	public wxutil::DialogBase
{
private:
	wxNotebook* _notebook;
	std::unique_ptr<wxImageList> _imageList;

	int _stimPageNum;
	int _responsePageNum;
	int _customStimPageNum;
	static int _lastShownPage;

	// The "extended" entity object managing the stims
	SREntityPtr _srEntity;

	// The position/size memoriser
	wxutil::WindowPosition _windowPosition;

	// The entity we're editing
	Entity* _entity;

	// The helper class managing the stims
	StimTypes _stimTypes;

	// The helper classes for editing the stims/responses
	StimEditor* _stimEditor;
	ResponseEditor* _responseEditor;
	//CustomStimEditor* _customStimEditor;

public:
	StimResponseEditor();

	// override DialogBase
	int ShowModal();

	// Command target to toggle the dialog
	static void ShowDialog(const cmd::ArgumentList& args);

private:
	/** greebo: Saves the current working set to the entity
	 */
	void save();

	/* WIDGET POPULATION */
	void populateWindow(); 			// Main window

	/** greebo: Checks the selection for a single entity.
	 */
	void rescanSelection();
};

} // namespace ui
