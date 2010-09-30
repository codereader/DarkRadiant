#include "ParticleStage.h"

#include "itextstream.h"
#include <boost/lexical_cast.hpp>

namespace particles
{

ParticleStage::ParticleStage()
{
	reset();
}

ParticleStage::ParticleStage(parser::DefTokeniser& tok)
{
	// Parse from the tokens, but don't allow any parse exceptions
	// from leaving this destructor.
	try
	{
		parseFromTokens(tok);
	}
	catch (parser::ParseException& p)
	{
		globalErrorStream() << "[particles]: Could not parse particle stage: " << 
			p.what() << std::endl;
	}
	catch (boost::bad_lexical_cast& e)
	{
		globalErrorStream() << "[particles]: Invalid cast when parsing particle stage: " << 
			e.what() << std::endl;
	}
}

void ParticleStage::reset()
{
	_count = 1;
	_material.clear();

	_duration = 1;
	_colour = Vector4(1,1,1,1);
	_fadeColour = Vector4(1,1,1,0);

	_fadeInFraction = 0;
	_fadeOutFraction = 0;
	_fadeIndexFraction = 0;
}

void ParticleStage::parseFromTokens(parser::DefTokeniser& tok)
{
	reset();

	std::string token = tok.nextToken();

	while (token != "}")
	{
		if (token == "count")
		{
			try
			{
				setCount(boost::lexical_cast<int>(tok.nextToken()));
			}
			catch (boost::bad_lexical_cast&)
			{
				std::cerr << "[particles] Bad count value '" << token 
						  << "'" << std::endl;
			}
		}
		else if (token == "color")
		{
			// Read 4 values and assemble as a vector4
			setColour(parseVector4(tok));
		}
		
		token = tok.nextToken();
	}
}

Vector4 ParticleStage::parseVector4(parser::DefTokeniser& tok)
{
	Vector4 col;

	try
	{
		col.x() = boost::lexical_cast<float>(tok.nextToken());
		col.y() = boost::lexical_cast<float>(tok.nextToken());
		col.z() = boost::lexical_cast<float>(tok.nextToken());
		col.w() = boost::lexical_cast<float>(tok.nextToken());
	}
	catch (boost::bad_lexical_cast&)
	{
		col = Vector4(1,1,1,1);

		globalErrorStream() << "[particles] Bad colour value." << std::endl;
	}

	return col;
}

} // namespace
