#ifndef EFFECTEDITOR_H_
#define EFFECTEDITOR_H_

#include "gtkutil/DialogWindow.h"

#include "StimResponse.h"
#include "ResponseEffectTypes.h"
#include "EffectArgumentItem.h"
#include <boost/shared_ptr.hpp>

// Forward Declarations
typedef struct _GtkWindow GtkWindow;
typedef struct _GtkWidget GtkWidget;
typedef struct _GtkListStore GtkListStore;

class EffectEditor :
	public gtkutil::DialogWindow
{
	// The overall vbox
	GtkWidget* _dialogVBox;
	
	// The vbox containing the argument widgets
	GtkWidget* _argVBox;
	
	// The list containing the possible effect types
	ResponseEffectTypeMap _effectTypes;
	
	GtkWidget* _effectTypeCombo;
	GtkListStore* _effectStore;
	
	// The list of argument items
	typedef boost::shared_ptr<EffectArgumentItem> ArgumentItemPtr;
	typedef std::vector<ArgumentItemPtr> ArgumentItemList;
	ArgumentItemList _argumentItems;
	
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

private:
	/** greebo: Parses the response effect for necessary arguments
	 * 			and creates the according widgets.
	 */
	void createArgumentWidgets(ResponseEffect& effect);
};

#endif /*EFFECTEDITOR_H_*/
