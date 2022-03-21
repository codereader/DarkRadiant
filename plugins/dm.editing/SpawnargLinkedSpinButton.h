#pragma once

#include "iundo.h"
#include "ieclass.h"
#include "ientity.h"
#include "string/convert.h"
#include "util/ScopedBoolLock.h"
#include <fmt/format.h>
#include <wx/spinctrl.h>
#include <wx/panel.h>
#include <wx/sizer.h>

namespace ui
{

/**
 * An enhanced spin button that is updating the named
 * entity property (spawnarg) when toggled.
 *
 * Due to some weird bug that prevented the wxSpinCtrlDouble
 * to be rendered with a wxScrolledWindow as immediate parent,
 * I had to work around it by putting the wxSpinCtrlDouble into
 * a parent wxPanel first.
 */
class SpawnargLinkedSpinButton : 
	public wxPanel
{
private:
	wxSpinCtrlDouble* _spinCtrl;

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
		wxPanel(parent, wxID_ANY),
		_spinCtrl(new wxSpinCtrlDouble(this, wxID_ANY)),
		_label(label),
		_propertyName(propertyName),
		_entity(NULL),
		_updateLock(false)
	{
		this->SetSizer(new wxBoxSizer(wxHORIZONTAL));
		this->GetSizer()->Add(_spinCtrl, 1, wxEXPAND);

		_spinCtrl->SetIncrement(increment);
		_spinCtrl->SetRange(min, max);
		_spinCtrl->SetDigits(digits);

		// 6 chars wide
		_spinCtrl->SetMaxSize(wxSize(GetCharWidth() * 9, -1));

		_spinCtrl->Connect(wxEVT_SPINCTRLDOUBLE, 
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

        std::string desc = _propertyName + ": "
                         + _entity->getEntityClass()->getAttributeDescription(_propertyName);
        _spinCtrl->SetToolTip(desc);

		if (_updateLock) return;

		util::ScopedBoolLock lock(_updateLock);

		_spinCtrl->SetValue(string::convert<float>(_entity->getKeyValue(_propertyName)));
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

			double floatVal = _spinCtrl->GetValue();
			std::string newValue = fmt::format("{0:." + string::to_string(_spinCtrl->GetDigits()) + "f}", floatVal);

			// Check if the new value conincides with an inherited one
			const std::string attr = _entity->getEntityClass()->getAttributeValue(_propertyName);
			if (!attr.empty() && string::to_float(attr) == floatVal)
			{
				// in which case the property just gets removed from the entity
				newValue = "";
			}

			_entity->setKeyValue(_propertyName, newValue);
		}
	}
};

} // namespace
