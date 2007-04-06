#ifndef RESPONSEEDITOR_H_
#define RESPONSEEDITOR_H_

typedef struct _GtkWidget GtkWidget;

namespace ui {

class ResponseEditor
{
	GtkWidget* _pageVBox;
public:
	/** greebo: Constructor creates all the widgets
	 */
	ResponseEditor();
	
	/** greebo: Operator cast to widget to pack this page into
	 * 			a notebook tab or other parent widget.
	 */
	operator GtkWidget*();

private:
	/** greebo: Creates all the widgets
	 */
	void populatePage();
};

} // namespace ui

#endif /*RESPONSEEDITOR_H_*/
