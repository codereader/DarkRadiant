#pragma once

#include "wxutil/dialog/DialogBase.h"

#include "ParticleSelector.h"

#include <string>
#include <sigc++/trackable.h>

class wxRadioButton;

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

    wxRadioButton* _funcEmitter;
    wxRadioButton* _funcSmoke;

private:
	// Constructor creates elements
	ParticleChooserDialog(bool showClassnameSelector);

    void _onItemActivated( wxDataViewEvent& ev );

    std::string getSelectedClassname();

public:

    struct SelectionResult
    {
        std::string selectedParticle;
        std::string selectedClassname;
    };

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
    static SelectionResult ChooseParticleAndEmitter(const std::string& currentParticle = "");

private:
    static SelectionResult RunDialog(bool showClassnameSelector, const std::string& currentParticle);
};

}
