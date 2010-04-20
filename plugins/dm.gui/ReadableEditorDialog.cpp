// Project related
#include "ReadableEditorDialog.h"
#include "XdFileChooserDialog.h"
#include "XDataSelector.h"
#include "GuiSelector.h"
#include "gui/GuiManager.h"
#include "TextViewInfoDialog.h"
#include "ReadableGuiView.h"

// General
#include "selectionlib.h"
#include "gtkutil/MultiMonitor.h"
#include "gtkutil/LeftAlignedLabel.h"
#include "gtkutil/RightAlignment.h"
#include "gtkutil/FramedWidget.h"
#include "gtkutil/dialog.h"
#include "gtkutil/ScrolledFrame.h"
#include "gtkutil/LeftAlignment.h"
#include "gtkutil/StockIconMenuItem.h"
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include "boost/filesystem/fstream.hpp"
#include "string/string.h"
#include "i18n.h"

// Modules
#include "imainframe.h"
#include "ientity.h"
#include "iregistry.h"
#include "imap.h"
#include "igame.h"
#include "idialogmanager.h"

namespace ui
{

// consts:
namespace 
{
	const char* const WINDOW_TITLE = N_("Readable Editor");

	const std::string RKEY_READABLE_BASECLASS("game/readables/readableBaseClass");

	const char* const NO_ENTITY_ERROR = N_("Cannot run Readable Editor on this selection.\n"
		"Please select a single XData entity."); 

	const char* const LABEL_PAGE_RELATED = N_("Page Editing:");
	const char* const LABEL_GENERAL_PROPERTIES = N_("General Properties:");

	const int MIN_ENTRY_WIDTH = 35;

	// Widget handles for use in the _widgets std::map
	enum
	{
		WIDGET_EDIT_PANE,
		WIDGET_READABLE_NAME,
		WIDGET_XDATA_NAME,
		WIDGET_SAVEBUTTON,
		WIDGET_NUMPAGES,
		WIDGET_RADIO_ONESIDED,
		WIDGET_RADIO_TWOSIDED,
		WIDGET_CURRENT_PAGE,
		WIDGET_PAGETURN_SND,
		WIDGET_GUI_ENTRY,
		WIDGET_PAGE_LEFT,
		WIDGET_PAGE_RIGHT,
		WIDGET_PAGE_TITLE,
		WIDGET_PAGE_RIGHT_TITLE,
		WIDGET_PAGE_RIGHT_TITLE_SCROLLED,
		WIDGET_PAGE_BODY,
		WIDGET_PAGE_RIGHT_BODY,
		WIDGET_PAGE_RIGHT_BODY_SCROLLED,
		WIDGET_MENU_APPEND,
		WIDGET_MENU_PREPEND,
		WIDGET_MENU_TOOLS,
		WIDGET_MENU_INSERT,
		WIDGET_MENU_DELETE,
		WIDGET_PANED,
		WIDGET_TEXTVIEW_TABLE,
	};

} // namespace

//////////////////////////////////////////////////////////////////////////////
// Public and protected methods:
ReadableEditorDialog::ReadableEditorDialog(Entity* entity) :
	gtkutil::BlockingTransientWindow(_(WINDOW_TITLE), GlobalMainFrame().getTopLevelWindow()),
	_guiView(new gui::ReadableGuiView),
	_result(RESULT_CANCEL),
	_entity(entity),
	_currentPageIndex(0),
	_xdLoader(new XData::XDataLoader()),
	_runningGuiLayoutCheck(false),
	_runningXDataUniquenessCheck(false),
	_xdNameSpecified(false),
	_useDefaultFilename(true),
	_saveInProgress(false)
{
	// Set the default border width in accordance to the HIG
	gtk_container_set_border_width(GTK_CONTAINER(getWindow()), 12);
	gtk_window_set_type_hint(GTK_WINDOW(getWindow()), GDK_WINDOW_TYPE_HINT_NORMAL);

	// Add a vbox for the dialog elements
	GtkWidget* vbox = gtk_vbox_new(FALSE, 6);

	// Create the editPane:
	GtkWidget* vboxEP = gtk_vbox_new(FALSE, 6);
	_widgets[WIDGET_EDIT_PANE] = vboxEP;
	gtk_box_pack_start(GTK_BOX(vboxEP), createGeneralPropertiesInterface(), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vboxEP), createPageRelatedInterface(), TRUE, TRUE, 0);

	// The paned contains the controls and the preview
	GtkPaned* paned = GTK_PANED(gtk_hpaned_new());
	gtk_paned_pack1(paned, vboxEP, TRUE, FALSE);
	gtk_paned_pack2(paned, gtkutil::FramedWidget(_guiView->getWidget()), TRUE, FALSE);

	_widgets[WIDGET_PANED] = GTK_WIDGET(paned);

	// Pack it all into vbox
	gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(paned), TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), createButtonPanel(), FALSE, FALSE, 0);

	createMenus();

	gtk_container_add(GTK_CONTAINER(getWindow()), vbox);
}

void ReadableEditorDialog::_postShow()
{
	// Load the initial values from the entity
	if (!initControlsFromEntity())
	{
		// User clicked cancel, so destroy the window.
		this->destroy();
		return;
	}

	// Initialize proper editing controls.
	populateControlsFromXData();

	// Initialise the GL widget after the widgets have been shown
	_guiView->initialiseView();

	BlockingTransientWindow::_postShow();
}

void ReadableEditorDialog::RunDialog(const cmd::ArgumentList& args)
{
	// Check prerequisites
	const SelectionInfo& info = GlobalSelectionSystem().getSelectionInfo();

	if (info.entityCount == 1 && info.totalCount == info.entityCount)
	{
		// Check the entity type
		Entity* entity = Node_getEntity(GlobalSelectionSystem().ultimateSelected());

		if (entity != NULL && entity->getKeyValue("editor_readable") == "1")
		{
			// Show the dialog
			ReadableEditorDialog dialog(entity);
			dialog.show();

			return;
		}
	}

	// Exactly one redable entity must be selected.
	gtkutil::errorDialog(_(NO_ENTITY_ERROR), GlobalMainFrame().getTopLevelWindow());
}

//////////////////////////////////////////////////////////////////////////////
// UI Creation:

