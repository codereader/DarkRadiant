#pragma once

#include "icommandsystem.h"
#include "gtkutil/window/BlockingTransientWindow.h"
#include "gtkutil/GladeWidgetHolder.h"
#include "gtkutil/WindowPosition.h"

#include <gtkmm/liststore.h>
#include <gtkmm/treeselection.h>

#include "iparticlepreview.h"

namespace ui
{

class ParticleEditor :
	public gtkutil::BlockingTransientWindow,
    private gtkutil::GladeWidgetHolder
{
public:
	// Treemodel definition
	struct DefColumns :
		public Gtk::TreeModel::ColumnRecord
	{
		DefColumns() { add(name); }

		Gtk::TreeModelColumn<std::string> name;
	};

private:
	// List of target_addobjectives entities
	DefColumns _defColumns;
	Glib::RefPtr<Gtk::ListStore> _defList;

	Glib::RefPtr<Gtk::TreeSelection> _defSelection;

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
	void _postShow();

	// gtkmm callbacks
	void _onCancel();
	void _onOK();
	void _onSelChanged();

	void setupParticleDefList();
	void populateParticleDefList();

	void activateEditPanels();
	void deactivateEditPanels();
};

} // namespace
