#include "ShaderDefinitionView.h"

#include <gtk/gtkvbox.h>

namespace ui 
{

ShaderDefinitionView::ShaderDefinitionView() :
	_vbox(gtk_vbox_new(FALSE, 0)),
	_view("def", true)
{
	gtk_box_pack_start(GTK_BOX(_vbox), _view, TRUE, TRUE, 0);
}

void ShaderDefinitionView::setShader(const std::string& shader)
{
	_shader = shader;
	
	update();
}

GtkWidget* ShaderDefinitionView::getWidget()
{
	return _vbox;
}

void ShaderDefinitionView::update()
{
	// Find the shader
}

} // namespace ui
