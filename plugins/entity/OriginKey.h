#pragma once

#include "ientity.h"

#include "math/Vector3.h"
#include <boost/function.hpp>
#include "string/convert.h"

const Vector3 ORIGINKEY_IDENTITY = Vector3(0, 0, 0);

class OriginKey :
	public KeyObserver
{
private:
	boost::function<void()> _originChanged;

	Vector3 _origin;

public:
	OriginKey(const boost::function<void()>& originChanged) :
		_originChanged(originChanged),
		_origin(ORIGINKEY_IDENTITY)
	{}

	const Vector3& get() const
	{
		return _origin;
	}

	void set(const Vector3& origin)
	{
		_origin = origin;
	}

	void snap(float snap)
	{
		_origin.snap(snap);
	}

	void onKeyValueChanged(const std::string& value)
	{
		_origin = string::convert<Vector3>(value);

		_originChanged();
	}

	void write(Entity& entity) const
	{
		entity.setKeyValue("origin", string::to_string(_origin));
	}
};
