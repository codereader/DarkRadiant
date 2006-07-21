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

#include "model.h"

#include <boost/algorithm/string/replace.hpp>

// Update the contained model from the provided keyvalues

void Model::modelChanged(std::string value) {
	// Sanitise the keyvalue - must use forward slashes
	boost::algorithm::replace_all(value, "\\", "/");
	
    m_resource.detach(*this);
    m_resource.setName(value.c_str());
    m_resource.attach(*this);
    m_modelChanged();
}