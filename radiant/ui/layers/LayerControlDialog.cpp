#include "LayerControlDialog.h"

#include "iradiant.h"

namespace ui {

LayerControlDialog::LayerControlDialog() :
	PersistentTransientWindow("Layers", GlobalRadiant().getMainWindow())
{}

void LayerControlDialog::toggleDialog() {
	if (isVisible()) {
		hide();
	}
	else {
		show();
	}
}

void LayerControlDialog::update() {

}

void LayerControlDialog::toggle() {
	Instance().toggleDialog();
}

LayerControlDialog& LayerControlDialog::Instance() {
	static LayerControlDialog _dialog;
	return _dialog;
}

} // namespace ui
