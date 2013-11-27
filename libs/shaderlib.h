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
#pragma once

#include "string/string.h"
#include "character.h"
#include "ishaders.h"
#include "igame.h"

inline bool shader_equal(const std::string& shader, const std::string& other)
{
  return string_equal_nocase(shader.c_str(), other.c_str());
}

inline bool shader_equal_n(const char* shader, const char* other, std::size_t n)
{
  return string_equal_nocase_n(shader, other, n);
}

inline bool shader_valid(const char* shader)
{
  return string_is_ascii(shader)
    && strchr(shader, ' ') == 0
    && strchr(shader, '\n') == 0
    && strchr(shader, '\r') == 0
    && strchr(shader, '\t') == 0
    && strchr(shader, '\v') == 0
    && strchr(shader, '\\') == 0;
}

inline const char* GlobalTexturePrefix_get()
{
  return GlobalMaterialManager().getTexturePrefix();
}

inline const char* shader_get_textureName(const char* name)
{
  return name + std::strlen(GlobalTexturePrefix_get());
}

inline std::string texdef_load_default_from_current_game()
{
	xml::NodeList list = GlobalGameManager().currentGame()->getLocalXPath("/defaults/defaultTexture");

	if (list.empty()) return "";

	return list[0].getAttributeValue("value");
}

inline const std::string& texdef_name_default()
{
	static std::string _default = texdef_load_default_from_current_game();
	return _default;
}
