#ifndef PROPERTYEDITOR_H_
#define PROPERTYEDITOR_H_

#include "ientityinspector.h"

#include <string>
#include <boost/shared_ptr.hpp>

/* FORWARD DECLS */
class Entity;

namespace Gtk
{
	class Widget;
}

namespace ui
{

/**
 * PropertyEditor shared pointer type.
 */
class PropertyEditor;
typedef boost::shared_ptr<PropertyEditor> PropertyEditorPtr;

/**
 * Base class for built-in PropertyEditor widgets. Derived classes
 * need to implement the createNew method for virtual construction.
 *
 * Derived classes should call setMainWdiget() to pass a smart pointer
 * to this base class. The Glib::RefPtr<Gtk::Widget> reference is then
 * held by the base class and destroyed along with the base class.
 */
class PropertyEditor :
	public IPropertyEditor
{
private:
	// The main widget, should be set by the subclass using setMainWidget()
	Gtk::Widget* _mainWidget;

protected:
	// The entity being focused (NULL if none there)
	Entity* _entity;

	// Protected blank constructor
	PropertyEditor();

	// Protected constructor
	PropertyEditor(Entity* entity);

protected:
	/**
	 * Subclasses should call this method after the editor widgets
	 * have been created. This base class will take responsibility
	 * of destroying this widget along with this class.
	 */
	void setMainWidget(Gtk::Widget* widget);

	/**
	 * greebo: Central method to assign values to the entit(ies) in question.
	 * This takes care of calling setKeyValue() on the selected entities
	 * as well as managing the UndoSystem.
	 */
	virtual void setKeyValue(const std::string& key, const std::string& value);

	/**
	 * greebo: Convenience method to retrieve a keyvalue from the edited entity.
	 */
	virtual std::string getKeyValue(const std::string& key);

public:
	virtual ~PropertyEditor();

	// IPropertyEditor implementation
	Gtk::Widget& getWidget();
};

}

#endif /*PROPERTYEDITOR_H_*/
