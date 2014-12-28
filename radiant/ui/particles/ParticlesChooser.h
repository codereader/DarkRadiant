#pragma once

#include "wxutil/dialog/DialogBase.h"

#include "wxutil/preview/ParticlePreview.h"
#include "wxutil/TreeModel.h"

#include "iparticles.h"
#include <string>
#include <map>
#include <memory>

namespace wxutil
{
	class TreeView;
}

namespace ui
{

class ParticlesChooser;
typedef std::shared_ptr<ParticlesChooser> ParticlesChooserPtr;

/**
 * \brief
 * Chooser dialog for selection and preview of particle systems.
 */
class ParticlesChooser : 
	public wxutil::DialogBase
{
private:
	class ThreadedParticlesLoader;
	std::unique_ptr<ThreadedParticlesLoader> _particlesLoader;

	// Tree store for shaders, and the tree selection
	wxutil::TreeModel::Ptr _particlesList;
	wxutil::TreeView* _treeView;

	// Last selected particle
	std::string _selectedParticle;

	// The particle to highlight once shown
	std::string _preSelectParticle;

	// The preview widget
    wxutil::ParticlePreviewPtr _preview;

private:
	// callbacks
	void _onSelChanged(wxDataViewEvent& ev);

	// Constructor creates elements
	ParticlesChooser();

	// WIDGET CONSTRUCTION 
	wxWindow* createTreeView(wxWindow* parent);

	// Static instance owner
	static ParticlesChooser& getInstance();
	static ParticlesChooserPtr& getInstancePtr();

	// Populate the list of particles
	void populateParticleList();
	void onTreeStorePopulationFinished(wxutil::TreeModel::PopulationFinishedEvent& ev);

	void setSelectedParticle(const std::string& particleName);

private:
	void onRadiantShutdown();
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
