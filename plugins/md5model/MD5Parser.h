#ifndef MD5PARSER_H_
#define MD5PARSER_H_

#include "MD5Model.h"
#include "parser/DefTokeniser.h"

namespace md5 {

/** greebo: The MD5 parser extends the DefTokeniser by providing functions
 *          to parse the inputstream to a given MD5Model. 
 */
class MD5Parser :
	public parser::BasicDefTokeniser<std::istream>
{
public:
	// Construct the MD5 Parser using the inputstream
	MD5Parser(std::istream& inputStream);
	
	/** greebo: Loads the data into the given model, using the
	 *          inputstream given to the constructor.
	 * 
	 * Note: might throw exceptions on parse errors, see parser::DefTokeniser.
	 */
	void parseToModel(MD5Model& model);
	
private:
	/**
	 * Helper: Parse an MD5 vector, which consists of three separated numbers 
	 * enclosed with parentheses.
	 */
	Vector3 parseVector3();
};

} // namespace md5

#endif /*MD5PARSER_H_*/
