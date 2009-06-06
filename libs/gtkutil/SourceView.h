#ifndef GTKUTIL_SOURCEVIEW_H_
#define GTKUTIL_SOURCEVIEW_H_

#include <string>

typedef struct _GtkSourceBuffer GtkSourceBuffer;
typedef struct _GtkSourceView GtkSourceView;
typedef struct _GtkWidget GtkWidget;
typedef struct _GtkSourceLanguageManager GtkSourceLanguageManager;

namespace gtkutil 
{

class SourceView
{
	// The top-level widget
	GtkWidget* _widget;

	GtkSourceView* _view;
	GtkSourceBuffer* _buffer;

	GtkSourceLanguageManager* _langManager;

public:
	/**
	 * Constructs a new sourceview with the given language ID as specified
	 * in the .lang files (e.g. "python").
	 *
	 * @readOnly: Set this to TRUE to disallow editing of the text buffer.
	 */
	SourceView(const std::string& language, bool readOnly);

	~SourceView();

	// Operator-cast to widget for packing into a parent container
	operator GtkWidget* () const;

	void setContents(const std::string& newContents);

	// Returns the contents of the source buffer
	std::string getContents();

	// Clears the contents of the buffer
	void clear();
};

} // namespace gtkutil

#endif /* GTKUTIL_SOURCEVIEW_H_ */
