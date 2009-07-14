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

#include "generic/callback.h"

namespace entity
{

/**
 * greebo: this is a class encapsulating the "_color" spawnarg
 * of entity, observing it and maintaining the corresponding shader.
 */
class Colour
{
	Callback _colourChanged;
	ShaderPtr _shader;

public:
	Vector3 m_colour;

	Colour(const Callback& colourChanged) : 
		_colourChanged(colourChanged),
		m_colour(1,1,1)
	{
		captureShader();
	}

	void colourChanged(const std::string& value)
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
		_colourChanged();
	}
	typedef MemberCaller1<Colour, const std::string&, &Colour::colourChanged> ColourChangedCaller;

	void writeToEntity(Entity* entity) const
	{
		entity->setKeyValue(
			"_color", 
			(boost::format("(%g %g %g)") % m_colour[0] % m_colour[1] % m_colour[2]).str()
		);
	}

	const ShaderPtr& getShader() const
	{
		return _shader;
	}

private:

	void captureShader()
	{
	  	_shader = GlobalRenderSystem().capture(
			(boost::format("(%g %g %g)") % m_colour[0] % m_colour[1] % m_colour[2]).str()
		);
	}
};

} // namespace entity

#endif
