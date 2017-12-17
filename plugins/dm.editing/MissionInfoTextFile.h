#pragma once

#include <string>
#include <memory>
#include <stdexcept>

namespace map
{

/**
* Base declaration of a text file containing mission
* information such as readme.txt and darkmod.txt.
*/
class MissionInfoTextFile
{
public:
	class ParseException :
		public std::runtime_error
	{
	public:
		ParseException(const std::string& msg) :
			runtime_error(msg.c_str())
		{}
	};

	// The filename of this file, e.g. "darkmod.txt"
	virtual std::string getFilename() = 0;

	// The full path to the description file
	virtual std::string getFullOutputPath();

	// Saves the contents of this instance to the path applicable to the current mod
	// Throws a std::runtime_error if the file couldn't be written
	virtual void saveToCurrentMod();

	// Retrieves the text representation of this instance, as it will be written to the darkmod.txt file
	virtual std::string toString() = 0;

protected:
	// Returns the mod output path (normalised with trailing slash, without the filename part)
	static std::string GetOutputPathForCurrentMod();
};

}