GtkWidget* ReadableEditorDialog::createGeneralPropertiesInterface()
{
	GtkWidget* vbox = gtk_vbox_new(FALSE, 6);

	// Add a Headline-label
	GtkWidget* generalPropertiesLabel = gtkutil::LeftAlignedLabel(
		std::string("<span weight=\"bold\">") + _(LABEL_GENERAL_PROPERTIES) + "</span>"
	);
	gtk_box_pack_start(GTK_BOX(vbox), generalPropertiesLabel, FALSE, FALSE, 0);

	// Create the table for the widget alignment
	GtkTable* table = GTK_TABLE(gtk_table_new(4, 2, FALSE));
	gtk_table_set_row_spacings(table, 6);
	gtk_table_set_col_spacings(table, 6);
	GtkWidget* alignmentMT = gtkutil::LeftAlignment(GTK_WIDGET(table), 18, 1.0);
	gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(alignmentMT), FALSE, FALSE, 0);

	int curRow = 0;

	// Readable Name
	GtkWidget* nameEntry = gtk_entry_new();
	_widgets[WIDGET_READABLE_NAME] = nameEntry;
	gtk_entry_set_width_chars(GTK_ENTRY(nameEntry), MIN_ENTRY_WIDTH);
	g_signal_connect(G_OBJECT(nameEntry), "key-press-event", G_CALLBACK(onKeyPress), this);

	GtkWidget* nameLabel = gtkutil::LeftAlignedLabel(_("Inventory Name:"));

	gtk_table_attach(table, nameLabel, 0, 1, curRow, curRow+1, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach_defaults(table, nameEntry, 1, 2, curRow, curRow+1);

	curRow++;

	// XData Name
	GtkWidget* xdataNameEntry = gtk_entry_new();
	_widgets[WIDGET_XDATA_NAME] = xdataNameEntry;
	g_signal_connect(G_OBJECT(xdataNameEntry), "key-press-event", G_CALLBACK(onKeyPress), this);
	g_signal_connect_after(G_OBJECT(xdataNameEntry), "focus-out-event", G_CALLBACK(onFocusOut), this);

	GtkWidget* xDataNameLabel = gtkutil::LeftAlignedLabel(_("XData Name:"));

	// Add a browse-button.
	GtkWidget* browseXdButton = gtk_button_new_with_label("");
	gtk_button_set_image(GTK_BUTTON(browseXdButton), gtk_image_new_from_stock(GTK_STOCK_OPEN, GTK_ICON_SIZE_SMALL_TOOLBAR) );
	g_signal_connect(G_OBJECT(browseXdButton), "clicked", G_CALLBACK(onBrowseXd), this);

	GtkWidget* hboxXd = gtk_hbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(hboxXd), GTK_WIDGET(xdataNameEntry), TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hboxXd), GTK_WIDGET(browseXdButton), FALSE, FALSE, 0);

	gtk_table_attach(table, xDataNameLabel, 0, 1, curRow, curRow+1, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach_defaults(table, hboxXd, 1, 2, curRow, curRow+1);

	curRow++;

	// Page count
	GtkWidget* numPagesSpin = gtk_spin_button_new_with_range(1, static_cast<gdouble>(XData::MAX_PAGE_COUNT), 1);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(numPagesSpin), 0);
	gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(numPagesSpin), TRUE);
	_widgets[WIDGET_NUMPAGES] = numPagesSpin;
	g_signal_connect(G_OBJECT(numPagesSpin), "value-changed", G_CALLBACK(onValueChanged), this);
	g_signal_connect(G_OBJECT(numPagesSpin), "key-press-event", G_CALLBACK(onKeyPress), this);

	GtkWidget* numPagesLabel = gtkutil::LeftAlignedLabel(_("Number of Pages:"));

	// Page Layout:
	GtkWidget* pageLayoutLabel = gtkutil::LeftAlignedLabel(_("Layout:"));
	GtkWidget* oneSidedRadio = gtk_radio_button_new_with_label(NULL, _("One-sided"));
	gtk_widget_add_events(oneSidedRadio, GDK_BUTTON_PRESS_MASK );
	g_signal_connect(G_OBJECT(oneSidedRadio), "button-press-event", G_CALLBACK(onOneSided), this);
	_widgets[WIDGET_RADIO_ONESIDED] = oneSidedRadio;

	GSList* pageLayoutGroup = gtk_radio_button_get_group(GTK_RADIO_BUTTON(oneSidedRadio));
	GtkWidget* twoSidedRadio = gtk_radio_button_new_with_label(pageLayoutGroup, _("Two-sided"));
	gtk_widget_add_events(twoSidedRadio, GDK_BUTTON_PRESS_MASK );
	g_signal_connect(G_OBJECT(twoSidedRadio), "button-press-event", G_CALLBACK(onTwoSided), this);
	_widgets[WIDGET_RADIO_TWOSIDED] = twoSidedRadio;

	// Add the radiobuttons to an hbox
	GtkWidget* hboxPL = gtk_hbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(hboxPL), GTK_WIDGET(numPagesSpin), TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hboxPL), GTK_WIDGET(pageLayoutLabel), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hboxPL), GTK_WIDGET(oneSidedRadio), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hboxPL), GTK_WIDGET(twoSidedRadio), FALSE, FALSE, 0);

	// Add numPages Label and hbox to the table.
	gtk_table_attach(table, numPagesLabel, 0, 1, curRow, curRow+1, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach_defaults(table, hboxPL, 1, 2, curRow, curRow+1);

	curRow++;

	// Pageturn Sound
	GtkWidget* pageTurnEntry = gtk_entry_new();
	_widgets[WIDGET_PAGETURN_SND] = pageTurnEntry;
	GtkWidget* pageTurnLabel = gtkutil::LeftAlignedLabel(_("Pageturn Sound:"));

	gtk_table_attach(table, pageTurnLabel, 0, 1, curRow, curRow+1, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach_defaults(table, pageTurnEntry, 1, 2, curRow, curRow+1);

	return vbox;
}

GtkWidget* ReadableEditorDialog::createPageRelatedInterface()
{
	GtkWidget* vbox = gtk_vbox_new(FALSE, 6);

	// Add a label for page related edits and add it to the vbox
	GtkWidget* pageRelatedLabel = gtkutil::LeftAlignedLabel(
		std::string("<span weight=\"bold\">") + _(LABEL_PAGE_RELATED) + "</span>"
	);
	gtk_box_pack_start(GTK_BOX(vbox), pageRelatedLabel, FALSE, FALSE, 0);

	// Insert Button
	GtkWidget* insertButton = gtk_button_new_with_label(_("Insert"));
	gtk_button_set_image(GTK_BUTTON(insertButton), gtk_image_new_from_stock(GTK_STOCK_ADD, GTK_ICON_SIZE_SMALL_TOOLBAR));
	g_signal_connect(G_OBJECT(insertButton), "clicked", G_CALLBACK(onInsert), this);

	// Delete Button
	GtkWidget* deleteButton = gtk_button_new_with_label(_("Delete"));
	gtk_button_set_image(GTK_BUTTON(deleteButton), gtk_image_new_from_stock(GTK_STOCK_REMOVE, GTK_ICON_SIZE_SMALL_TOOLBAR));
	g_signal_connect(G_OBJECT(deleteButton), "clicked", G_CALLBACK(onDelete), this);

	// Page Switcher
	GtkWidget* prevPage = gtk_button_new_with_label("");
	gtk_button_set_image(GTK_BUTTON(prevPage), gtk_image_new_from_stock(GTK_STOCK_GO_BACK, GTK_ICON_SIZE_SMALL_TOOLBAR));
	g_signal_connect(G_OBJECT(prevPage), "clicked", G_CALLBACK(onPrevPage), this);

	GtkWidget* nextPage = gtk_button_new_with_label("");
	gtk_button_set_image(GTK_BUTTON(nextPage), gtk_image_new_from_stock(GTK_STOCK_GO_FORWARD, GTK_ICON_SIZE_SMALL_TOOLBAR));
	g_signal_connect(G_OBJECT(nextPage), "clicked", G_CALLBACK(onNextPage), this);

	GtkWidget* firstPage = gtk_button_new_with_label("");
	gtk_button_set_image(GTK_BUTTON(firstPage), gtk_image_new_from_stock(GTK_STOCK_GOTO_FIRST, GTK_ICON_SIZE_SMALL_TOOLBAR));
	g_signal_connect(G_OBJECT(firstPage), "clicked", G_CALLBACK(onFirstPage), this);

	GtkWidget* lastPage = gtk_button_new_with_label("");
	gtk_button_set_image(GTK_BUTTON(lastPage), gtk_image_new_from_stock(GTK_STOCK_GOTO_LAST, GTK_ICON_SIZE_SMALL_TOOLBAR));
	g_signal_connect(G_OBJECT(lastPage), "clicked", G_CALLBACK(onLastPage), this);

	GtkWidget* currPageLabel = gtkutil::LeftAlignedLabel(_("Current Page:"));
	GtkWidget* currPageDisplay = gtkutil::LeftAlignedLabel("1");
	_widgets[WIDGET_CURRENT_PAGE] = currPageDisplay;

	// Add the elements to an hbox
	GtkWidget* hboxPS = gtk_hbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(hboxPS), GTK_WIDGET(insertButton), TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hboxPS), GTK_WIDGET(firstPage), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hboxPS), GTK_WIDGET(prevPage), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hboxPS), GTK_WIDGET(currPageLabel), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hboxPS), GTK_WIDGET(currPageDisplay), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hboxPS), GTK_WIDGET(nextPage), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hboxPS), GTK_WIDGET(lastPage), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hboxPS), GTK_WIDGET(deleteButton), TRUE, TRUE, 0);

	gtk_box_pack_start(GTK_BOX(vbox), gtkutil::LeftAlignment(GTK_WIDGET(hboxPS), 18, 1.0), FALSE, FALSE, 0);

	// Add a gui chooser with a browse-button
	GtkWidget* guiLabel = gtkutil::LeftAlignedLabel(_("Gui Definition:"));

	GtkWidget* guiEntry = gtk_entry_new();
	_widgets[WIDGET_GUI_ENTRY] = guiEntry;
	g_signal_connect(G_OBJECT(guiEntry), "key-press-event", G_CALLBACK(onKeyPress), this);
	g_signal_connect_after(G_OBJECT(guiEntry), "focus-out-event", G_CALLBACK(onFocusOut), this);

	GtkWidget* browseGuiButton = gtk_button_new_with_label("");
	gtk_button_set_image(GTK_BUTTON(browseGuiButton), gtk_image_new_from_stock(GTK_STOCK_OPEN, GTK_ICON_SIZE_SMALL_TOOLBAR) );
	g_signal_connect(G_OBJECT(browseGuiButton), "clicked", G_CALLBACK(onBrowseGui), this);

	// Add the elements to an hbox
	GtkWidget* hboxGui = gtk_hbox_new(FALSE, 12);
	gtk_box_pack_start(GTK_BOX(hboxGui), GTK_WIDGET(guiLabel), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hboxGui), GTK_WIDGET(guiEntry), TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hboxGui), GTK_WIDGET(browseGuiButton), FALSE, FALSE, 0);

	// Pack it into an alignment so that it's indented.
	GtkWidget* alignment = gtkutil::LeftAlignment(GTK_WIDGET(hboxGui), 18, 1.0); 
	gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(alignment), FALSE, FALSE, 0);

	// Page title and body edit fields: Create a 3x3 table
	GtkTable* tablePE = GTK_TABLE(gtk_table_new(4, 3, FALSE));
	gtk_table_set_row_spacings(tablePE, 6);
	gtk_table_set_col_spacings(tablePE, 12);
	gtk_table_set_row_spacing(tablePE, 2, 0);
	gtk_table_set_col_spacing(tablePE, 2, 0);

	_widgets[WIDGET_TEXTVIEW_TABLE] = GTK_WIDGET(tablePE);

	// Pack it into an alignment and add it to vbox
	GtkWidget* alignmentTable = gtkutil::LeftAlignment(GTK_WIDGET(tablePE), 18, 1.0);
	gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(alignmentTable), TRUE, TRUE, 0);
	gint curRow = 0;

	// Create "left" and "right" labels and add them to the first row of the table
	GtkWidget* pageLeftLabel = gtk_label_new(_("Left"));
	gtk_label_set_justify(GTK_LABEL(pageLeftLabel), GTK_JUSTIFY_CENTER);
	_widgets[WIDGET_PAGE_LEFT] = pageLeftLabel;

	GtkWidget* pageRightLabel = gtk_label_new(_("Right"));
	gtk_label_set_justify(GTK_LABEL(pageRightLabel), GTK_JUSTIFY_CENTER);
	_widgets[WIDGET_PAGE_RIGHT] = pageRightLabel;

	gtk_table_attach(tablePE, pageLeftLabel, 1, 2, curRow, curRow+1, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(tablePE, pageRightLabel, 2, 3, curRow, curRow+1, GTK_FILL, GTK_FILL, 0, 0);

	curRow++;

	// Create "title" label and title-textViews and add them to the second row of the table. Add the key-press-event.
	GtkWidget* titleLabel = gtkutil::LeftAlignedLabel(_("Title:"));

	GtkWidget* textViewTitle = gtk_text_view_new();
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(textViewTitle), GTK_WRAP_WORD);
	_widgets[WIDGET_PAGE_TITLE] = textViewTitle;

	GtkTextBuffer* buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textViewTitle));
	g_signal_connect(G_OBJECT(buffer), "changed", G_CALLBACK(onTextChanged), this);

	GtkWidget* textViewRightTitle = gtk_text_view_new();
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(textViewRightTitle), GTK_WRAP_WORD);
	_widgets[WIDGET_PAGE_RIGHT_TITLE] = textViewRightTitle;

	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textViewRightTitle));
	g_signal_connect(G_OBJECT(buffer), "changed", G_CALLBACK(onTextChanged), this);
	_widgets[WIDGET_PAGE_RIGHT_TITLE_SCROLLED] = gtkutil::ScrolledFrame(textViewRightTitle);

	gtk_table_attach(tablePE, titleLabel, 0, 1, curRow, curRow+1, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(tablePE, gtkutil::ScrolledFrame(textViewTitle), 1, 2, curRow, curRow+1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GTK_FILL, 0, 0);
	gtk_table_attach(tablePE, _widgets[WIDGET_PAGE_RIGHT_TITLE_SCROLLED], 2, 3, curRow, curRow+1, GtkAttachOptions(GTK_FILL |GTK_EXPAND), GTK_FILL, 0, 0);

	curRow++;

	// Create "body" label and body-textViews and add them to the third row of the table. Add the key-press-event.
	GtkWidget* bodyLabel = gtkutil::LeftAlignedLabel(_("Body:"));

	GtkWidget* textViewBody = gtk_text_view_new();
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(textViewBody), GTK_WRAP_WORD);
	_widgets[WIDGET_PAGE_BODY] = textViewBody;
	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textViewBody));
	g_signal_connect(G_OBJECT(buffer), "changed", G_CALLBACK(onTextChanged), this);

	GtkWidget* textViewRightBody = gtk_text_view_new();
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(textViewRightBody), GTK_WRAP_WORD);
	_widgets[WIDGET_PAGE_RIGHT_BODY] = textViewRightBody;
	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textViewRightBody));
	g_signal_connect(G_OBJECT(buffer), "changed", G_CALLBACK(onTextChanged), this);
	_widgets[WIDGET_PAGE_RIGHT_BODY_SCROLLED] = gtkutil::ScrolledFrame(textViewRightBody);

	gtk_table_attach(tablePE, bodyLabel, 0, 1, curRow, curRow+1, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach_defaults(tablePE, gtkutil::ScrolledFrame(textViewBody), 1, 2, curRow, curRow+1);
	gtk_table_attach_defaults(tablePE, _widgets[WIDGET_PAGE_RIGHT_BODY_SCROLLED], 2, 3, curRow, curRow+1);

	return vbox;
}

