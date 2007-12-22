#ifndef BRUSHTOKENIMPORTER_H_
#define BRUSHTOKENIMPORTER_H_

#include "imap.h"
#include "parser/DefTokeniser.h"

class Face;
class Brush;

// Importer for parsing the Face-related tokens 
class Doom3FaceTokenImporter {
	Face& _face;
public:
	Doom3FaceTokenImporter(Face& face);

	// Public entry point, parses the entire Face
	bool importTokens(parser::DefTokeniser& tokeniser);

private:
	// Imports the face plane tokens (plane equation)
	void importFacePlaneTokens(parser::DefTokeniser& tokeniser);
	
	// Imports the texture matrix
	void importTexDefTokens(parser::DefTokeniser& tokeniser);
	
	// Imports the shader name
	void importFaceShaderTokens(parser::DefTokeniser& tokeniser);
	
	// Imports the contents flags (zero for Doom3)
	void importContentsFlagsTokens(parser::DefTokeniser& tok);
};

// Importer able to parse brush faces from tokens
class BrushTokenImporter : 
	public MapImporter
{
	Brush& _brush;
public:
	BrushTokenImporter(Brush& brush);
	
    /**
     * Required token import method.
     */
    virtual bool importTokens(parser::DefTokeniser& tokeniser);
};

#endif /*BRUSHTOKENIMPORTER_H_*/
