#ifndef PROPERTYEDITOR_H_
#define PROPERTYEDITOR_H_

#include "ientityinspector.h"
#include "gtkutil/ifc/Widget.h"

#include <string>
#include <boost/shared_ptr.hpp>

/* FORWARD DECLS */
class Entity;

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
 */
class PropertyEditor : 
	public IPropertyEditor
{
protected:

	// The entity being focused (NULL if none there)
	Entity* _entity;

	// Protected blank constructor
	PropertyEditor();

	// Protected constructor
	PropertyEditor(Entity* entity);

protected:

	/**
	 * greebo: Central method to assign values to the entit(ies) in question.
	 * This takes care of calling setKeyValue() on the selected entities
	 * as well as managing the UndoSystem.
	 */
	virtual void setKeyValue(const std::string& key, const std::string& value);
};

}

#endif /*PROPERTYEDITOR_H_*/