void ReadableEditorDialog::createMenus()
{
	// Insert menu
	GtkWidget* mIns = gtk_menu_new();

	GtkWidget* insWhole = gtk_menu_item_new_with_label(_("Insert whole Page"));
	g_signal_connect(G_OBJECT(insWhole), "activate", G_CALLBACK(onInsertWhole), this);
	gtk_menu_shell_append(GTK_MENU_SHELL(mIns), insWhole);

	GtkWidget* insLeft = gtk_menu_item_new_with_label(_("Insert on left Side"));
	g_signal_connect(G_OBJECT(insLeft), "activate", G_CALLBACK(onInsertLeft), this);
	gtk_menu_shell_append(GTK_MENU_SHELL(mIns), insLeft);

	GtkWidget* insRight = gtk_menu_item_new_with_label(_("Insert on right Side"));
	g_signal_connect(G_OBJECT(insRight), "activate", G_CALLBACK(onInsertRight), this);
	gtk_menu_shell_append(GTK_MENU_SHELL(mIns), insRight);

	gtk_widget_show_all(mIns);
	_widgets[WIDGET_MENU_INSERT] = mIns;

	// Delete Menu
	GtkWidget* mDel = gtk_menu_new();

	GtkWidget* delWhole = gtk_menu_item_new_with_label(_("Delete whole Page"));
	g_signal_connect(G_OBJECT(delWhole), "activate", G_CALLBACK(onDeleteWhole), this);
	gtk_menu_shell_append(GTK_MENU_SHELL(mDel), delWhole);

	GtkWidget* delLeft = gtk_menu_item_new_with_label(_("Delete on left Side"));
	g_signal_connect(G_OBJECT(delLeft), "activate", G_CALLBACK(onDeleteLeft), this);
	gtk_menu_shell_append(GTK_MENU_SHELL(mDel), delLeft);

	GtkWidget* delRight = gtk_menu_item_new_with_label(_("Delete on right Side"));
	g_signal_connect(G_OBJECT(delRight), "activate", G_CALLBACK(onDeleteRight), this);
	gtk_menu_shell_append(GTK_MENU_SHELL(mDel), delRight);

	gtk_widget_show_all(mDel);
	_widgets[WIDGET_MENU_DELETE] = mDel;

	// Append Menu
	GtkWidget* mApp = gtk_menu_new();

	GtkWidget* append = gtkutil::StockIconMenuItem(GTK_STOCK_ADD, _("Append Page"));
	g_signal_connect(G_OBJECT(append), "activate", G_CALLBACK(onMenuAppend), this);
	gtk_menu_shell_append(GTK_MENU_SHELL(mApp), append);

	gtk_widget_show_all(mApp);
	_widgets[WIDGET_MENU_APPEND] = mApp;

	// Prepend Menu
	GtkWidget* mPre = gtk_menu_new();

	GtkWidget* prepend = gtkutil::StockIconMenuItem(GTK_STOCK_ADD, _("Prepend Page"));
	g_signal_connect(G_OBJECT(prepend), "activate", G_CALLBACK(onMenuPrepend), this);
	gtk_menu_shell_append(GTK_MENU_SHELL(mPre), prepend);

	gtk_widget_show_all(mPre);
	_widgets[WIDGET_MENU_PREPEND] = mPre;

	// Tools Menu
	GtkWidget* mTools = gtk_menu_new();

	GtkWidget* impSum = gtkutil::StockIconMenuItem(GTK_STOCK_DND, _("Show last XData import summary"));
	g_signal_connect(G_OBJECT(impSum), "activate", G_CALLBACK(onXdImpSum), this);
	gtk_menu_shell_append(GTK_MENU_SHELL(mTools), impSum);

	GtkWidget* dupDef = gtkutil::StockIconMenuItem(GTK_STOCK_COPY, _("Show duplicated definitions"));
	g_signal_connect(G_OBJECT(dupDef), "activate", G_CALLBACK(onDupDef), this);
	gtk_menu_shell_append(GTK_MENU_SHELL(mTools), dupDef);

	GtkWidget* guiImp = gtkutil::StockIconMenuItem(GTK_STOCK_DND, _("Show Gui import summary"));
	g_signal_connect(G_OBJECT(guiImp), "activate", G_CALLBACK(onGuiImpSum), this);
	gtk_menu_shell_append(GTK_MENU_SHELL(mTools), guiImp);

	gtk_widget_show_all(mTools);
	_widgets[WIDGET_MENU_TOOLS] = mTools;
}

GtkWidget* ReadableEditorDialog::createButtonPanel()
{
	GtkWidget* hbx = gtk_hbox_new(TRUE, 6);

	GtkWidget* cancelButton = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
	g_signal_connect(G_OBJECT(cancelButton), "clicked", G_CALLBACK(onCancel), this);
	
	_widgets[WIDGET_SAVEBUTTON] = gtk_button_new_from_stock(GTK_STOCK_SAVE);
	g_signal_connect(G_OBJECT(_widgets[WIDGET_SAVEBUTTON]), "clicked", G_CALLBACK(onSave), this);
	
	GtkWidget* saveCloseButton = gtk_button_new_with_label(_("Save & Close"));
	gtk_button_set_image(GTK_BUTTON(saveCloseButton), gtk_image_new_from_stock(GTK_STOCK_APPLY, GTK_ICON_SIZE_LARGE_TOOLBAR));
	g_signal_connect(G_OBJECT(saveCloseButton), "clicked", G_CALLBACK(onSaveClose), this);

	GtkWidget* toolsButton = gtk_button_new_with_label(_("Tools"));
	gtk_button_set_image(GTK_BUTTON(toolsButton), gtk_image_new_from_stock(GTK_STOCK_PREFERENCES, GTK_ICON_SIZE_SMALL_TOOLBAR) );
	g_signal_connect(G_OBJECT(toolsButton), "clicked", G_CALLBACK(onToolsClicked), this);

	gtk_box_pack_end(GTK_BOX(hbx), saveCloseButton, TRUE, TRUE, 0);
	gtk_box_pack_end(GTK_BOX(hbx), _widgets[WIDGET_SAVEBUTTON], TRUE, TRUE, 0);
	gtk_box_pack_end(GTK_BOX(hbx), cancelButton, TRUE, TRUE, 0);
	gtk_box_pack_end(GTK_BOX(hbx), toolsButton, TRUE, TRUE, 18);

	return gtkutil::RightAlignment(hbx);
}

