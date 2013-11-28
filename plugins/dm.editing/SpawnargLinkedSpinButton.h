#pragma once

#include "iundo.h"
#include "ieclass.h"
#include "ientity.h"
#include <gtkmm/box.h>
#include <gtkmm/spinbutton.h>
#include "gtkutil/LeftAlignedLabel.h"
#include "string/convert.h"
#include "util/ScopedBoolLock.h"
#include <boost/format.hpp>

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
		pack_start(*Gtk::manage(new gtkutil::LeftAlignedLabel(label)), true, true, 0);

		_spinButton->set_adjustment(*Gtk::manage(new Gtk::Adjustment(min, min, max, increment)));

		pack_start(*_spinButton, false, false, 0);

		_spinButton->signal_value_changed().connect(sigc::mem_fun(*this, &SpawnargLinkedSpinButton::onSpinButtonChanged));
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

		set_tooltip_text(_propertyName + ": " + _entity->getEntityClass()->getAttribute(_propertyName).getDescription());

		if (_updateLock) return;

		util::ScopedBoolLock lock(_updateLock);

		_spinButton->set_value(string::convert<float>(_entity->getKeyValue(_propertyName)));
	}

protected:
	
	void onSpinButtonChanged()
	{
		// Update the spawnarg if we have a valid entity
		if (!_updateLock && _entity != NULL)
		{
			util::ScopedBoolLock lock(_updateLock);
			UndoableCommand cmd("editAIProperties");

			double floatVal = _spinButton->get_value();
			std::string newValue = (boost::format("%." + string::to_string(_spinButton->get_digits()) + "f") % floatVal).str();

			// Check if the new value conincides with an inherited one
			const EntityClassAttribute& attr = _entity->getEntityClass()->getAttribute(_propertyName);

			if (!attr.getValue().empty() && string::to_float(attr.getValue()) == floatVal)
			{
				// in which case the property just gets removed from the entity
				newValue = "";
			}

			_entity->setKeyValue(_propertyName, newValue);
		}
	}
};

} // namespace
