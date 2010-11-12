#ifndef _FIXUP_MAP_DIALOG_H_
#define _FIXUP_MAP_DIALOG_H_

#include "icommandsystem.h"

#include "gtkutil/dialog/Dialog.h"
#include "gtkutil/dialog/DialogElements.h"

namespace ui
{

class FixupMapDialog :
	public gtkutil::Dialog
{
private:
	IDialog::Handle _pathEntry;

public:
	FixupMapDialog();

	std::string getFixupFilePath();

	static void RunDialog(const cmd::ArgumentList& args);
};

} // namespace ui

#endif /* _FIXUP_MAP_DIALOG_H_ */