//////////////////////////////////////////////////////////////////////////////
// Private Methods:

bool ReadableEditorDialog::initControlsFromEntity()
{
	// Inv_name
	gtk_entry_set_text(GTK_ENTRY(_widgets[WIDGET_READABLE_NAME]), _entity->getKeyValue("inv_name").c_str());

	// Xdata contents
	gtk_entry_set_text(GTK_ENTRY(_widgets[WIDGET_XDATA_NAME]), _entity->getKeyValue("xdata_contents").c_str());

	// Construct the map-based Filename
	_mapBasedFilename = GlobalMapModule().getMapName();
	std::size_t nameStartPos = _mapBasedFilename.rfind("/") + 1;
	if (nameStartPos != std::string::npos)
	{
		_mapBasedFilename = _mapBasedFilename.substr(nameStartPos, _mapBasedFilename.rfind(".") - nameStartPos);
	}
	std::string defaultXdName = "readables/" + _mapBasedFilename + "/" + _("<Name_Here>");
	_mapBasedFilename += ".xd";

	// Load xdata
	if (!_entity->getKeyValue("xdata_contents").empty())
	{
		XdFileChooserDialog::Result result = XdFileChooserDialog::import( 
			_entity->getKeyValue("xdata_contents"), _xData, _xdFilename, _xdLoader, this
		);

		switch (result)
		{
			case XdFileChooserDialog::RESULT_CANCEL:
				return false;
			case XdFileChooserDialog::RESULT_IMPORT_FAILED:
			{
				std::string msg = (boost::format(_("Failed to import %s.")) % _entity->getKeyValue("xdata_contents")).str();
				msg += "\n";
				msg += _("Creating a new XData definition...");
				msg += "\n\n";
				msg += _("Do you want to open the import summary?");

				ui::IDialogPtr dialog = GlobalDialogManager().createMessageBox(_("Import failed"),
					msg,
					ui::IDialog::MESSAGE_ASK, GTK_WINDOW(getWindow()));

				if (dialog->run() == ui::IDialog::RESULT_YES)
				{
					showXdImportSummary();
				}
				updateGuiView();
				break;
			}
			default:	//Import success
				_xdNameSpecified = true;
				_useDefaultFilename = false;
				refreshWindowTitle();
				return true;
		}
	}

	//No Xdata definition was defined or failed to import. Use default filename and create a OneSidedXData-object
	if (_entity->getKeyValue("name").find("book") == std::string::npos)
	{
		_xData.reset(new XData::OneSidedXData(defaultXdName));
	}
	else
	{
		_xData.reset(new XData::TwoSidedXData(defaultXdName));
	}
	_xData->setNumPages(1);

	refreshWindowTitle();
	return true;
}

bool ReadableEditorDialog::save()
{
	_saveInProgress = true;

	// Name
	_entity->setKeyValue("inv_name", gtk_entry_get_text(GTK_ENTRY(_widgets[WIDGET_READABLE_NAME])));

	// Xdata contents
	_entity->setKeyValue("xdata_contents", gtk_entry_get_text(GTK_ENTRY(_widgets[WIDGET_XDATA_NAME])));

	// Current content to XData Object
	storeXData();

	// Get the storage path and check its validity
	std::string storagePath = constructStoragePath();

	if (!_useDefaultFilename && !boost::filesystem::exists(storagePath))
	{
		// The file does not exist, so we have imported a definition contained inside a PK4.
		gtkutil::errorDialog(
			_("You have imported an XData definition that is contained in a PK4, which can't be accessed for saving.") + 
			std::string("\n\n") + 
			_("Please rename your XData definition, so that it is stored under a different filename."), 
			GTK_WINDOW(getWindow())
		);

		_saveInProgress = false;
		return false;
	}

	// Start exporting
	XData::FileStatus fst = _xData->xport(storagePath, XData::Merge);

	if (fst == XData::DefinitionExists)
	{
		switch (_xData->xport( storagePath, XData::MergeOverwriteExisting))
		{
		case XData::OpenFailed: 
			gtkutil::errorDialog(
				(boost::format(_("Failed to open %s for saving.")) % _xdFilename).str(),
				GTK_WINDOW(getWindow())
			);
			_saveInProgress = false;
			return false;
		case XData::MergeFailed: 
			gtkutil::errorDialog(
				_("Merging failed, because the length of the definition to be overwritten could not be retrieved."),
				GTK_WINDOW(getWindow())
			);
			_saveInProgress = false;
			return false;
		default: 
			//success!
			_saveInProgress = false;
			return true;
		}
	}
	else if (fst == XData::OpenFailed)
	{
		gtkutil::errorDialog(
			(boost::format(_("Failed to open %s for saving.")) % _xdFilename).str(),
			GTK_WINDOW(this->getWindow())
		);
	}

	_saveInProgress = false;
	return false;
}

std::string ReadableEditorDialog::constructStoragePath()
{
	// Construct the storage path from Registry keys.
	std::string storagePath;
	if (_useDefaultFilename)
	{
		switch (GlobalRegistry().getInt(RKEY_READABLES_STORAGE_FOLDER))
		{
			case 0: // Use Mod dir
				storagePath = GlobalGameManager().getModPath();
				if (storagePath.empty())
				{
					// Mod path not defined. Use base Path
					storagePath = GlobalRegistry().get(RKEY_ENGINE_PATH) + "base/";
					gtkutil::errorDialog(_("Mod path not defined. Using Base path..."), GTK_WINDOW(getWindow()));
				}
				storagePath += XData::XDATA_DIR + _mapBasedFilename;
				break;
			case 1: // Use Mod Base dir
				storagePath = GlobalGameManager().getModBasePath();
				if (storagePath.empty())
				{
					// Mod Base Path not defined. Use Mod path or base path successively.
					storagePath = GlobalGameManager().getModPath();
					if (storagePath.empty())
					{
						storagePath = GlobalRegistry().get(RKEY_ENGINE_PATH) + "base/";
						gtkutil::errorDialog(_("Mod Base path not defined, neither is Mod path. Using Engine path..."), 
							GTK_WINDOW(getWindow()));
						storagePath += XData::XDATA_DIR + _mapBasedFilename;
						break;
					}

					gtkutil::errorDialog(_("Mod Base path not defined. Using Mod path..."), GTK_WINDOW(getWindow()));
				}
				storagePath += XData::XDATA_DIR + _mapBasedFilename;
				break;
			default: // Use custom folder
				storagePath = GlobalRegistry().get(RKEY_READABLES_CUSTOM_FOLDER);
				if (storagePath.empty())
				{
					// Custom path not defined. Use Mod path or base path successively.
					storagePath = GlobalGameManager().getModPath();
					if (storagePath.empty())
					{
						storagePath = GlobalRegistry().get(RKEY_ENGINE_PATH) + "base/";
						gtkutil::errorDialog(_("Mod Base path not defined, neither is Mod path. Using Engine path..."), GTK_WINDOW(getWindow()));
						storagePath += XData::XDATA_DIR + _mapBasedFilename;
						break;
					}
					storagePath += XData::XDATA_DIR + _mapBasedFilename;
					gtkutil::errorDialog(_("Mod Base path not defined. Using Mod path..."), GTK_WINDOW(getWindow()));
					break;
				}
				storagePath += "/" + _mapBasedFilename;
				break;
		}
	}
	else
	{
		// We are exporting a previously imported XData definition. Retrieve engine path and append _xdFilename
		storagePath = GlobalRegistry().get(RKEY_ENGINE_PATH) + _xdFilename;
	}

	return storagePath;
}

void ReadableEditorDialog::refreshWindowTitle()
{
	std::string title = constructStoragePath();
	title = title.substr( title.find_first_not_of( GlobalRegistry().get(RKEY_ENGINE_PATH) ) );
	title = std::string(_("Readable Editor")) +  "  -  " + title;

	gtk_window_set_title(GTK_WINDOW(getWindow()), title.c_str());
}

void ReadableEditorDialog::storeXData()
{
	//NumPages does not need storing, because it's stored directly after changing it.
	_xData->setName(gtk_entry_get_text(GTK_ENTRY(_widgets[WIDGET_XDATA_NAME])));
	_xData->setSndPageTurn(gtk_entry_get_text(GTK_ENTRY(_widgets[WIDGET_PAGETURN_SND])));

	storeCurrentPage();
}

void ReadableEditorDialog::toggleTwoSidedEditingInterface(bool show)
{
	if (show)
	{
		gtk_table_set_col_spacing(GTK_TABLE(_widgets[WIDGET_TEXTVIEW_TABLE]), 1, 12);
		gtk_widget_show(_widgets[WIDGET_PAGE_RIGHT_BODY_SCROLLED]);
		gtk_widget_show(_widgets[WIDGET_PAGE_RIGHT_TITLE_SCROLLED]);
		gtk_widget_show(_widgets[WIDGET_PAGE_LEFT]);
		gtk_widget_show(_widgets[WIDGET_PAGE_RIGHT]);
	}
	else
	{
		gtk_table_set_col_spacing(GTK_TABLE(_widgets[WIDGET_TEXTVIEW_TABLE]), 1, 0);
		gtk_widget_hide(_widgets[WIDGET_PAGE_RIGHT_BODY_SCROLLED]);
		gtk_widget_hide(_widgets[WIDGET_PAGE_RIGHT_TITLE_SCROLLED]);
		gtk_widget_hide(_widgets[WIDGET_PAGE_LEFT]);
		gtk_widget_hide(_widgets[WIDGET_PAGE_RIGHT]);
	}
}

