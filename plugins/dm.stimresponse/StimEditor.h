#ifndef STIMEDITOR_H_
#define STIMEDITOR_H_

typedef struct _GtkWidget GtkWidget;

namespace ui {

class StimEditor
{
	GtkWidget* _pageVBox;
public:
	/** greebo: Constructor creates all the widgets
	 */
	StimEditor();
	
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

#endif /*STIMEDITOR_H_*/
