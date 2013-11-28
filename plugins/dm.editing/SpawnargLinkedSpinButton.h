#pragma once

#include "iundo.h"
#include "ieclass.h"
#include "ientity.h"
#include <gtkmm/box.h>
#include <gtkmm/spinbutton.h>
#include "gtkutil/LeftAlignedLabel.h"
#include "string/convert.h"

namespace ui
{

/**
 * An enhanced spin button that is updating the named
 * entity property (spawnarg) when toggled.
 */
class SpawnargLinkedSpinButton : 
	public Gtk::HBox
{
private:
	std::string _propertyName;

	Entity* _entity;
	Gtk::SpinButton* _spinButton;

	bool _updateLock;

public:
	SpawnargLinkedSpinButton(const std::string& label, 
						     const std::string& propertyName, 
							 double min,
							 double max,
						     double increment = 1, 
							 guint digits = 0) :
		Gtk::HBox(false, 6),
		_propertyName(propertyName),
		_entity(NULL),
		_spinButton(Gtk::manage(new Gtk::SpinButton(increment, digits))),
		_updateLock(false)
	{
		pack_start(*Gtk::manage(new gtkutil::LeftAlignedLabel(label)), false, false, 0);

		_spinButton->set_adjustment(*Gtk::manage(new Gtk::Adjustment(min, min, max, increment)));

		pack_start(*_spinButton, false, false, 0);

		_spinButton->signal_changed().connect(sigc::mem_fun(*this, &SpawnargLinkedSpinButton::onSpinButtonChanged));
	}

	// Sets the edited Entity object
	void setEntity(Entity* entity)
	{
		_entity = entity;

		if (_entity == NULL) 
		{
			set_tooltip_text("");
			return;
		}

		set_tooltip_text(_entity->getEntityClass()->getAttribute(_propertyName).getDescription());

		float value = string::convert<float>(_entity->getKeyValue(_propertyName));

		_updateLock = true;
		_spinButton->set_value(value);
		_updateLock = false;
	}

protected:
	void onSpinButtonChanged()
	{
		// Update the spawnarg if we have a valid entity
		if (!_updateLock && _entity != NULL)
		{
			UndoableCommand cmd("editAIProperties");

			std::string newValue = string::convert<std::string>(_spinButton->get_value());

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
