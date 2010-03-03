#ifndef _READABLE_EDITOR_DIALOG_H_
#define _READABLE_EDITOR_DIALOG_H_

#include "icommandsystem.h"

#include "gtkutil/window/BlockingTransientWindow.h"
#include "gui/GuiView.h"
#include <map>
#include <gtk/gtk.h>
#include "XDataLoader.h"
#include "string/string.h"

class Entity;

namespace ui
{


class ReadableEditorDialog :
	public gtkutil::BlockingTransientWindow
{

public:
	enum Result
	{
		RESULT_OK,
		RESULT_CANCEL,
		NUM_RESULTS,
	};

private:
	gui::GuiViewPtr _guiView;

	// A container for storing enumerated widgets
	std::map<int, GtkWidget*> _widgets;

	Result _result;

	// The entity we're working with
	Entity* _entity;

	// The XData-definition of the entity
	XData::XDataPtr _xData;

	// The filename of the XData definition
	std::string _xdFilename;

	// The xData loader
	XData::XDataLoaderPtr _xdLoader;

	// The index of the current page.
	std::size_t _currentPageIndex;

	// Has the XData name been specified?
	bool _xdNameSpecified;

public:
	// Pass the working entity to the constructor
	ReadableEditorDialog(Entity* entity);

	static void RunDialog(const cmd::ArgumentList& args);

protected:
	virtual void _postShow();

private:
	// Save all settings on the entity and exports xdata.
	void save();

	// Retrieves information from the entity and imports xdata. If the user cancels, the window is destroyed in _postshow
	bool initControlsFromEntity();

	// _show_ TwoSided editing-interface.
	void toggleTwoSidedEditing(bool show);

	// Stores the contents of the current page inside the xData-object.
	void storeCurrentPage();

	// Stores the contents of all controls in the xData-object.
	void storeXData();

	// Updates the page related inputs and the preview renderer. Also adds default guis.
	// Warning: Contents are overwritten. storeCurrentPage() should be called beforehand.
	void showPage(std::size_t pageIndex);		//updates _guiView!!!!!!!!!!!!!!!!

	// Populates the controls with the information in the xdata object. Adds a default snd_page_turn, if not defined.
	void populateControlsFromXData();

	// Checks whether the chosen XData name already exists. If it does a popup will ask the user whether it should be imported.
	void checkXDataUniqueness();

	// Deleting and inserting of pages.
	void insertPage();
	void deletePage();

	// Shifting of twosided readables
	//void shiftRight();
	//void shiftLeft();

	// Inserts a page on the right or left side of the current page.
	void insertSide(bool rightSide);

	// Deletes the left or right side of the current page.
	void deleteSide(bool rightSide);

	// Ui Creation:
	GtkWidget* createEditPane();
	GtkWidget* createGeneralPropertiesInterface();
	GtkWidget* createPageRelatedInterface();
	GtkWidget* createButtonPanel();
	void createMenus();

	// Callback methods for Signals:
	static void onCancel(GtkWidget* widget, ReadableEditorDialog* self);
	static void onSave(GtkWidget* widget, ReadableEditorDialog* self);
	static void onBrowseXd(GtkWidget* widget, ReadableEditorDialog* self);
	static void onFirstPage(GtkWidget* widget, ReadableEditorDialog* self);
	static void onPrevPage(GtkWidget* widget, ReadableEditorDialog* self);
	static void onNextPage(GtkWidget* widget, ReadableEditorDialog* self);
	static void onLastPage(GtkWidget* widget, ReadableEditorDialog* self);
	static void onBrowseGui(GtkWidget* widget, ReadableEditorDialog* self);
	static void onInsert(GtkWidget* widget, ReadableEditorDialog* self);
	static void onDelete(GtkWidget* widget, ReadableEditorDialog* self);
	static void onValueChanged(GtkWidget* widget, ReadableEditorDialog* self);
	static void onMenuAppend(GtkWidget* widget, ReadableEditorDialog* self);
	static void onMenuPrepend(GtkWidget* widget, ReadableEditorDialog* self);
	static void onToolsClicked(GtkWidget* widget, ReadableEditorDialog* self);
	static void onImpSum(GtkWidget* widget, ReadableEditorDialog* self);
	static void onDupDef(GtkWidget* widget, ReadableEditorDialog* self);
	static void onInsertWhole(GtkWidget* widget, ReadableEditorDialog* self);
	static void onInsertLeft(GtkWidget* widget, ReadableEditorDialog* self);
	static void onInsertRight(GtkWidget* widget, ReadableEditorDialog* self);
	static void onDeleteWhole(GtkWidget* widget, ReadableEditorDialog* self);
	static void onDeleteLeft(GtkWidget* widget, ReadableEditorDialog* self);
	static void onDeleteRight(GtkWidget* widget, ReadableEditorDialog* self);

	
	// Callback methods for Events:
	static gboolean onOneSided(GtkWidget* widget, GdkEventKey* event, ReadableEditorDialog* self);
	static gboolean onTwoSided(GtkWidget* widget, GdkEventKey* event, ReadableEditorDialog* self);
	static gboolean onFocusOut(GtkWidget* widget, GdkEventKey* event, ReadableEditorDialog* self);
	static gboolean onKeyPress(GtkWidget *widget, GdkEventKey *event, ReadableEditorDialog* self);	//updates _guiView!!!!!!!!!!!!!!!!

	// Helper Methods:

	// Read Text from a given TextView Widget identified by its widget enumerator.
	inline std::string readTextBuffer(int wEnum)
	{
		GtkTextBuffer* buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(_widgets[wEnum]));
		GtkTextIter start, end;
		gtk_text_buffer_get_bounds(buffer,&start,&end);
		return gtk_text_buffer_get_text(buffer,&start,&end, TRUE);
	}

	// Sets the text of a TextView identified by its widget enumerator and scrolls it to the end.
	inline void setTextViewAndScroll(int wEnum, std::string text)
	{
		GtkTextBuffer* bfr = GTK_TEXT_BUFFER(gtk_text_view_get_buffer(GTK_TEXT_VIEW(_widgets[wEnum])));
		gtk_text_buffer_set_text( bfr, text.c_str(), text.size() );
		GtkTextIter ending;
		gtk_text_buffer_get_end_iter( bfr, &ending );
		GtkTextMark* pMarkEnd = gtk_text_buffer_create_mark(bfr, "", &ending,FALSE);
		gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(_widgets[wEnum]), pMarkEnd, 0, FALSE, 0, 0);	//for some strange reason scroll to iter does not work...
	}
};

} // namespace ui

#endif /* _READABLE_EDITOR_DIALOG_H_ */
