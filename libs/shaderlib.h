#pragma once

#include "string/string.h"
#include "character.h"
#include "ishaders.h"
#include "ibrush.h"
#include "iundo.h"
#include "ipatch.h"
#include "iselection.h"
#include "scene/Traverse.h"
#include "gamelib.h"

inline bool shader_equal(const std::string& shader, const std::string& other)
{
  return string_equal_nocase(shader.c_str(), other.c_str());
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

inline const std::string& texdef_name_default()
{
	static std::string _default = game::current::getValue<std::string>("/defaults/defaultTexture", "_default");
	return _default;
}

namespace scene
{

/**
 * greebo: This replaces the shader of the visited face/patch with <replace>
 * if the face is textured with <find> and increases the given <counter>.
 */
class ShaderReplacer
{
private:
	const std::string _find;
	const std::string _replace;
	int _counter;

public:
	ShaderReplacer(const std::string& find, const std::string& replace) :
		_find(find),
		_replace(replace),
		_counter(0)
	{}

	int getReplacedCount() const
	{
		return _counter;
	}

	void operator()(IFace& face)
	{
		if (face.getShader() == _find)
		{
			face.setShader(_replace);
			_counter++;
		}
	}

	void operator()(IPatch& patch)
	{
		if (patch.getShader() == _find)
		{
			patch.setShader(_replace);
			_counter++;
		}
	}

	void operator()(const IPatchNodePtr& node)
	{
		(*this)(node->getPatch());
	}
};

inline int findAndReplaceShader(const std::string& find, const std::string& replace, bool selectedOnly)
{
	std::string command("textureFindReplace");
	command += "-find " + find + " -replace " + replace;
	UndoableCommand undo(command);

	// Construct a visitor class
	ShaderReplacer replacer(find, replace);

	if (selectedOnly)
	{
		if (GlobalSelectionSystem().getSelectionMode() != selection::SelectionMode::Component)
		{
			// Find & replace all the brush and patch shaders
			GlobalSelectionSystem().foreachFace(std::ref(replacer));
			GlobalSelectionSystem().foreachPatch(std::ref(replacer));
		}
	}
	else
	{
		scene::foreachVisibleFace(std::ref(replacer));
		scene::foreachVisiblePatch(std::ref(replacer));
	}

	return replacer.getReplacedCount();
}

}
