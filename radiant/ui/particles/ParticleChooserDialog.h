#pragma once

#include "wxutil/dialog/DialogBase.h"

#include "ParticleSelector.h"

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
class ParticleChooserDialog : 
	public wxutil::DialogBase,
    public sigc::trackable
{
private:
    ParticleSelector* _selector;

private:
	// Constructor creates elements
	ParticleChooserDialog();

    void _onItemActivated( wxDataViewEvent& ev );

public:

	/**
	 * Display the dialog and return the name of the selected
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
