#include "InfoFile.h"

#include "stream/textstream.h"

#include "Doom3MapFormat.h"
#include <boost/lexical_cast.hpp>

namespace map {

// Pass the input stream to the constructor
InfoFile::InfoFile(std::istream& infoStream) :
	_tok(infoStream),
	_isValid(false)
{}

const InfoFile::LayerNameList& InfoFile::getLayerNames() const {
	return _layerNames;
}

void InfoFile::parse() {
	// parse the header
	try {
		_tok.assertNextToken("DarkRadiant");
		_tok.assertNextToken("Map");
		_tok.assertNextToken("Information");
		_tok.assertNextToken("File");
		_tok.assertNextToken("Version");
		float version = boost::lexical_cast<float>(_tok.nextToken());

		if (version != MAP_INFO_VERSION) {
			throw parser::ParseException("Map version invalid");
		}
	}
	catch (parser::ParseException e) {
        globalErrorStream() 
            << "[mapdoom3] Unable to parse info file header: " 
            << e.what() << "\n";
        return;
    }
    catch (boost::bad_lexical_cast e) {
        globalErrorStream() 
            << "[mapdoom3] Unable to parse info file version: " 
            << e.what() << "\n";
        return;
    }

	_tok.assertNextToken("{");
	
	_isValid = true;
}

void InfoFile::parseLayerNames() {
	
}

} // namespace map
