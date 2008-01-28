#ifndef DIFFICULTY_EDITOR_H_
#define DIFFICULTY_EDITOR_H_

#include <boost/shared_ptr.hpp>

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
	// TODO DifficultySettings& _settings;

	GtkWidget* _editor;
	GtkWidget* _label;

public:
	// Returns the actual editor widget (contains all controls and views)
	GtkWidget* GetEditor();

	// Returns the label for packing into a GtkNotebook tab.
	GtkWidget* GetNotebookLabel();
};
typedef boost::shared_ptr<DifficultyEditor> DifficultyEditorPtr;

} // namespace ui

#endif /* DIFFICULTY_EDITOR_H_ */