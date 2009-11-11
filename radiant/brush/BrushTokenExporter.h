#ifndef BRUSHTOKENEXPORTER_H_
#define BRUSHTOKENEXPORTER_H_

#include "imap.h"
class Brush;
class Face;
#include <ostream>

/** Token exporter for Doom 3 brush faces.
 */
class Doom3FaceTokenExporter
{
	const Face& _face;
public:
	Doom3FaceTokenExporter(const Face& face);

	// Export tokens to the provided stream
	void exportTokens(std::ostream& os) const;
	
private:
	// Export the plane specification for a Doom 3 brushface.
	void exportFacePlaneTokens(std::ostream& os) const;
	
	// Export the texture coordinate information from a Doom 3 brushface.
	void exportTexDefTokens(std::ostream& os) const;
	
	// Export Doom 3 face shader information.
	void exportFaceShaderTokens(std::ostream& os) const;
	
	// Export shader surface flags.
	void exportContentsFlagsTokens(std::ostream& os) const;
};

/* Token exporter for Doom 3 brushes.
 */
class BrushTokenExporter : 
	public MapExporter
{
	const Brush& _brush;

public:
	BrushTokenExporter(const Brush& brush);
	virtual ~BrushTokenExporter() {}

	// Required export function
	virtual void exportTokens(std::ostream& os) const;
};

#endif /*BRUSHTOKENEXPORTER_H_*/