void ReadableEditorDialog::showPage(std::size_t pageIndex)
{
	//Gui Def before:
	std::string guiBefore = gtk_entry_get_text(GTK_ENTRY(_widgets[WIDGET_GUI_ENTRY]));

	// Update CurrentPage Label
	_currentPageIndex = pageIndex;
	gtk_label_set_text(GTK_LABEL(_widgets[WIDGET_CURRENT_PAGE]), sizetToStr(pageIndex+1).c_str());

	if (_xData->getPageLayout() == XData::TwoSided)
	{
		// Update Gui statement entry from xData
		if (!_xData->getGuiPage(pageIndex).empty())
		{
			gtk_entry_set_text(GTK_ENTRY(_widgets[WIDGET_GUI_ENTRY]), _xData->getGuiPage(pageIndex).c_str());
		}
		else
		{
			gtk_entry_set_text(GTK_ENTRY(_widgets[WIDGET_GUI_ENTRY]), XData::DEFAULT_TWOSIDED_GUI);
		}

		setTextViewAndScroll(WIDGET_PAGE_RIGHT_TITLE, _xData->getPageContent(XData::Title, pageIndex, XData::Right));
		setTextViewAndScroll(WIDGET_PAGE_RIGHT_BODY, _xData->getPageContent(XData::Body, pageIndex, XData::Right));
	}
	else
	{
		// Update Gui statement entry from xData
		if (!_xData->getGuiPage(pageIndex).empty())
		{
			gtk_entry_set_text(GTK_ENTRY(_widgets[WIDGET_GUI_ENTRY]), _xData->getGuiPage(pageIndex).c_str());
		}
		else
		{
			gtk_entry_set_text(GTK_ENTRY(_widgets[WIDGET_GUI_ENTRY]), XData::DEFAULT_ONESIDED_GUI);
		}
	}

	// Update page statements textviews from xData
	setTextViewAndScroll(WIDGET_PAGE_TITLE, _xData->getPageContent(XData::Title, pageIndex, XData::Left));
	setTextViewAndScroll(WIDGET_PAGE_BODY, _xData->getPageContent(XData::Body, pageIndex, XData::Left));

	// Update the GUI View if the gui changed. For the page contents, updateGuiView is called automatically
	// by onTextChange.
	if (guiBefore != gtk_entry_get_text(GTK_ENTRY(_widgets[WIDGET_GUI_ENTRY])))
		updateGuiView();
}

void ReadableEditorDialog::updateGuiView(GtkWindow* parent, const std::string& guiPath, const std::string& xDataName, const std::string& xDataPath)
{
	// If the name of an xData object is passed it will be rendered instead of the current
	// xData object, to enable previewing of XData definitions induced by the XDataSelector.
	if (!xDataName.empty())
	{
		XData::XDataPtr xd;
		XData::XDataMap xdMap;

		if (_xdLoader->importDef(xDataName, xdMap, xDataPath))
		{
			xd = xdMap.begin()->second;
			_guiView->setGui(xd->getGuiPage(0));
		}
		else
		{
			std::string msg = (boost::format(_("Failed to import %s.")) % xDataName).str();
			msg += "\n\n";
			msg += _("Do you want to open the import summary?");

			ui::IDialogPtr dialog = GlobalDialogManager().createMessageBox(_("Import failed"),
				msg,
				ui::IDialog::MESSAGE_ASK,
				parent != NULL ? parent : GTK_WINDOW(getWindow())
			);
			if (dialog->run() == ui::IDialog::RESULT_YES)
			{
				showXdImportSummary();
			}
			return;
		}

		const gui::GuiPtr& gui = _guiView->getGui();

		if (gui == NULL)
		{
			std::string msg = (boost::format(_("Failed to load gui definition %s.")) % xd->getGuiPage(0)).str();
			msg += "\n\n";
			msg += _("Do you want to open the import summary?");

			ui::IDialogPtr dialog = GlobalDialogManager().createMessageBox(_("Import failed"),
				msg,
				ui::IDialog::MESSAGE_ASK,
				parent != NULL ? parent : GTK_WINDOW(getWindow())
			);
			if (dialog->run() == ui::IDialog::RESULT_YES)
			{
				showGuiImportSummary();
			}
			return;
		}

		// Load data from xdata into the GUI's state variables
		if (xd->getPageLayout() == XData::OneSided)
		{
			// One-sided has title and body
			gui->setStateString("title", xd->getPageContent(XData::Title, 0, XData::Left));
			gui->setStateString("body", xd->getPageContent(XData::Body, 0, XData::Left));
		}
		else
		{
			// Two-sided has four important state strings
			gui->setStateString("left_title", xd->getPageContent(XData::Title, 0, XData::Left));
			gui->setStateString("left_body", xd->getPageContent(XData::Body, 0, XData::Left));

			gui->setStateString("right_title", xd->getPageContent(XData::Title, 0, XData::Right));
			gui->setStateString("right_body", xd->getPageContent(XData::Body, 0, XData::Right));
		}

		// Initialise the time of this GUI
		gui->initTime(0);

		// Run the first frame
		gui->update(16);
	}
	else
	{
		if (guiPath.empty())
		{
			_guiView->setGui(gtk_entry_get_text(GTK_ENTRY(_widgets[WIDGET_GUI_ENTRY])));
		}
		else
		{
			_guiView->setGui(guiPath);
		}

		const gui::GuiPtr& gui = _guiView->getGui();

		if (gui == NULL)
		{
			std::string nameGui = guiPath.empty() ? gtk_entry_get_text(GTK_ENTRY(_widgets[WIDGET_GUI_ENTRY])) : guiPath;

			std::string msg = (boost::format(_("Failed to load gui definition %s.")) % nameGui).str();
			msg += "\n\n";
			msg += _("Do you want to open the import summary?");

			ui::IDialogPtr dialog = GlobalDialogManager().createMessageBox(_("Import failed"),
				msg,
				ui::IDialog::MESSAGE_ASK,
				parent != NULL ? parent : GTK_WINDOW(this->getWindow())
			);
			if (dialog->run() == ui::IDialog::RESULT_YES)
			{
				showGuiImportSummary();
			}
			return;
		}

		// Load data from xdata into the GUI's state variables
		if (_xData->getPageLayout() == XData::OneSided)
		{
			// One-sided has title and body
			gui->setStateString("title", readTextBuffer(WIDGET_PAGE_TITLE));
			gui->setStateString("body", readTextBuffer(WIDGET_PAGE_BODY));
		}
		else
		{
			// Two-sided has four important state strings
			gui->setStateString("left_title", readTextBuffer(WIDGET_PAGE_TITLE));
			gui->setStateString("left_body", readTextBuffer(WIDGET_PAGE_BODY));

			gui->setStateString("right_title", readTextBuffer(WIDGET_PAGE_RIGHT_TITLE));
			gui->setStateString("right_body", readTextBuffer(WIDGET_PAGE_RIGHT_BODY));
		}

		// Initialise the time of this GUI
		gui->initTime(0);

		// Run the first frame
		gui->update(16);
	}

	_guiView->redraw();
}

void ReadableEditorDialog::storeCurrentPage()
{
	// Store the GUI-Page
	_xData->setGuiPage( gtk_entry_get_text(GTK_ENTRY(_widgets[WIDGET_GUI_ENTRY])), _currentPageIndex);

	// On OneSidedXData the Side-enum is discarded, so it's save to call this method
	_xData->setPageContent(XData::Title, _currentPageIndex, XData::Left, readTextBuffer(WIDGET_PAGE_TITLE));
	_xData->setPageContent(XData::Body, _currentPageIndex, XData::Left, readTextBuffer(WIDGET_PAGE_BODY));

	if (_xData->getPageLayout() == XData::TwoSided)
	{
		_xData->setPageContent(XData::Title, _currentPageIndex, XData::Right, readTextBuffer(WIDGET_PAGE_RIGHT_TITLE));
		_xData->setPageContent(XData::Body, _currentPageIndex, XData::Right, readTextBuffer(WIDGET_PAGE_RIGHT_BODY));
	}
}

void ReadableEditorDialog::populateControlsFromXData()
{
	toggleTwoSidedEditingInterface(_xData->getPageLayout() == XData::TwoSided);
	showPage(0);

	gtk_entry_set_text( GTK_ENTRY(_widgets[WIDGET_XDATA_NAME]), _xData->getName().c_str());
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(_widgets[WIDGET_NUMPAGES]), static_cast<gdouble>(_xData->getNumPages()));

	std::string sndString = _xData->getSndPageTurn();

	gtk_entry_set_text( 
		GTK_ENTRY(_widgets[WIDGET_PAGETURN_SND]),
		sndString.empty() ? XData::DEFAULT_SNDPAGETURN : sndString.c_str()
	);

	if (_xData->getPageLayout() == XData::TwoSided)
	{
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(_widgets[WIDGET_RADIO_TWOSIDED]), TRUE);
	}
	else
	{
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(_widgets[WIDGET_RADIO_ONESIDED]), TRUE);
	}
}

