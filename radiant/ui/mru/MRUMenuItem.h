#ifndef MRUMENUITEM_H_
#define MRUMENUITEM_H_

#include <string>

// Forward declaration
typedef struct _GtkWidget GtkWidget; 

namespace ui {

// Forward declaration
class MRU;

/* greebo: An MRUMenuItem holds the information of a single menu entry,
 * this consists mainly of the GtkWidget* (in the menu).
 * 
 * Use the GtkWidget* operator to retrieve the actual widget. 
 */
class MRUMenuItem 
{
	// The label of this widget
	std::string _label;
	
	// The reference to the main class for loading maps and stuff
	MRU& _mru;
	
	// The number of this MRU item to be displayed
	unsigned int _index;
	
	// The internally stored name and reference to the GtkWidget
	GtkWidget* _widget;
	
public:
	// Constructor
	MRUMenuItem(const std::string& label, ui::MRU& _mru, unsigned int index);
	
	// Copy Constructor
	MRUMenuItem(const ui::MRUMenuItem& other);
	
	void setWidget(GtkWidget* widget);
	operator GtkWidget* ();
	
	// Triggers loading the map represented by this widget 
	void activate();
	
	// Shows/hides the widget
	void show();
	void hide();
	
	// Sets/Retrieves the label
	void setLabel(const std::string& label);
	std::string getLabel() const;
	
	int getIndex() const;
	
}; // class MRUMenuItem
	
} // namespace ui

#endif /*MRUMENUITEM_H_*/
