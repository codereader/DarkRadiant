#pragma once

#include "ibrush.h"
#include "math/Vector3.h"

namespace brush
{

class BrushSettings :
	public IBrushSettings
{
private:
	Vector3 _vertexColour; 
    Vector3 _selectedVertexColour;

	sigc::signal<void> _signalSettingsChanged;
public:
	BrushSettings() :
		_vertexColour(0, 1, 0),
        _selectedVertexColour(0, 0, 1)
	{}

	const Vector3& getVertexColour() const override
	{
		return _vertexColour;
	}

	void setVertexColour(const Vector3& colour) override
	{
		_vertexColour = colour;

		_signalSettingsChanged.emit();
	}

    const Vector3& getSelectedVertexColour() const override
    {
        return _selectedVertexColour;
    }

    void setSelectedVertexColour(const Vector3& colour) override
    {
        _selectedVertexColour = colour;

        _signalSettingsChanged.emit();
    }

	sigc::signal<void>& signal_settingsChanged() override
	{
		return _signalSettingsChanged;
	}
};

}