void ReadableEditorDialog::checkXDataUniqueness()
{
	_runningXDataUniquenessCheck = true;

	std::string xdn = gtk_entry_get_text(GTK_ENTRY(_widgets[WIDGET_XDATA_NAME]));

	if (_xData->getName() == xdn)
	{
		_runningXDataUniquenessCheck = false;
		return;
	}

	_xdLoader->retrieveXdInfo();

	XData::StringVectorMap::const_iterator it = _xdLoader->getDefinitionList().find(xdn);

	if (it != _xdLoader->getDefinitionList().end())
	{
		// The definition already exists. Ask the user whether it should be imported. If not make a different name suggestion.
		IDialogPtr popup = GlobalDialogManager().createMessageBox(
			_("Import definition?"), 
			(boost::format(_("The definition %s already exists. Should it be imported?")) % xdn).str(),
			ui::IDialog::MESSAGE_ASK, GTK_WINDOW(getWindow())
		);
		
		std::string message = "";

		if (popup->run() == ui::IDialog::RESULT_YES)
		{
			switch (XdFileChooserDialog::import( xdn, _xData, _xdFilename, _xdLoader, this))
			{
			case XdFileChooserDialog::RESULT_CANCEL:
				break;
			case XdFileChooserDialog::RESULT_IMPORT_FAILED:
				message = _("Import failed:");
				message += "\n\t" + _xdLoader->getImportSummary()[_xdLoader->getImportSummary().size() - 1];
				message += "\n\n";
				message += _("Consult the import summary for further information.");
				message += "\n\n";
				break;
			default:	//Import success
				_xdNameSpecified = true;
				_useDefaultFilename = false;
				populateControlsFromXData();
				_runningXDataUniquenessCheck = false;
				refreshWindowTitle();
				return;
			}
		}

		// Dialog RESULT_NO, XdFileChooserDialog::RESULT_CANCEL or import failed! Make a different name suggestion!
		std::string suggestion;

		for (int n = 1; n > 0; n++)
		{
			suggestion = xdn + intToStr(n);

			if (_xdLoader->getDefinitionList().find(suggestion) == _xdLoader->getDefinitionList().end())
			{
				// The suggested XData-name does not exist.
				break;
			}
		}

		// Update entry and XData object. Notify the user about the suggestion.
		gtk_entry_set_text(GTK_ENTRY(_widgets[WIDGET_XDATA_NAME]), suggestion.c_str());
		_xData->setName(suggestion);

		message += (boost::format(_("To avoid duplicated XData definitions "
			"the current definition has been renamed to %s.")) % suggestion).str();

		popup = GlobalDialogManager().createMessageBox(
			_("XData has been renamed."), 
			message,
			IDialog::MESSAGE_CONFIRM, GTK_WINDOW(getWindow())
		);
		popup->run();
	}
	else
	{
		_xData->setName(xdn);
	}

	_xdNameSpecified = true;
	_useDefaultFilename = true;
	_runningXDataUniquenessCheck = false;
	refreshWindowTitle();
}

void ReadableEditorDialog::insertPage()
{
	storeCurrentPage();
	_xData->setNumPages(_xData->getNumPages()+1);

	gtk_spin_button_set_value(GTK_SPIN_BUTTON(_widgets[WIDGET_NUMPAGES]), static_cast<gdouble>(_xData->getNumPages()));

	for (std::size_t n = _xData->getNumPages() - 1; n > _currentPageIndex; n--)
	{
		_xData->setGuiPage(_xData->getGuiPage(n - 1), n);

		_xData->setPageContent(XData::Title, n, XData::Left,
			_xData->getPageContent(XData::Title, n - 1, XData::Left)
		);

		_xData->setPageContent(XData::Body, n, XData::Left,
			_xData->getPageContent(XData::Body, n - 1, XData::Left)
		);
	}

	// New Page:
	_xData->setPageContent(XData::Title, _currentPageIndex, XData::Left, "");
	_xData->setPageContent(XData::Body, _currentPageIndex, XData::Left, "");
	_xData->setGuiPage(_xData->getGuiPage(_currentPageIndex + 1), _currentPageIndex);

	// Right side:
	if (_xData->getPageLayout() == XData::TwoSided)
	{
		for (std::size_t n = _xData->getNumPages() - 1; n > _currentPageIndex; n--)
		{
			_xData->setGuiPage(_xData->getGuiPage(n - 1), n);

			_xData->setPageContent(XData::Title, n, XData::Right,
				_xData->getPageContent(XData::Title, n - 1, XData::Right)
			);

			_xData->setPageContent(XData::Body, n, XData::Right,
				_xData->getPageContent(XData::Body, n - 1, XData::Right)
			);
		}

		// New Page:
		_xData->setPageContent(XData::Title, _currentPageIndex, XData::Right, "");
		_xData->setPageContent(XData::Body, _currentPageIndex, XData::Right, "");
	}

	showPage(_currentPageIndex);
}

void ReadableEditorDialog::deletePage()
{
	if (_currentPageIndex == _xData->getNumPages() - 1)
	{
		if (_currentPageIndex == 0)
		{
			_xData->setNumPages(0);
			_xData->setNumPages(1);
			showPage(0);
			return;
		}

		gtk_spin_button_set_value(GTK_SPIN_BUTTON(_widgets[WIDGET_NUMPAGES]), static_cast<gdouble>(_currentPageIndex));
	}
	else
	{
		for (std::size_t n = _currentPageIndex; n < _xData->getNumPages()-1; n++)
		{
			_xData->setGuiPage(_xData->getGuiPage(n+1), n);

			_xData->setPageContent(XData::Title, n, XData::Left,
				_xData->getPageContent(XData::Title, n+1, XData::Left)
			);

			_xData->setPageContent(XData::Body, n, XData::Left,
				_xData->getPageContent(XData::Body, n+1, XData::Left)
			);
		}

		// Right Side
		if (_xData->getPageLayout() == XData::TwoSided)
		{
			for (std::size_t n = _currentPageIndex; n < _xData->getNumPages()-1; n++)
			{
				_xData->setGuiPage(_xData->getGuiPage(n+1), n);

				_xData->setPageContent(XData::Title, n, XData::Right,
					_xData->getPageContent(XData::Title, n+1, XData::Right)
				);

				_xData->setPageContent(XData::Body, n, XData::Right,
					_xData->getPageContent(XData::Body, n+1, XData::Right)
				);
			}
		}

		_xData->setNumPages(_xData->getNumPages() - 1);

		gtk_spin_button_set_value(GTK_SPIN_BUTTON(_widgets[WIDGET_NUMPAGES]), static_cast<gdouble>(_xData->getNumPages()));

		showPage(_currentPageIndex);
	}
}


void ReadableEditorDialog::deleteSide(bool rightSide)
{
	storeCurrentPage();

	if (!rightSide)
	{
		_xData->setPageContent(XData::Title, _currentPageIndex, XData::Left,
			_xData->getPageContent(XData::Title, _currentPageIndex, XData::Right)
		);

		_xData->setPageContent(XData::Body, _currentPageIndex, XData::Left,
			_xData->getPageContent(XData::Body, _currentPageIndex, XData::Right)
		);
	}

	if (_currentPageIndex < _xData->getNumPages() - 1)
	{
		_xData->setPageContent(XData::Title, _currentPageIndex, XData::Right,
			_xData->getPageContent(XData::Title, _currentPageIndex + 1, XData::Left)
		);
		
		_xData->setPageContent(XData::Body, _currentPageIndex, XData::Right,
			_xData->getPageContent(XData::Body, _currentPageIndex + 1, XData::Left)
		);

		for (std::size_t n = _currentPageIndex + 1; n < _xData->getNumPages() - 1; n++)
		{
			_xData->setPageContent(XData::Title, n, XData::Left,
				_xData->getPageContent(XData::Title, n, XData::Right));

			_xData->setPageContent(XData::Title, n, XData::Right,
				_xData->getPageContent(XData::Title, n+1, XData::Left));

			_xData->setPageContent(XData::Body, n, XData::Left,
				_xData->getPageContent(XData::Body, n, XData::Right));

			_xData->setPageContent(XData::Body, n, XData::Right,
				_xData->getPageContent(XData::Body, n+1, XData::Left));
		}

		_xData->setPageContent(XData::Title, _xData->getNumPages() - 1, XData::Left,
			_xData->getPageContent(XData::Title, _xData->getNumPages() - 1, XData::Right));

		_xData->setPageContent(XData::Body, _xData->getNumPages() - 1, XData::Left,
			_xData->getPageContent(XData::Body, _xData->getNumPages() - 1, XData::Right));
	}

	if (_xData->getPageContent(XData::Title, _xData->getNumPages() - 1, XData::Left).empty() &&
		_xData->getPageContent(XData::Body, _xData->getNumPages() - 1, XData::Left).empty())
	{
		// The last page has no content anymore, so delete it.
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(_widgets[WIDGET_NUMPAGES]), static_cast<gdouble>(_xData->getNumPages() - 1));
	}
	else
	{
		_xData->setPageContent(XData::Title, _xData->getNumPages() - 1, XData::Right, "");
		_xData->setPageContent(XData::Body, _xData->getNumPages() - 1, XData::Right, "");
	}

	showPage(_currentPageIndex);
}

