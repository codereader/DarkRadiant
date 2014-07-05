#pragma once

#include "icommandsystem.h"

#include "gtkutil/dialog/Dialog.h"

namespace ui
{

class FixupMapDialog :
	public wxutil::Dialog
{
private:
	IDialog::Handle _pathEntry;

public:
	FixupMapDialog();

	std::string getFixupFilePath();

	static void RunDialog(const cmd::ArgumentList& args);
};

} // namespace ui
