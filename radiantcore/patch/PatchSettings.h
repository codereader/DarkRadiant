#pragma once

#include "ipatch.h"
#include "math/Vector3.h"

namespace patch
{

class PatchSettings :
	public IPatchSettings
{
private:
	Vector3 _vertexColourCorner;
	Vector3 _vertexColourInside;

	sigc::signal<void> _signalSettingsChanged;

	std::vector<Vector3> _vertexColours;
public:
	PatchSettings() :
		_vertexColours(static_cast<std::size_t>(PatchEditVertexType::NumberOfVertexTypes))
	{
		_vertexColours[static_cast<std::size_t>(PatchEditVertexType::Corners)] = Vector3(1, 0, 1);
		_vertexColours[static_cast<std::size_t>(PatchEditVertexType::Inside)] = Vector3(0, 1, 0);
	}

	const Vector3& getVertexColour(PatchEditVertexType type) const override
	{
		assert(type != PatchEditVertexType::NumberOfVertexTypes);
		return _vertexColours[static_cast<std::size_t>(type)];
	}

	void setVertexColour(PatchEditVertexType type, const Vector3& value) override
	{
		assert(type != PatchEditVertexType::NumberOfVertexTypes);
		_vertexColours[static_cast<std::size_t>(type)] = value;

		_signalSettingsChanged.emit();
	}

	sigc::signal<void>& signal_settingsChanged() override
	{
		return _signalSettingsChanged;
	}
};

}