void ReadableEditorDialog::insertSide(bool rightSide)
{
	storeCurrentPage();

	if (!_xData->getPageContent(XData::Title, _xData->getNumPages()-1, XData::Right).empty() ||
		!_xData->getPageContent(XData::Body, _xData->getNumPages()-1, XData::Right).empty())
	{
		//Last side has content. Raise numPages before shifting
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(_widgets[WIDGET_NUMPAGES]), static_cast<gdouble>(_xData->getNumPages() + 1));
	}

	for (std::size_t n = _xData->getNumPages() - 1; n>_currentPageIndex; n--)
	{
		_xData->setPageContent(XData::Title, n, XData::Right,
			_xData->getPageContent(XData::Title, n, XData::Left));

		_xData->setPageContent(XData::Title, n, XData::Left,
			_xData->getPageContent(XData::Title, n-1, XData::Right));

		_xData->setPageContent(XData::Body, n, XData::Right,
			_xData->getPageContent(XData::Body, n, XData::Left));

		_xData->setPageContent(XData::Body, n, XData::Left,
			_xData->getPageContent(XData::Body, n-1, XData::Right));
	}

	if (!rightSide)
	{
		_xData->setPageContent(XData::Title, _currentPageIndex, XData::Right,
			_xData->getPageContent(XData::Title, _currentPageIndex, XData::Left));

		_xData->setPageContent(XData::Body, _currentPageIndex, XData::Right,
			_xData->getPageContent(XData::Body, _currentPageIndex, XData::Left));

		_xData->setPageContent(XData::Title, _currentPageIndex, XData::Left, "");
		_xData->setPageContent(XData::Body, _currentPageIndex, XData::Left, "");
	}
	else
	{
		_xData->setPageContent(XData::Title, _currentPageIndex, XData::Right, "");
		_xData->setPageContent(XData::Body, _currentPageIndex, XData::Right, "");
	}

	showPage(_currentPageIndex);
}

void ReadableEditorDialog::useOneSidedEditing()
{
	if (_xData->getPageLayout() != XData::OneSided)
	{
		toggleLayout();
	}
}

void ReadableEditorDialog::useTwoSidedEditing()
{
	if (_xData->getPageLayout() != XData::TwoSided)
	{
		toggleLayout();
	}
}

void ReadableEditorDialog::toggleLayout()
{
	// Convert OneSided XData to TwoSided and vice versa and refresh controls
	storeXData();
	_xData->togglePageLayout(_xData);
	populateControlsFromXData();
}

void ReadableEditorDialog::checkGuiLayout()
{
	_runningGuiLayoutCheck = true;

	std::string guiName = gtk_entry_get_text(GTK_ENTRY(_widgets[WIDGET_GUI_ENTRY]));

	std::string msg;
	switch ( gui::GuiManager::Instance().getGuiType(guiName) )
	{
		case gui::NO_READABLE:
			msg = _("The specified gui definition is not a readable.");
			break;
		case gui::ONE_SIDED_READABLE:
			if (_xData->getPageLayout() != XData::OneSided)
			{
				msg = _("The specified gui definition is not suitable for the currently chosen page-layout.");
			}
			else
			{
				_runningGuiLayoutCheck = false;
				updateGuiView();
				return;
			}
			break;
		case gui::TWO_SIDED_READABLE:
			if (_xData->getPageLayout() != XData::TwoSided)
			{
				msg = _("The specified gui definition is not suitable for the currently chosen page-layout.");
			}
			else
			{
				_runningGuiLayoutCheck = false;
				updateGuiView();
				return;
			}
			break;
		case gui::IMPORT_FAILURE:
			msg = _("Failure during import.");
			break;
		case gui::FILE_NOT_FOUND:
			msg = _("The specified Definition does not exist.");
			break;
	}

	IDialogPtr dialog = GlobalDialogManager().createMessageBox(
		_("Not a suitable Gui Definition!"), 
		msg + "\n\n" + std::string(_("Start the Gui Browser?")),
		IDialog::MESSAGE_ASK, GTK_WINDOW(getWindow()));

	if (dialog->run() == ui::IDialog::RESULT_YES)
	{
		XData::PageLayout layoutBefore = _xData->getPageLayout();
		std::string guiName = GuiSelector::run(_xData->getPageLayout() == XData::TwoSided, *this);

		if (!guiName.empty())
		{
			gtk_entry_set_text(GTK_ENTRY(_widgets[WIDGET_GUI_ENTRY]), guiName.c_str());
			_runningGuiLayoutCheck = false;
			updateGuiView();
			return;
		}
		else
		{
			if (_xData->getPageLayout() != layoutBefore)
			{
				toggleLayout();
			}

			// User clicked cancel. Use default layout:
			if (_xData->getPageLayout() == XData::TwoSided)
			{
				gtk_entry_set_text(GTK_ENTRY(_widgets[WIDGET_GUI_ENTRY]), XData::DEFAULT_TWOSIDED_GUI);
			}
			else
			{
				gtk_entry_set_text(GTK_ENTRY(_widgets[WIDGET_GUI_ENTRY]), XData::DEFAULT_ONESIDED_GUI);
			}

			updateGuiView();

			dialog = GlobalDialogManager().createMessageBox(_("Switching to default Gui..."),
				_("You didn't choose a Gui. Using the default Gui now."), 
				IDialog::MESSAGE_CONFIRM, GTK_WINDOW(getWindow()));
			dialog->run();
			_runningGuiLayoutCheck = false;
			return;
		}
	}
	// Entry should hold focus.
	gtk_widget_grab_focus(_widgets[WIDGET_GUI_ENTRY]);
	_runningGuiLayoutCheck = false;
}

void ReadableEditorDialog::showXdImportSummary()
{
	XData::StringList summary = _xdLoader->getImportSummary();

	if (summary.empty())
	{
		gtkutil::errorDialog(_("No import summary available. An XData definition has to be imported first..."), 
			GTK_WINDOW(getWindow()) );
		return;
	}

	std::string sum;

	for (std::size_t n = 0; n < summary.size(); n++)
	{
		sum += summary[n];
	}

	TextViewInfoDialog dialog(_("XData import summary"), sum, GTK_WINDOW(getWindow()));
	dialog.show();
}

void ReadableEditorDialog::showGuiImportSummary()
{
	XData::StringList errors = gui::GuiManager::Instance().getErrorList();
	if (errors.empty())
	{
		gtkutil::errorDialog(_("No import summary available. Browse Gui Definitions first."), GTK_WINDOW(getWindow()) );
		return;
	}

	std::string summary;

	for (std::size_t n = 0; n < errors.size(); n++)
	{
		summary += errors[n];
	}

	TextViewInfoDialog dialog(_("Gui import summary"), summary, GTK_WINDOW(getWindow()));
	dialog.show();
}

//////////////////////////////////////////////////////////////////////////////
// Callback Methods for Signals:

void ReadableEditorDialog::onInsertWhole(GtkWidget* widget, ReadableEditorDialog* self)
{
	self->insertPage();
}

void ReadableEditorDialog::onInsertLeft(GtkWidget* widget, ReadableEditorDialog* self)
{
	self->insertSide(false);
}

void ReadableEditorDialog::onInsertRight(GtkWidget* widget, ReadableEditorDialog* self)
{
	self->insertSide(true);
}

void ReadableEditorDialog::onDeleteWhole(GtkWidget* widget, ReadableEditorDialog* self)
{
	self->deletePage();
}

void ReadableEditorDialog::onDeleteLeft(GtkWidget* widget, ReadableEditorDialog* self)
{
	self->deleteSide(false);
}

void ReadableEditorDialog::onDeleteRight(GtkWidget* widget, ReadableEditorDialog* self)
{
	self->deleteSide(true);
}

void ReadableEditorDialog::onCancel(GtkWidget* widget, ReadableEditorDialog* self) 
{
	self->_result = RESULT_CANCEL;

	self->destroy();
}

void ReadableEditorDialog::onSave(GtkWidget* widget, ReadableEditorDialog* self) 
{
	if (self->_xdNameSpecified)
	{
		self->save();
	}
	else
	{
		gtkutil::errorDialog(_("Please specify an XData name first!"), GTK_WINDOW(self->getWindow()) );
	}
}

void ReadableEditorDialog::onSaveClose(GtkWidget* widget, ReadableEditorDialog* self) 
{
	if (!self->_saveInProgress)
	{
		if (self->_xdNameSpecified)
		{

			if (self->save())
			{
				// Done, just destroy the window
				self->_result = RESULT_OK;
				self->destroy();
			}
		}
		else
		{
			gtkutil::errorDialog(_("Please specify an XData name first!"), GTK_WINDOW(self->getWindow()) );
		}
	}
}

void ReadableEditorDialog::onBrowseXd(GtkWidget* widget, ReadableEditorDialog* self) 
{
	// FileChooser for xd-files
	self->_xdLoader->retrieveXdInfo();

	std::string res = XDataSelector::run(self->_xdLoader->getDefinitionList(), self);

	if (res.empty())
	{
		self->updateGuiView();
		return;
	}
	
	// Import the file:
	switch (XdFileChooserDialog::import(res, self->_xData, self->_xdFilename, self->_xdLoader, self))
	{
		case XdFileChooserDialog::RESULT_IMPORT_FAILED:
		{
			std::string msg = (boost::format(_("Failed to import %s.")) % res).str();
			msg += "\n\n";
			msg += _("Do you want to open the import summary?");

			ui::IDialogPtr dialog = GlobalDialogManager().createMessageBox(_("Import failed"),
				msg,
				ui::IDialog::MESSAGE_ASK, GTK_WINDOW(self->getWindow()));

			if (dialog->run() == ui::IDialog::RESULT_YES)
			{
				self->showXdImportSummary();
			}
			self->updateGuiView();
			return;
		}
		case XdFileChooserDialog::RESULT_OK:
			self->_xdNameSpecified = true;
			self->_useDefaultFilename = false;
			self->populateControlsFromXData();
			self->refreshWindowTitle();
			return;
		default:	// Canceled.
			self->updateGuiView();
			return;
	}
}


