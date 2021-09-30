#pragma once

#include "wxutil/dialog/DialogBase.h"

#include "wxutil/preview/ParticlePreview.h"
#include "wxutil/dataview/ResourceTreeView.h"

#include "iparticles.h"
#include <string>
#include <map>
#include <memory>
#include <sigc++/trackable.h>

namespace ui
{

/**
 * \brief
 * Chooser dialog for selection and preview of particle systems.
 */
class ParticlesChooser : 
	public wxutil::DialogBase,
    public sigc::trackable
{
private:
    wxutil::ResourceTreeView::Columns _columns;

	// Tree view listing all the particles
	wxutil::ResourceTreeView* _treeView;

	// Last selected particle
	std::string _selectedParticle;

	// The preview widget
    wxutil::ParticlePreviewPtr _preview;

private:
	// callbacks
	void _onSelChanged(wxDataViewEvent& ev);

	// Constructor creates elements
	ParticlesChooser();

	// WIDGET CONSTRUCTION 
	wxutil::ResourceTreeView* createTreeView(wxWindow* parent);

	// Populate the list of particles
	void populateParticleList();

	void setSelectedParticle(const std::string& particleName);
    void _onItemActivated( wxDataViewEvent& ev );

private:
	void reloadParticles();

public:

	/**
	 * Display the singleton dialog and return the name of the selected
	 * particle system, or the empty string if none was selected.
	 *
	 * @param currentParticle
	 * The particle name which should be highlighted in the list when the dialog
	 * is first displayed. If this value is left at the default value of "", no
	 * particle will be selected.
	 *
	 * @returns
	 * The name of the particle selected by the user, or an empty string if the
	 * choice was cancelled or invalid.
	 */
	static std::string ChooseParticle(const std::string& currentParticle = "");
};

}
