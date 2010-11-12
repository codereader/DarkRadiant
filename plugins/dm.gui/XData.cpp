#include "XData.h"

#include "i18n.h"
#include <boost/lexical_cast.hpp>
#include <boost/filesystem/convenience.hpp>

namespace XData
{

//XData implementations:
//->export:
FileStatus XData::xport( const std::string& filename, ExporterCommand cmd )
{
	boost::filesystem::path Path(filename);

	boost::filesystem::path parent = Path.parent_path();

	// Ensure the parent path exists
	boost::filesystem::create_directories(parent);

	if (boost::filesystem::exists(Path))
	{
		switch (cmd)
		{
		case Merge:
			{
			//Check if definition already exists and return DefinitionExists. If it does not, append the definition to the file.
				boost::filesystem::fstream file(Path, std::ios_base::in | std::ios_base::out | std::ios_base::app);
				if (!file.is_open())
					return OpenFailed;
				std::stringstream ss;
				ss << file.rdbuf();
				std::string String = ss.str();
				std::size_t DefPos = String.find(_name);
				while (DefPos != std::string::npos)	//A name of a XData could be contained in another XData's name. Check that...
				{
					char before = String[DefPos > 0 ? DefPos-1 : 0];
					char after = String[DefPos+_name.length() < String.size() ? DefPos+_name.length() : 0];
					if ((DefPos == 0 || before == ' ' || before == '\t' || before == '\n') && (DefPos+_name.length() == String.size() || after == ' ' || after == '\t' || after == '\n'))	//other delimiters necessary?
					{
						_definitionStart = 0;
						//Definition found. But we want to have it clean, so let's see how many spaces are before the definition begin.
						while (DefPos > 0)
						{
							if (String[--DefPos] != ' ' && String[DefPos] != '\t' && String[DefPos] != '\n')
							{
								_definitionStart = DefPos+1;
								break;
							}
						}

						file.close();
						return DefinitionExists;
					}
					DefPos = String.find(_name,DefPos+_name.length());
				}
				file << "\n\n\n" << generateXDataDef();
				file.close();
				return AllOk;
			}
		case MergeOverwriteExisting:
			{
			//Find the old definition in the target file and delete it. Append the new definition.
			//_definitionStart has been set in the first iteration of this method.
				boost::filesystem::fstream file(Path, std::ios_base::in);
				if (!file.is_open())
					return OpenFailed;
				std::stringstream ss;
				ss << file.rdbuf();
				std::string OutString = ss.str();
				file.close();
				int DefLength = getDefLength(OutString.substr(_definitionStart));
				if (DefLength == 0)		//If the definitionlength can't be obtained, the merge fails.
					return MergeFailed;
				OutString.erase(_definitionStart, DefLength);
				std::string insString = std::string( (_definitionStart != 0) ? "\n\n\n" : "" )		//No new lines in the first line
					+ generateXDataDef()
					+ std::string( (_definitionStart == OutString.size()) ? "" : "\n\n\n" );		//No new lines at the end of file
				OutString.insert(_definitionStart, insString);
				file.open(Path, std::ios_base::out | std::ios_base::trunc);
				if (!file.is_open())
					return OpenFailed;
				file << OutString;
				file.close();
				return AllOk;
			}
		case Overwrite:
			{
			//Warn if the target file contains multiple definitions: return MultipleDefinitions.
			//Warn if the definition in the target file does not match the current definition: return DefinitionMisMatch
			//else overwrite existing file.
				std::string DefName;
				boost::filesystem::ifstream file(Path, std::ios_base::in);
				if (!file.is_open())
					return OpenFailed;
				try	{ DefName = getDefinitionNameFromXD(file); }
				catch (...)
				{
					std::cerr << "[XData::xport] Syntax error in file " << filename << ". Overwriting the file..." << std::endl;
					break;
				}
				if (DefName == _name)	//Definition will be overwritten...
					break;
				else if (DefName == "")	//MultipleDefinitions exist
					return MultipleDefinitions;
				return DefinitionMismatch;
			}
		case OverwriteMultDef:
			//Replace the file no matter what.
			break;
		default: return FileExists;
		}
	}

	//Write the definition into the file.
	boost::filesystem::ofstream file(Path, std::ios_base::out | std::ios_base::trunc);
	if (!file.is_open())
		return OpenFailed;
	file << generateXDataDef();
	file.close();

	return AllOk;
}


const std::string XData::generateXDataDef() const
{
	std::stringstream xDataDef;
	xDataDef << _name << "\n" << "{" << "\n" << "\tprecache" << "\n" << "\t\"num_pages\"\t: \"" << _numPages << "\"\n";

	std::stringstream ss;
	std::string TempString;

	xDataDef << getContentDef();

	for (std::size_t n=0; n<_numPages; n++)
	{
		xDataDef << "\t\"gui_page" << n+1 << "\"\t: \"" << _guiPage[n] << "\"\n";
	}
	xDataDef << "\t\"snd_page_turn\"\t: \"" << _sndPageTurn << "\"\n}";//*/

	return xDataDef.str();
}

const std::size_t XData::getDefLength(const std::string& def) const
{
	std::size_t charIndex = 0;
	while (def[charIndex] != '\0')
	{
		if (def[++charIndex] == '{')
		{
			int BracketCount = 1;
			while (def[++charIndex] != '\0' && BracketCount > 0)
			{
				// Move out of brackets
				if (def[charIndex] == '{')
					++BracketCount;
				else if (def[charIndex] == '}')
					--BracketCount;
			}
			if (BracketCount > 0)
				break;
			while (def[charIndex] != '\0')
			{
				// The definitionlength also expands over trailing spaces
				if ( (def[charIndex] != ' ') && (def[charIndex] != '\t') && (def[charIndex] != '\n') )
					break;
				++charIndex;
			}
			return charIndex;
		}
	}
	return 0;	//no appropriate bracketstructure was found.
}

const std::string XData::getDefinitionNameFromXD(boost::filesystem::ifstream& file) const
{
	std::string ReturnString;
	parser::BasicDefTokeniser<std::istream> tok(file);
	bool FirstDef = true;
	while (tok.hasMoreTokens())
	{
		if (FirstDef)
			ReturnString = tok.nextToken();
		else
			return "";
		FirstDef = false;
		tok.assertNextToken("{");
		jumpOutOfBrackets(tok, 1);
	}
	return ReturnString;
}

const std::string XData::generateTextDef(const std::string& rawString) const
{
	std::stringstream ss;
	std::stringstream xDataDef;
	std::string TempString;
	xDataDef << "\t{\n";
	if (rawString != "")
	{
		ss << rawString;
		while ( std::getline(ss, TempString) )	//replace "\n" and add an escape-character before quotes.
		{
			std::size_t QuotePos = TempString.find("\"",0);
			while ( QuotePos != std::string::npos )
			{
				TempString.insert(QuotePos, "\\");
				QuotePos = TempString.find("\"",QuotePos+2);
			}
			xDataDef << "\t\t\"" << TempString << "\"\n";
		}
		xDataDef << "\t}\n";
	}
	else
		xDataDef << "\t\t\"\"\n\t}\n";
	return xDataDef.str();
}
//->general:
void XData::resizeVectors(std::size_t targetSize)
{
	std::string fill = "";
	if (_guiPage.size() > 0)
		fill = _guiPage[_guiPage.size()-1];
	_guiPage.resize(targetSize, fill);
}

void XData::jumpOutOfBrackets(parser::DefTokeniser& tok, int currentDepth) const
{
	while ( tok.hasMoreTokens() && currentDepth > 0)
	{
		std::string token = tok.nextToken();
		if (token == "{")
			currentDepth += 1;
		else if (token == "}")
			currentDepth -= 1;
	}
}

//TwoSidedXData implementations:

void TwoSidedXData::resizeVectors(std::size_t targetSize)
{
	XData::resizeVectors(targetSize);
	_pageLeftBody.resize(targetSize, "");
	_pageLeftTitle.resize(targetSize, "");
	_pageRightBody.resize(targetSize, "");
	_pageRightTitle.resize(targetSize, "");
}

void TwoSidedXData::setPageContent(ContentType cc, std::size_t pageIndex, Side side, const std::string& content)
{
	if (pageIndex >= _numPages)
		throw std::runtime_error(_("Page Index out of bounds."));
	switch (cc)
	{
	case Title:
		switch (side)
		{
		case Left:
			_pageLeftTitle[pageIndex] = content;
			break;
		default:
			_pageRightTitle[pageIndex] = content;
		}
		break;
	default:
		switch (side)
		{
		case Left:
			_pageLeftBody[pageIndex] = content;
			break;
		default:
			_pageRightBody[pageIndex] = content;
		}
	}
}

const std::string& TwoSidedXData::getPageContent(ContentType cc, std::size_t pageIndex, Side side) const
{
	if (pageIndex >= _numPages)
		throw std::runtime_error(_("Page Index out of bounds."));
	switch (cc)
	{
	case Title:
		switch (side)
		{
		case Left:
			return _pageLeftTitle[pageIndex];
		default:
			return _pageRightTitle[pageIndex];
		}
	default:
		switch (side)
		{
		case Left:
			return _pageLeftBody[pageIndex];
		default:
			return _pageRightBody[pageIndex];
		}
	}
}

const std::string TwoSidedXData::getContentDef() const
{
	std::stringstream xDataDef;
	for (std::size_t n = 0; n < _numPages; n++)
	{
		//Page Left Title
		xDataDef << "\t\"page" << n+1 << "_left_title\"\t:\n";
		xDataDef << generateTextDef(_pageLeftTitle[n]);

		//Page Left Body
		xDataDef << "\t\"page" << n+1 << "_left_body\"\t:\n";
		xDataDef << generateTextDef(_pageLeftBody[n]);

		//Page Right Title
		xDataDef << "\t\"page" << n+1 << "_right_title\"\t:\n";
		xDataDef << generateTextDef(_pageRightTitle[n]);

		//Page Right Body
		xDataDef << "\t\"page" << n+1 << "_right_body\"\t:\n";
		xDataDef << generateTextDef(_pageRightBody[n]);
	}
	return xDataDef.str();
}

void TwoSidedXData::togglePageLayout(XDataPtr& target) const
{
	XDataPtr newXData(new OneSidedXData(_name));
	newXData->setNumPages(_numPages*2);	//delete last page if no content later.
	newXData->setSndPageTurn(_sndPageTurn);

	// Add default guiPage to all guiPage-entries.
	newXData->setGuiPage( StringList( newXData->getNumPages(), DEFAULT_ONESIDED_GUI) );

	// Reshuffle the TwoSided pages contents into the OneSided page contents.
	for (std::size_t n = 0; n < _numPages; n++)
	{
		newXData->setPageContent(Title, 2*n, Left, _pageLeftTitle[n] );
		newXData->setPageContent(Body, 2*n, Left, _pageLeftBody[n] );
		newXData->setPageContent(Title, 2*n+1, Left, _pageRightTitle[n]	);
		newXData->setPageContent(Body, 2*n+1, Left, _pageRightBody[n] );
	}

	if ( (_pageRightTitle[_numPages-1] == "") && (_pageRightBody[_numPages-1] == "") )
		// Last page is empty. Set new numpages accordingly.
		newXData->setNumPages(newXData->getNumPages()-1);

	target = newXData;
}
//OneSidedXData implementations:

void OneSidedXData::resizeVectors(std::size_t targetSize)
{
	XData::resizeVectors(targetSize);
	_pageBody.resize(targetSize, "");
	_pageTitle.resize(targetSize, "");
}

void OneSidedXData::setPageContent(ContentType cc, std::size_t pageIndex, Side side, const std::string& content)
{
	if (pageIndex >= _numPages)
		throw std::runtime_error(_("Page Index out of bounds."));
	switch (cc)
	{
	case Title:
		_pageTitle[pageIndex] = content;
		break;
	case Body:
	default:
		_pageBody[pageIndex] = content;
	}
}

const std::string& OneSidedXData::getPageContent(ContentType cc, std::size_t pageIndex, Side side) const
{
	if (pageIndex >= _numPages)
		throw std::runtime_error(_("Page Index out of bounds."));
	switch (cc)
	{
	case Title:
		return _pageTitle[pageIndex];
	default:
		return _pageBody[pageIndex];
	}
}

const std::string OneSidedXData::getContentDef() const
{
	std::stringstream xDataDef;

	for (std::size_t n = 0; n < _numPages; n++)
	{
		//Title
		xDataDef << "\t\"page" << n+1 << "_title\"\t:\n";
		xDataDef << generateTextDef(_pageTitle[n]);

		//Body
		xDataDef << "\t\"page" << n+1 << "_body\"\t:\n";
		xDataDef << generateTextDef(_pageBody[n]);
	}

	return xDataDef.str();
}

void OneSidedXData::togglePageLayout(XDataPtr& target) const
{
	XDataPtr newXData(new TwoSidedXData(_name));
	newXData->setNumPages( (_numPages+1)/2);
	newXData->setSndPageTurn(_sndPageTurn);

	// Add default guiPage to all guiPage-entries.
	newXData->setGuiPage( StringList( newXData->getNumPages(), DEFAULT_TWOSIDED_GUI) );

	// Reshuffle the TwoSided pages contents into the OneSided page contents.
	for (std::size_t n = 0; n < newXData->getNumPages()-1; n++)
	{
		newXData->setPageContent(Title, n, Left, _pageTitle[2*n] );
		newXData->setPageContent(Body, n, Left, _pageBody[2*n] );
		newXData->setPageContent(Title, n, Right, _pageTitle[2*n+1]	);
		newXData->setPageContent(Body, n, Right, _pageBody[2*n+1] );
	}
	newXData->setPageContent(Title, newXData->getNumPages()-1, Left, _pageTitle[2*(newXData->getNumPages()-1)] );
	newXData->setPageContent(Body, newXData->getNumPages()-1, Left, _pageBody[2*(newXData->getNumPages()-1)] );
	if ( (_numPages % 2) == 0)
	{
		// Prevent vector subscript exceeded.
		newXData->setPageContent(Title, newXData->getNumPages()-1, Right, _pageTitle[_numPages-1] );
		newXData->setPageContent(Body, newXData->getNumPages()-1, Right, _pageBody[_numPages-1] );
	}

	target = newXData;
}

} // namespace XData