void ReadableEditorDialog::onNextPage(GtkWidget* widget, ReadableEditorDialog* self) 
{
	if (self->_currentPageIndex+1 < self->_xData->getNumPages())
	{
		self->storeCurrentPage();
		self->showPage(self->_currentPageIndex+1);
	}
	else
	{
		gtk_menu_popup(GTK_MENU(self->_widgets[WIDGET_MENU_APPEND]), NULL, NULL, NULL, NULL, 1, GDK_CURRENT_TIME);
	}
}

void ReadableEditorDialog::onPrevPage(GtkWidget* widget, ReadableEditorDialog* self) 
{
	if (self->_currentPageIndex > 0)
	{
		self->storeCurrentPage();
		self->showPage(self->_currentPageIndex-1);
	}
	else
	{
		gtk_menu_popup(GTK_MENU(self->_widgets[WIDGET_MENU_PREPEND]), NULL, NULL, NULL, NULL, 1, GDK_CURRENT_TIME);
	}
}

void ReadableEditorDialog::onFirstPage(GtkWidget* widget, ReadableEditorDialog* self)
{
	if (self->_currentPageIndex != 0)
	{
		self->storeCurrentPage();
		self->showPage(0);
	}
	else
	{
		gtk_menu_popup(GTK_MENU(self->_widgets[WIDGET_MENU_PREPEND]), NULL, NULL, NULL, NULL, 1, GDK_CURRENT_TIME);
	}
}

void ReadableEditorDialog::onLastPage(GtkWidget* widget, ReadableEditorDialog* self)
{
	if (self->_currentPageIndex != self->_xData->getNumPages() - 1)
	{
		self->storeCurrentPage();
		self->showPage(self->_xData->getNumPages()-1);
	}
	else
	{
		gtk_menu_popup(GTK_MENU(self->_widgets[WIDGET_MENU_APPEND]), NULL, NULL, NULL, NULL, 1, GDK_CURRENT_TIME);
	}
}

void ReadableEditorDialog::onBrowseGui(GtkWidget* widget, ReadableEditorDialog* self) 
{
	XData::PageLayout layoutBefore = self->_xData->getPageLayout();
	std::string guiDefBefore = gtk_entry_get_text(GTK_ENTRY(self->_widgets[WIDGET_GUI_ENTRY]));
	std::string guiName = GuiSelector::run(self->_xData->getPageLayout() == XData::TwoSided, *self);

	if (!guiName.empty())
	{
		gtk_entry_set_text(GTK_ENTRY(self->_widgets[WIDGET_GUI_ENTRY]), guiName.c_str());
	}
	else
	{
		if (self->_xData->getPageLayout() != layoutBefore)
			self->toggleLayout();
		if (gtk_entry_get_text(GTK_ENTRY(self->_widgets[WIDGET_GUI_ENTRY])) != guiDefBefore)
			gtk_entry_set_text(GTK_ENTRY(self->_widgets[WIDGET_GUI_ENTRY]), guiDefBefore.c_str());
		self->updateGuiView();
	}
}

void ReadableEditorDialog::onInsert(GtkWidget* widget, ReadableEditorDialog* self)
{
	if (self->_xData->getPageLayout() == XData::TwoSided)
	{
		gtk_menu_popup(GTK_MENU(self->_widgets[WIDGET_MENU_INSERT]), NULL, NULL, NULL, NULL, 1, GDK_CURRENT_TIME);
	}
	else 
	{
		self->insertPage();
	}
}

void ReadableEditorDialog::onDelete(GtkWidget* widget, ReadableEditorDialog* self)
{
	if (self->_xData->getPageLayout() == XData::TwoSided)
	{
		gtk_menu_popup(GTK_MENU(self->_widgets[WIDGET_MENU_DELETE]), NULL, NULL, NULL, NULL, 1, GDK_CURRENT_TIME);
	}
	else 
	{
		self->deletePage();
	}
}	

void ReadableEditorDialog::onValueChanged(GtkWidget* widget, ReadableEditorDialog* self)
{
	std::size_t nNP =  gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(self->_widgets[WIDGET_NUMPAGES]));

	self->_xData->setNumPages(nNP);

	if (self->_currentPageIndex >= nNP)
	{
		self->showPage(nNP - 1);
	}
}

void ReadableEditorDialog::onMenuAppend(GtkWidget* widget, ReadableEditorDialog* self)
{
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(self->_widgets[WIDGET_NUMPAGES]), static_cast<gdouble>(self->_xData->getNumPages() + 1));

	self->storeCurrentPage();
	self->showPage(self->_currentPageIndex + 1);
}

void ReadableEditorDialog::onMenuPrepend(GtkWidget* widget, ReadableEditorDialog* self)
{
	self->insertPage();
}

void ReadableEditorDialog::onToolsClicked(GtkWidget* widget, ReadableEditorDialog* self)
{
	gtk_menu_popup(GTK_MENU(self->_widgets[WIDGET_MENU_TOOLS]), NULL, NULL, NULL, NULL, 1, GDK_CURRENT_TIME);
}

void ReadableEditorDialog::onXdImpSum(GtkWidget* widget, ReadableEditorDialog* self)
{
	self->showXdImportSummary();
}

void ReadableEditorDialog::onGuiImpSum(GtkWidget* widget, ReadableEditorDialog* self)
{
	self->showGuiImportSummary();
}

void ReadableEditorDialog::onDupDef(GtkWidget* widget, ReadableEditorDialog* self)
{
	self->_xdLoader->retrieveXdInfo();

	XData::StringVectorMap dupDefs;

	try
	{ 
		dupDefs = self->_xdLoader->getDuplicateDefinitions();
	}
	catch (std::runtime_error&)
	{
		IDialogPtr dialog = GlobalDialogManager().createMessageBox(
			_("Duplicated XData definitions"), 
			_("There are no duplicated definitions!"), 
			ui::IDialog::MESSAGE_CONFIRM, GTK_WINDOW(self->getWindow())
		);

		dialog->run();
		return;
	}

	std::string out;

	for (XData::StringVectorMap::iterator it = dupDefs.begin(); it != dupDefs.end(); ++it)
	{
		std::string occ;

		for (std::size_t n = 0; n < it->second.size()-1; n++)
		{
			occ += it->second[n] + ", ";
		}

		occ += it->second[it->second.size() - 1];

		out += (boost::format(_("%s has been defined in:")) % it->first).str();
		out +="\n\t";
		out += occ;
		out += ".\n\n";
	}
	TextViewInfoDialog dialog(_("Duplicated XData definitions"), out, GTK_WINDOW(self->getWindow()));
	dialog.show();
}


//////////////////////////////////////////////////////////////////////////////
// Callback Methods for Events:

gboolean ReadableEditorDialog::onOneSided(GtkWidget* widget, GdkEventKey* event, ReadableEditorDialog* self) 
{
	self->useOneSidedEditing();
	return FALSE;
}

gboolean ReadableEditorDialog::onTwoSided(GtkWidget* widget, GdkEventKey* event, ReadableEditorDialog* self) 
{
	self->useTwoSidedEditing();
	return FALSE;
}

gboolean ReadableEditorDialog::onFocusOut(GtkWidget* widget, GdkEventKey* event, ReadableEditorDialog* self)
{
	if (widget == self->_widgets[WIDGET_XDATA_NAME])
	{
		// Only call checkXDataUniqueness if the method is not running yet.
		if (!self->_runningXDataUniquenessCheck)
		{
			self->checkXDataUniqueness();
		}
		return FALSE;
	}
	else // WIDGET_GUI_ENTRY
	{
		// Only call checkGuiLayout() if the method is not yet running
		if (!self->_runningGuiLayoutCheck)
		{
			self->checkGuiLayout();
		}
		return FALSE;
	}
}

void ReadableEditorDialog::onTextChanged(GtkTextBuffer* textbuffer, ReadableEditorDialog* self)
{
	// Update the preview
	self->updateGuiView();
}

gboolean ReadableEditorDialog::onKeyPress(GtkWidget *widget, GdkEventKey* event, ReadableEditorDialog* self)
{
	bool xdWidget = (widget == self->_widgets[WIDGET_XDATA_NAME]);
	if (xdWidget || widget == self->_widgets[WIDGET_READABLE_NAME])
	{
		switch (event->keyval)
		{
			// Some forbidden keys
			case GDK_space:
			case GDK_Tab:
			case GDK_exclam:
			case GDK_asterisk:
			case GDK_plus:
			case GDK_KP_Multiply:
			case GDK_KP_Subtract:
			case GDK_KP_Add:
			case GDK_KP_Separator:
			case GDK_comma:
			case GDK_period:
			case GDK_colon:
			case GDK_semicolon:
			case GDK_question:
			case GDK_minus: 
				return TRUE;
			// Check Uniqueness of the XData-Name.
			case GDK_Return:
			case GDK_KP_Enter:
				if (xdWidget)
				{
					self->checkXDataUniqueness();
				}
				return FALSE;
			default: 
				return FALSE;
		}
	}

	if (widget == self->_widgets[WIDGET_NUMPAGES])
	{
		if (event->keyval != GDK_Escape)
		{
			return FALSE;
		}

		// Restore the old value.
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(self->_widgets[WIDGET_NUMPAGES]), static_cast<gdouble>(self->_xData->getNumPages()));
		return TRUE;
	}

	if (widget == self->_widgets[WIDGET_GUI_ENTRY])
	{
		if (event->keyval != GDK_Return && event->keyval != GDK_KP_Enter)
		{
			return FALSE;
		}

		self->checkGuiLayout();
		return TRUE;
	}

	return FALSE;
}

} // namespace ui
