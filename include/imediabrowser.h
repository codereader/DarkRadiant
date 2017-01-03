#pragma once

#include "imodule.h"
#include <string>

namespace ui
{

/**
 * Interface to the MediaBrowser, which is displaying the 
 * available materials, accessible as a page in the 
 * GroupDialog's notebook.
 */
class IMediaBrowser :
	public RegisterableModule
{
public:
	virtual ~IMediaBrowser() 
	{}

	/**
	 * Returns the texture name of the currently selected item. 
	 * Will return an empty string if a folder is selected or 
	 * nothing is selected at all.
	 */
	virtual std::string getSelection() = 0;

	/** Set the given path as the current selection, highlighting it
	* in the tree view.
	*
	* @param selection
	* The fullname of the item to select, or the empty string if there
	* should be no selection.
	*/
	virtual void setSelection(const std::string& selection) = 0;

	// The tab name as registred in the GroupDialog
	const char* const getGroupDialogTabName() const
	{
		return "mediabrowser";
	}
};

}

const char* const MODULE_MEDIABROWSER = "MediaBrowser";

inline ui::IMediaBrowser& GlobalMediaBrowser()
{
	// Cache the reference locally
	static ui::IMediaBrowser& _mediaBrowser(
		*std::static_pointer_cast<ui::IMediaBrowser>(
			module::GlobalModuleRegistry().getModule(MODULE_MEDIABROWSER)
		)
	);
	return _mediaBrowser;
}
