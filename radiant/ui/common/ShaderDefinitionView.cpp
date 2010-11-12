#include "ShaderDefinitionView.h"

#include "i18n.h"
#include "ishaders.h"
#include "gtkutil/LeftAlignedLabel.h"

#include <gtkmm/table.h>

namespace ui
{

ShaderDefinitionView::ShaderDefinitionView() :
	Gtk::VBox(false, 6),
	_view(Gtk::manage(new gtkutil::SourceView("d3material", true)))
{
	Gtk::Table* table = Gtk::manage(new Gtk::Table(2, 2, false));
	pack_start(*table, false, false,  0);

	Gtk::Label* nameLabel = Gtk::manage(new gtkutil::LeftAlignedLabel(_("Material:")));
	Gtk::Label* materialFileLabel = Gtk::manage(new gtkutil::LeftAlignedLabel(_("Defined in:")));

	_materialName = Gtk::manage(new gtkutil::LeftAlignedLabel(""));
	_filename = Gtk::manage(new gtkutil::LeftAlignedLabel(""));

	nameLabel->set_size_request(90, -1);
	materialFileLabel->set_size_request(90, -1);

	table->attach(*nameLabel, 0, 1, 0, 1, Gtk::AttachOptions(0), Gtk::AttachOptions(0), 0, 0);
	table->attach(*materialFileLabel, 0, 1, 1, 2, Gtk::AttachOptions(0), Gtk::AttachOptions(0), 0, 0);

	table->attach(*_materialName, 1, 2, 0, 1);
	table->attach(*_filename, 1, 2, 1, 2);

	pack_start(*Gtk::manage(new gtkutil::LeftAlignedLabel(_("Definition"))), false, false, 0);
	pack_start(*_view, true, true, 0);
}

void ShaderDefinitionView::setShader(const std::string& shader)
{
	_shader = shader;

	update();
}

void ShaderDefinitionView::update()
{
	// Find the shader
	MaterialPtr material = GlobalMaterialManager().getMaterialForName(_shader);

	if (material == NULL)
	{
		// Null-ify the contents
		_materialName->set_markup("");
		_filename->set_markup("");

		_view->set_sensitive(false);

		return;
	}

	// Add the shader and file name
	_materialName->set_markup("<b>" + material->getName() + "</b>");
	_filename->set_markup(std::string("<b>") + material->getShaderFileName() + "</b>");

	_view->set_sensitive(true);

	// Surround the definition with curly braces, these are not included
	std::string definition = _shader + "\n{\n\r";
	definition += material->getDefinition();
	definition += "\n\r}";

	_view->setContents(definition);
}

} // namespace ui
