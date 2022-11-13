#pragma once

#include "ui/ientityinspector.h"
#include <wx/event.h>

#include <string>
#include <memory>

/* FORWARD DECLS */
class Entity;
class wxBitmap;

namespace ui
{

/**
 * PropertyEditor shared pointer type.
 */
class PropertyEditor;
typedef std::shared_ptr<PropertyEditor> PropertyEditorPtr;

/**
 * Base class for built-in PropertyEditor widgets.
 *
 * Derived classes should call setMainWdiget() to pass a smart pointer
 * to this base class. The reference is then held by the base and 
 * destroyed along with it.
 */
class PropertyEditor :
	public wxEvtHandler,
	public IPropertyEditor
{
private:
	// The main widget, should be set by the subclass using setMainWidget()
	wxPanel* _mainWidget;

	std::function<void()> _oneButtonPanelCallback;

protected:
	// The selected entities we're working with
    IEntitySelection& _entities;

    sigc::signal<void(const std::string&, const std::string&)> _sigKeyValueApplied;

	// Protected constructor
	PropertyEditor(IEntitySelection& entities);

protected:
	/**
	 * Subclasses should call this method after the editor widgets
	 * have been created. This base class will take responsibility
	 * of destroying this widget along with this class.
	 */
	void setMainWidget(wxPanel* widget);

	/**
	 * greebo: Central method to assign values to the entit(ies) in question.
	 * This takes care of calling setKeyValue() on the selected entities
	 * as well as managing the UndoSystem.
	 */
	virtual void setKeyValueOnSelection(const std::string& key, const std::string& value);

	/**
	 * greebo: Convenience method to retrieve a keyvalue from the edited entity.
	 * Note: This also considers inherited key values.
	 */
	virtual std::string getKeyValueFromSelection(const std::string& key);

	/**
	 * greebo: Since many property editors consists of a single browse button, 
	 * the base class provides this convenience method.
	 */
	void constructBrowseButtonPanel(wxWindow* parent, const std::string& label,
								 const wxBitmap& bitmap);

	// When using constructBrowseButtonPanel() subclasses should override this method to catch the event
	virtual void onBrowseButtonClick() {}

private:
	// wxWidgets callback when using the browse-button-panel design
	void _onBrowseButtonClick(wxCommandEvent& ev);

public:
	virtual ~PropertyEditor();

	// IPropertyEditor implementation
	wxPanel* getWidget() override;

    void updateFromEntities() override
    {}

    sigc::signal<void(const std::string&, const std::string&)>& signal_keyValueApplied() override;
};

} // namespace
