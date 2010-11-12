/*
Copyright (C) 2001-2006, William Joseph.
All Rights Reserved.

This file is part of GtkRadiant.

GtkRadiant is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

GtkRadiant is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GtkRadiant; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#if !defined(INCLUDED_COLOUR_H)
#define INCLUDED_COLOUR_H

#include "ientity.h"
#include "irender.h"

namespace entity
{

/**
 * greebo: this is a class encapsulating the "_color" spawnarg
 * of entity, observing it and maintaining the corresponding shader.
 */
class Colour :
	public KeyObserver
{
private:
	ShaderPtr _wireShader;

public:
	Vector3 m_colour;

	Colour() :
		m_colour(1,1,1)
	{
		captureShader();
	}

	// Called when "_color" keyvalue changes
	void onKeyValueChanged(const std::string& value)
	{
		// Initialise the colour with white, in case the string parse fails
		m_colour[0] = m_colour[1] = m_colour[2] = 1;

		// Use a stringstream to parse the string
		std::stringstream strm(value);

		strm << std::skipws;
		strm >> m_colour.x();
		strm >> m_colour.y();
		strm >> m_colour.z();

		captureShader();
	}

	const ShaderPtr& getWireShader() const
	{
		return _wireShader;
	}

private:

	void captureShader()
	{
		std::string wireCol = (boost::format("<%g %g %g>") % m_colour[0] % m_colour[1] % m_colour[2]).str();
		_wireShader = GlobalRenderSystem().capture(wireCol);
	}
};

} // namespace entity

#endif
