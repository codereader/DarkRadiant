#ifndef DIFFICULTY_EDITOR_H_
#define DIFFICULTY_EDITOR_H_

#include <string>
#include <boost/shared_ptr.hpp>

#include "DifficultySettings.h"

typedef struct _GtkWidget GtkWidget;

namespace ui {

/**
 * greebo: A Difficulty Editor provides the front-end for editing a 
 *         set of Difficulty Settings (e.g. "Easy" setting). The actual
 *         data is stored in a DifficultySettings instance.
 */
class DifficultyEditor
{
	// The actual settings we're working with
	difficulty::DifficultySettingsPtr _settings;

	GtkWidget* _editor;

	GtkWidget* _labelHBox;
	GtkWidget* _label; // the actual label

public:
	// Constructor, pass the label
	DifficultyEditor(const std::string& label, const difficulty::DifficultySettingsPtr& settings);

	// Returns the actual editor widget (contains all controls and views)
	GtkWidget* getEditor();

	// Returns the label for packing into a GtkNotebook tab.
	GtkWidget* getNotebookLabel();

	// Set the title label of this editor pane
	void setLabel(const std::string& label);
};
typedef boost::shared_ptr<DifficultyEditor> DifficultyEditorPtr;

} // namespace ui

#endif /* DIFFICULTY_EDITOR_H_ */