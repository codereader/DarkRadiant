#pragma once

#include <stdexcept>
#include "MissionInfoTextFile.h"

namespace map
{

class ReadmeTxt;
typedef std::shared_ptr<ReadmeTxt> ReadmeTxtPtr;

/**
* An object representing the readme.txt file as found in the
* mission folder's root directory. It contains detailed info
* and/or instructions for the player installing the mission.
*/
class ReadmeTxt :
	public MissionInfoTextFile
{
private:
	std::string _contents;

public:
	static const char* NAME()
	{
		return "readme.txt";
	}

	std::string getFilename() override;

	const std::string& getContents();
	void setContents(const std::string& contents);

	// Named constructor parsing the given string into a DarkmodTxt instance
	// A parse exception will be thrown if the file is not compliant
	static ReadmeTxtPtr CreateFromString(const std::string& contents);

	// Named constructor parsing the given stream into a DarkmodTxt instance
	// A parse exception will be thrown if the file is not compliant
	static ReadmeTxtPtr CreateFromStream(std::istream& stream);

	// A parse exception will be thrown if the file is not compliant
	static ReadmeTxtPtr LoadForCurrentMod();

	// Retrieves the text representation of this instance, as it will be written to the readme.txt file
	std::string toString() override;
};

}
