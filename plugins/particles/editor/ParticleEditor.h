#pragma once

#include "icommandsystem.h"
#include "gtkutil/window/BlockingTransientWindow.h"
#include "gtkutil/GladeWidgetHolder.h"
#include "gtkutil/WindowPosition.h"

#include "iparticlepreview.h"

namespace ui
{

class ParticleEditor :
	public gtkutil::BlockingTransientWindow,
    private gtkutil::GladeWidgetHolder
{
private:
	IParticlePreviewPtr _preview;

	// The position/size memoriser
	gtkutil::WindowPosition _windowPosition;

private:
	ParticleEditor();

public:
	/**
	 * Static method to display the Particles Editor dialog.
	 */
	static void displayDialog(const cmd::ArgumentList& args);

private:
	void _preHide();
	void _preShow();

	// gtkmm callbacks
	void _onCancel();
	void _onOK();
};

} // namespace
