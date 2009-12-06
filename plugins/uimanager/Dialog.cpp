#include "Dialog.h"

#include "DialogManager.h"

namespace ui
{

Dialog::Dialog(std::size_t id, DialogManager& owner) :
	_id(id),
	_owner(owner)
{}

std::size_t Dialog::getId() const
{
	return _id;
}

IDialog::Result Dialog::run()
{
	return IDialog::RESULT_CANCELLED;
}

// Frees this dialog and all its allocated resources.  Once a dialog as been destroyed, 
// calling any methods on this object results in undefined behavior.
void Dialog::destroy()
{
	// Nofity the manager, this will clear ourselves as soon as the last reference is gone
	// which might happen right after this call
	_owner.notifyDestroy(_id);

	// Do not call any other member methods after this point
}

} // namespace ui
