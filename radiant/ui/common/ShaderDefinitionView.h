#ifndef SHADER_DEFINITION_VIEW_H_
#define SHADER_DEFINITION_VIEW_H_

#include <string>
#include "gtkutil/SourceView.h"
typedef struct _GtkWidget GtkWidget;

namespace ui 
{

class ShaderDefinitionView
{
	// The shader which should be previewed
	std::string _shader;

	// The top-level widget
	GtkWidget* _vbox;

	// The actual code view
	gtkutil::SourceView _view;

public:
	ShaderDefinitionView();

	// Returns the topmost widget for packing this view into a parent container
	GtkWidget* getWidget();

	void setShader(const std::string& shader);

	void update();
};

} // namespace ui

#endif /* SHADER_DEFINITION_VIEW_H_ */
