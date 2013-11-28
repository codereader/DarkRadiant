#pragma once

#include "iundo.h"
#include "ieclass.h"
#include <gtkmm/checkbutton.h>

namespace ui
{

/**
 * An enhanced checkbox that is updating the named
 * entity property (spawnarg) when toggled.
 *
 * The logic toggled = "1" can be optionally inversed such that 
 * an unchecked box reflects a property value of "1".
 */
class SpawnargLinkedCheckbox : 
	public Gtk::CheckButton
{
private:
	bool _inverseLogic;

	std::string _propertyName;

	Entity* _entity;

	bool _updateLock;

public:
	SpawnargLinkedCheckbox(const std::string& label, 
						   const std::string& propertyName, 
						   bool inverseLogic = false) :
		Gtk::CheckButton(label),
		_inverseLogic(inverseLogic),
		_propertyName(propertyName),
		_entity(NULL),
		_updateLock(false)
	{}

	// Sets the edited Entity object
	void setEntity(Entity* entity)
	{
		_entity = entity;

		if (_entity == NULL) 
		{
			set_tooltip_text("");
			return;
		}

		set_tooltip_text(_propertyName + ": " + _entity->getEntityClass()->getAttribute(_propertyName).getDescription());

		bool value = _entity->getKeyValue(_propertyName) == "1";

		_updateLock = true;
		set_active(_inverseLogic ? !value : value);
		_updateLock = false;
	}

protected:
	void on_toggled()
	{
		Gtk::CheckButton::on_toggled();

		// Update the spawnarg if we have a valid entity
		if (!_updateLock && _entity != NULL)
		{
			UndoableCommand cmd("editAIProperties");

			std::string newValue = "";

			if (_inverseLogic)
			{
				newValue = get_active() ? "0" : "1"; // Active => "0"
			}
			else
			{
				newValue = get_active() ? "1" : "0";
			}

			// Check if the new value conincides with an inherited one
			if (_entity->getEntityClass()->getAttribute(_propertyName).getValue() == newValue)
			{
				// in which case the property just gets removed from the entity
				newValue = "";
			}

			_entity->setKeyValue(_propertyName, newValue);
		}
	}
};

} // namespace
