#include "MaterialsList.h"

#include "i18n.h"
#include "irender.h"

#include <iostream>

namespace ui
{

namespace
{
    // Columns for the list
    struct Columns: public Gtk::TreeModel::ColumnRecord
    {
        Gtk::TreeModelColumn<Glib::ustring> material;
        Gtk::TreeModelColumn<bool> visible;

        Columns() { add(material); add(visible); }
    };

    const Columns& COLUMNS()
    {
        static const Columns _instance;
        return _instance;
    }
}

MaterialsList::MaterialsList(RenderSystemPtr renderSystem)
: _store(Gtk::ListStore::create(COLUMNS())),
  _renderSystem(renderSystem)
{
    g_assert(_renderSystem);
    g_assert(_store);

    // View columns
    append_column(_("Material"), COLUMNS().material);
    append_column(_("Visible"), COLUMNS().visible);

    // Set up editable toggle column
    Gtk::CellRendererToggle* cell = dynamic_cast<Gtk::CellRendererToggle*>(
        get_column_cell_renderer(1)
    );
    if (cell)
    {
        cell->set_activatable(true);
        cell->signal_toggled().connect(
            sigc::mem_fun(this, &MaterialsList::visibleCellClicked)
        );
    }

    // Material column should expand, toggle shouldn't
    get_column(0)->set_expand(true);
    get_column(1)->set_expand(false);

    set_model(_store);
}

void MaterialsList::visibleCellClicked(const Glib::ustring& treePath)
{
    Gtk::TreeModel::iterator i = _store->get_iter(treePath);
    if (!i) return;

    // Toggle the model data
    Gtk::TreeRow row = *i;
    row[COLUMNS().visible] = !row[COLUMNS().visible];

    // Hide or show the respective shader
    assert(_renderSystem);
    ShaderPtr shader = _renderSystem->capture(
        row.get_value(COLUMNS().material)
    );
    shader->setVisible(row.get_value(COLUMNS().visible));

    _visibilityChangedSignal.emit();
}

void MaterialsList::clear()
{
    _store->clear();
}

void MaterialsList::addMaterial(const Glib::ustring& name)
{
    Gtk::TreeModel::Row row = *_store->append();

    ShaderPtr shader = _renderSystem->capture(name);

    row[COLUMNS().material] = name;
    row[COLUMNS().visible] = shader && shader->isVisible();
}

}
