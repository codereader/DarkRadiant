#include "FloatingLayout.h"

namespace ui {

std::string FloatingLayout::getName() {
	return FLOATING_LAYOUT_NAME;
}

void FloatingLayout::activate() {
	// TODO
}

void FloatingLayout::deactivate() {
	// TODO
}

// The creation function, needed by the mainframe layout manager
FloatingLayoutPtr FloatingLayout::CreateInstance() {
	return FloatingLayoutPtr(new FloatingLayout);
}

} // namespace ui
