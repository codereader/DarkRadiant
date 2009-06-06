#include "ShaderDefinitionView.h"

#include "ishaders.h"
#include <gtk/gtkvbox.h>
#include <gtk/gtktable.h>
#include "gtkutil/LeftAlignedLabel.h"

namespace ui 
{

ShaderDefinitionView::ShaderDefinitionView() :
	_vbox(gtk_vbox_new(FALSE, 6)),
	_view("d3material", true)
{
	GtkTable* table = GTK_TABLE(gtk_table_new(2, 2, FALSE));
	gtk_box_pack_start(GTK_BOX(_vbox), GTK_WIDGET(table), FALSE, FALSE, 0);

	GtkWidget* nameLabel = gtkutil::LeftAlignedLabel("Material:");
	GtkWidget* materialFileLabel = gtkutil::LeftAlignedLabel("Defined in:");

	_materialName = gtkutil::LeftAlignedLabel("");
	_filename = gtkutil::LeftAlignedLabel("");

	gtk_widget_set_size_request(nameLabel, 90, -1);
	gtk_widget_set_size_request(materialFileLabel, 90, -1);

	gtk_table_attach(table, nameLabel, 0, 1, 0, 1,
					(GtkAttachOptions) (0),
					(GtkAttachOptions) (0), 0, 0);
	gtk_table_attach(table, materialFileLabel, 0, 1, 1, 2,
					(GtkAttachOptions) (0),
					(GtkAttachOptions) (0), 0, 0);
	
	gtk_table_attach_defaults(table, _materialName, 1, 2, 0, 1);
	gtk_table_attach_defaults(table, _filename, 1, 2, 1, 2);

	gtk_box_pack_start(GTK_BOX(_vbox), gtkutil::LeftAlignedLabel("Definition"), FALSE, FALSE, 0);
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
	MaterialPtr material = GlobalMaterialManager().getMaterialForName(_shader);
	
	if (material == NULL) 
	{
		// Null-ify the contents
		gtk_label_set_markup(GTK_LABEL(_materialName), "");
		gtk_label_set_markup(GTK_LABEL(_filename), "");

		gtk_widget_set_sensitive(_view, FALSE);

		return;
	}
	
	// Add the shader and file name 
	gtk_label_set_markup(GTK_LABEL(_materialName), ("<b>" + material->getName() + "</b>").c_str());
	gtk_label_set_markup(GTK_LABEL(_filename), (std::string("<b>") + material->getShaderFileName() + "</b>").c_str());

	gtk_widget_set_sensitive(_view, TRUE);

	// Surround the definition with curly braces, these are not included
	std::string definition = _shader + "\n{\n";
	definition += material->getDefinition();
	definition += "\n}";

	_view.setContents(definition);
}

} // namespace ui
