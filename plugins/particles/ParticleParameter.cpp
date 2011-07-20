#include "ParticleParameter.h"

#include "ParticleStage.h"

#include "itextstream.h"
#include <boost/lexical_cast.hpp>

namespace particles
{

void ParticleParameter::setFrom(float value)
{
	_from = value;

	_stage.onParameterChanged();
}

void ParticleParameter::setTo(float value)
{
	_to = value;

	_stage.onParameterChanged();
}

void ParticleParameter::parseFromTokens(parser::DefTokeniser& tok)
{
	std::string val = tok.nextToken();

	try
	{
		setFrom(boost::lexical_cast<float>(val));
	}
	catch (boost::bad_lexical_cast&)
	{
		globalErrorStream() << "[particles] Bad lower value, token is '" <<
			val << "'" << std::endl;
	}

	if (tok.peek() == "to")
	{
		// Upper value is there, parse it
		tok.skipTokens(1); // skip the "to"

		val = tok.nextToken();

		try
		{
			// cut off the quotes before converting to double
			setTo(boost::lexical_cast<float>(val));
		}
		catch (boost::bad_lexical_cast&)
		{
			globalErrorStream() << "[particles] Bad upper value, token is '" <<
				val << "'" << std::endl;
		}
	}
	else
	{
		setTo(getFrom());
	}
}

} // namespace
