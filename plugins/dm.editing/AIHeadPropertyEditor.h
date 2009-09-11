#ifndef AI_HEADPROPERTYEDITOR_H_ 
#define AI_HEADPROPERTYEDITOR_H_

#include "ientityinspector.h"
#include <set>

namespace ui
{

class AIHeadPropertyEditor :
	public IPropertyEditor
{
public:
	typedef std::set<std::string> HeadList;

private:
	// The top-level widget
	GtkWidget* _widget;

	static HeadList _availableHeads;

protected:
	// gtkutil::Widget impl.
	virtual GtkWidget* _getWidget() const
	{
		return _widget;
	}

public:
	// Default constructor
	AIHeadPropertyEditor();

	AIHeadPropertyEditor(Entity* entity, 
		const std::string& key, const std::string& options);

	IPropertyEditorPtr createNew(Entity* entity, 
								const std::string& key,
								const std::string& options);

private:
	// Searches all entity classes for available heads
	static void FindAvailableHeads();

	static void onChooseButton(GtkWidget* button, AIHeadPropertyEditor* self);
};

} // namespace ui

#endif /* AI_HEADPROPERTYEDITOR_H_ */
