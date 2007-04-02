#ifndef EFFECTEDITOR_H_
#define EFFECTEDITOR_H_

#include "gtkutil/DialogWindow.h"

#include "StimResponse.h"

// Forward Declarations
typedef struct _GtkWindow GtkWindow;
typedef struct _GtkWidget GtkWidget;

class EffectEditor :
	public gtkutil::DialogWindow
{
	// The overall vbox
	GtkWidget* _dialogVBox;
	
public:
	EffectEditor(GtkWindow* parent);
	
	/** greebo: Creates the widgets 
	 */
	void populateWindow();
	
	/** greebo: Shows the window (it stays hidden till this
	 * 			method is called).
	 * 
	 * @response: The Stim/Response object the effect is associated with
	 * 			  (this should be a response, although stims work as well).
	 * 
	 * @effectIndex: The response effect index within the given Response. 
	 */
	void editEffect(StimResponse& response, const unsigned int effectIndex);
};

#endif /*EFFECTEDITOR_H_*/
