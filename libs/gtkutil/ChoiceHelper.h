#pragma once

#include <wx/choice.h>
#include "string/convert.h"

namespace wxutil
{

// Various wxChoice helper methods 
class ChoiceHelper
{
public:
	// Returns the numeric ID (that has been attached to the choice items) of the current selection.
	// Returns wxNOT_FOUND if no valid selection could be determined
	static int GetSelectionId(wxChoice* choice)
	{
		if (choice->GetSelection() != wxNOT_FOUND)
		{
			wxClientData* data = choice->GetClientObject(choice->GetSelection());

			if (data == NULL) return wxNOT_FOUND;

			wxStringClientData* idStr = dynamic_cast<wxStringClientData*>(data);

			if (idStr == NULL) return wxNOT_FOUND;

			return string::convert<int>(idStr->GetData().ToStdString(), wxNOT_FOUND);
		}
		else
		{
			return -1;
		} 
	}

	// Selects the item whose numeric attached id is equal to <id>
	static void SelectItemByStoredId(wxChoice* choice, int id)
	{
		for (unsigned int i = 0; i < choice->GetCount(); ++i)
		{
			wxStringClientData* idStr = static_cast<wxStringClientData*>(choice->GetClientObject(i));
			int foundId = string::convert<int>(idStr->GetData().ToStdString(), wxNOT_FOUND);

			if (foundId == id)
			{
				choice->SetSelection(i);
				return;
			}
		}

		choice->SetSelection(wxNOT_FOUND);
	} 
};

} // namespace
