#include "EntityClassChooser.h"
#include "EntityClassTreePopulator.h"

#include "i18n.h"
#include "iregistry.h"
#include "imainframe.h"
#include "iuimanager.h"

#include <gtkmm/box.h>
#include <gtkmm/textview.h>
#include <gtkmm/button.h>
#include <gtkmm/stock.h>

#include "gtkutil/TreeModel.h"
#include "gtkutil/ScrolledFrame.h"
#include "gtkutil/RightAlignment.h"
#include "gtkutil/IconTextColumn.h"
#include "gtkutil/MultiMonitor.h"
#include "string/string.h"

namespace ui
{

    namespace
    {
        const char* const ECLASS_CHOOSER_TITLE = N_("Create entity");
    }

EntityClassChooser::EntityClassChooser()
: gtkutil::BlockingTransientWindow(_(ECLASS_CHOOSER_TITLE),
                                   GlobalMainFrame().getTopLevelWindow()),
  gtkutil::GladeWidgetHolder(
        GlobalUIManager().getGtkBuilderFromFile("EntityClassChooser.glade")
  ),
  _treeStore(Gtk::TreeStore::create(_columns)),
  _selection(NULL),
  _selectedName(""),
  _modelPreview(GlobalUIManager().createModelPreview()),
  _result(RESULT_CANCELLED)
{
    // Set the default size of the window
    const Glib::RefPtr<Gtk::Window>& mainWindow = GlobalMainFrame().getTopLevelWindow();
    Gdk::Rectangle rect = gtkutil::MultiMonitor::getMonitorForWindow(mainWindow);
    set_default_size(
        static_cast<int>(rect.get_width() * 0.7f),
        static_cast<int>(rect.get_height() * 0.6f)
    );

    _modelPreview->setSize(static_cast<int>(rect.get_width() * 0.3f));

    // Create GUI elements and pack into main VBox
    add(*getGladeWidget<Gtk::Widget>("mainHbox"));
    g_assert(get_child() != NULL);

    Gtk::HBox* hbox = getGladeWidget<Gtk::HBox>("mainHbox");
    assert(hbox == get_child());

    // Connect button signals
    getGladeWidget<Gtk::Button>("okButton")->signal_clicked().connect(
        sigc::mem_fun(*this, &EntityClassChooser::callbackOK)
    );
    getGladeWidget<Gtk::Button>("cancelButton")->signal_clicked().connect(
        sigc::mem_fun(*this, &EntityClassChooser::callbackCancel)
    );

    hbox->pack_end(*_modelPreview->getWidget(), false, false, 0);

    // Register to the eclass manager
    GlobalEntityClassManager().addObserver(this);

    // Populate the model and setup the tree view
    setupTreeView();
    loadEntityClasses();
}
// Display the singleton instance
std::string EntityClassChooser::chooseEntityClass()
{
    Instance().show();

    if (Instance().getResult() == RESULT_OK)
    {
        return Instance().getSelectedEntityClass();
    }
    else
    {
        return ""; // Empty selection on cancel
    }
}

EntityClassChooser& EntityClassChooser::Instance()
{
    EntityClassChooserPtr& instancePtr = InstancePtr();

    if (instancePtr == NULL)
    {
        // Not yet instantiated, do it now
        instancePtr.reset(new EntityClassChooser);

        // Register this instance with GlobalRadiant() at once
        GlobalRadiant().addEventListener(instancePtr);
    }

    return *instancePtr;
}

EntityClassChooserPtr& EntityClassChooser::InstancePtr()
{
    static EntityClassChooserPtr _instancePtr;
    return _instancePtr;
}

void EntityClassChooser::onRadiantShutdown()
{
    globalOutputStream() << "EntityClassChooser shutting down." << std::endl;

    GlobalEntityClassManager().removeObserver(this);

    _modelPreview = IModelPreviewPtr();

    // Final step at shutdown, release the shared ptr
    InstancePtr().reset();
}

void EntityClassChooser::onEClassReload()
{
    // Reload the class tree
    loadEntityClasses();
}


EntityClassChooser::Result EntityClassChooser::getResult()
{
    return _result;
}

void EntityClassChooser::setSelectedEntityClass(const std::string& eclass)
{
    gtkutil::TreeModel::findAndSelectString(
        getGladeWidget<Gtk::TreeView>("entityTreeView"),
        eclass,
        _columns.name
    );
}

const std::string& EntityClassChooser::getSelectedEntityClass() const
{
    return _selectedName;
}

void EntityClassChooser::_postHide()
{
    BlockingTransientWindow::_postHide();

    // Release the models when the dialog is hidden again
    _modelPreview->clear();
}

void EntityClassChooser::_onDeleteEvent()
{
    _result = RESULT_CANCELLED;

    // greebo: Clear the selected name on hide, we don't want to create another entity when
    // the user clicks on the X in the upper right corner.
    _selectedName.clear();

    hide(); // just hide, don't call base class which might delete this dialog
}

void EntityClassChooser::_postShow()
{
    // Initialise the GL widget after the widgets have been shown
    _modelPreview->initialisePreview();

    // Update the member variables
    updateSelection();

    // Focus on the treeview
    getGladeWidget<Gtk::TreeView>("entityTreeView")->grab_focus();

    // Call the base class, will enter main loop
    BlockingTransientWindow::_postShow();
}

void EntityClassChooser::loadEntityClasses()
{
    // Clear the tree store first
    _treeStore->clear();

    // Populate it with the list of entity
    // classes by using a visitor class.
    EntityClassTreePopulator visitor(_treeStore, _columns);
    GlobalEntityClassManager().forEach(visitor);

    // insert the data, using the same walker class as Visitor
    visitor.forEachNode(visitor);
}

// Create the tree view

void EntityClassChooser::setupTreeView()
{
    // Construct the tree view widget with the now-populated model
    Gtk::TreeView* treeView = getGladeWidget<Gtk::TreeView>("entityTreeView");
    treeView->set_model(_treeStore);

    // Use the TreeModel's full string search function
    treeView->set_search_equal_func(sigc::ptr_fun(gtkutil::TreeModel::equalFuncStringContains));

    _selection = treeView->get_selection();
    _selection->set_mode(Gtk::SELECTION_BROWSE);
    _selection->signal_changed().connect(sigc::mem_fun(*this, &EntityClassChooser::callbackSelectionChanged));

    // Single column with icon and name
    Gtk::TreeViewColumn* col = Gtk::manage(
        new gtkutil::IconTextColumn(_("Classname"), _columns.name, _columns.icon)
    );
    col->set_sort_column(_columns.name);

    treeView->append_column(*col);

    gtkutil::TreeModel::applyFoldersFirstSortFunc(_treeStore, _columns.name, _columns.isFolder);
}

// Update the usage information
void EntityClassChooser::updateUsageInfo(const std::string& eclass)
{
    // Lookup the IEntityClass instance
    IEntityClassPtr e = GlobalEntityClassManager().findOrInsert(eclass, true);

    // Set the usage panel to the IEntityClass' usage information string
    Gtk::TextView* usageText = getGladeWidget<Gtk::TextView>("usageTextView");
    Glib::RefPtr<Gtk::TextBuffer> buf = usageText->get_buffer();

    // Create the concatenated usage string
    std::string usage = "";
    EntityClassAttributeList usageAttrs = e->getAttributeList("editor_usage");

    for (EntityClassAttributeList::const_iterator i = usageAttrs.begin();
         i != usageAttrs.end();
         ++i)
    {
        // Add only explicit (non-inherited) usage strings
        if (!i->inherited)
        {
            if (!usage.empty())
                usage += std::string("\n") + i->value;
            else
                usage += i->value;
        }
    }

    buf->set_text(usage);
}

void EntityClassChooser::updateSelection()
{
    Gtk::TreeModel::iterator iter = _selection->get_selected();

    if (iter)
    {
        Gtk::TreeModel::Row row = *iter;

        if (!row[_columns.isFolder])
        {
            // Make the OK button active
            getGladeWidget<Gtk::Widget>("okButton")->set_sensitive(true);

            // Set the panel text with the usage information
            std::string selName = row[_columns.name];
            updateUsageInfo(selName);

            // Lookup the IEntityClass instance
            IEntityClassPtr eclass = GlobalEntityClassManager().findClass(selName);

            if (eclass != NULL)
            {
                _modelPreview->setModel(eclass->getAttribute("model").value);
                _modelPreview->setSkin(eclass->getAttribute("skin").value);
            }

            // Update the _selectionName field
            _selectedName = selName;

            return; // success
        }
    }

    // Nothing selected
    _modelPreview->setModel("");
    _modelPreview->setSkin("");

    getGladeWidget<Gtk::Widget>("okButton")->set_sensitive(false);
}

void EntityClassChooser::callbackCancel()
{
    _result = RESULT_CANCELLED;
    _selectedName.clear();

    hide(); // breaks main loop
}

void EntityClassChooser::callbackOK()
{
    _result = RESULT_OK;

    hide(); // breaks main loop
}

void EntityClassChooser::callbackSelectionChanged()
{
    updateSelection();
}

} // namespace ui
