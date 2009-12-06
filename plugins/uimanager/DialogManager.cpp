#include "DialogManager.h"

#include "itextstream.h"

namespace ui
{

DialogManager::DialogManager() :
	_highestIndex(0)
{}

DialogManager::~DialogManager()
{
	globalOutputStream() << "DialogManager: " << _dialogs.size() 
		<< " dialogs still in memory at shutdown." << std::endl;

	_dialogs.clear();
}

IDialogPtr DialogManager::createDialog()
{
	// Allocate a new dialog
	DialogPtr dialog(new Dialog(++_highestIndex, *this));

	// Store it in the local map so that references are held
	_dialogs[dialog->getId()] = dialog;

	return dialog;
}

void DialogManager::notifyDestroy(std::size_t id)
{
	DialogMap::iterator found = _dialogs.find(id);

	if (found != _dialogs.end())
	{
		_dialogs.erase(found);
	}
	else
	{
		globalWarningStream() << "Cannot destroy dialog with id " << id << ", not found in map." << std::endl;
	}
}

} // namespace ui
