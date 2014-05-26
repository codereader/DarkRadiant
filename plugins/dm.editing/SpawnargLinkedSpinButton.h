#pragma once

#include "iundo.h"
#include "ieclass.h"
#include "ientity.h"
#include "string/convert.h"
#include "util/ScopedBoolLock.h"
#include <boost/format.hpp>
#include <wx/spinctrl.h>

namespace ui
{

/**
 * An enhanced spin button that is updating the named
 * entity property (spawnarg) when toggled.
 */
class SpawnargLinkedSpinButton : 
	public wxSpinCtrlDouble
{
private:
	std::string _label;

	std::string _propertyName;

	Entity* _entity;

	bool _updateLock;
	
public:
	SpawnargLinkedSpinButton(wxWindow* parent,
							 const std::string& label, 
						     const std::string& propertyName, 
							 double min,
							 double max,
						     double increment = 1, 
							 unsigned int digits = 0) :
		wxSpinCtrlDouble(parent, wxID_ANY),
		_label(label),
		_propertyName(propertyName),
		_entity(NULL),
		_updateLock(false)
	{
		SetIncrement(increment);
		SetRange(min, max);
		SetDigits(digits);

		// 6 chars wide
		SetMaxSize(wxSize(GetCharWidth() * 9, -1));

		Connect(wxEVT_SPINCTRLDOUBLE, 
			wxSpinDoubleEventHandler(SpawnargLinkedSpinButton::onSpinButtonChanged), NULL, this);
	}

	const std::string& getLabel() const
	{
		return _label;
	}

	// Sets the edited Entity object
	void setEntity(Entity* entity)
	{
		_entity = entity;

		if (_entity == NULL) 
		{
			SetToolTip("");
			return;
		}

		SetToolTip(_propertyName + ": " + _entity->getEntityClass()->getAttribute(_propertyName).getDescription());

		if (_updateLock) return;

		util::ScopedBoolLock lock(_updateLock);

		SetValue(string::convert<float>(_entity->getKeyValue(_propertyName)));
	}

protected:
	
	void onSpinButtonChanged(wxSpinDoubleEvent& ev)
	{
		ev.Skip();

		// Update the spawnarg if we have a valid entity
		if (!_updateLock && _entity != NULL)
		{
			util::ScopedBoolLock lock(_updateLock);
			UndoableCommand cmd("editAIProperties");

			double floatVal = GetValue();
			std::string newValue = (boost::format("%." + string::to_string(GetDigits()) + "f") % floatVal).str();

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
