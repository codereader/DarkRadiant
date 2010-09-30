#include "ParticleStage.h"

#include "itextstream.h"
#include <boost/lexical_cast.hpp>

namespace particles
{

namespace
{
	// Returns next token as number, or returns zero on fail, printing the given error message
	template<typename Type>
	inline Type parseWithErrorMsg(parser::DefTokeniser& tok, const char* errorMsg)
	{
		std::string str = tok.nextToken();

		try
		{
			return boost::lexical_cast<Type>(str);
		}
		catch (boost::bad_lexical_cast&)
		{
			globalErrorStream() << "[particles] " << errorMsg << ", token is '" << 
				str << "'" << std::endl;
			return 0;
		}
	}
}

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
	_cycles = 0;
	_bunching = 0.0f;

	_timeOffset = 0;
	_deadTime = 0;

	recalculateCycleMsec();

	_colour = Vector4(1,1,1,1);
	_fadeColour = Vector4(1,1,1,0);

	_fadeInFraction = 0.0f;
	_fadeOutFraction = 0.0f;
	_fadeIndexFraction = 0.0f;
}

void ParticleStage::parseFromTokens(parser::DefTokeniser& tok)
{
	reset();

	std::string token = tok.nextToken();

	while (token != "}")
	{
		if (token == "count")
		{
			setCount(parseWithErrorMsg<int>(tok, "Bad count value"));
		}
		else if (token == "material")
		{
			setMaterialName(tok.nextToken());
		}
		else if (token == "time") // duration
		{
			setDuration(parseWithErrorMsg<float>(tok, "Bad duration value"));
		}
		else if (token == "cycles")
		{
			setCycles(parseWithErrorMsg<float>(tok, "Bad cycles value"));
		}
		else if (token == "timeOffset")
		{
			setTimeOffset(parseWithErrorMsg<float>(tok, "Bad time offset value"));
		}
		else if (token == "deadTime")
		{
			setDeadTime(parseWithErrorMsg<float>(tok, "Bad dead time value"));
		}
		else if (token == "bunching")
		{
			setBunching(parseWithErrorMsg<float>(tok, "Bad bunching value"));
		}
		else if (token == "color")
		{
			setColour(parseVector4(tok));
		}
		else if (token == "fadeColor")
		{
			setFadeColour(parseVector4(tok));
		}
		else if (token == "fadeIn")
		{
			setFadeInFraction(parseWithErrorMsg<float>(tok, "Bad fade in fraction value"));
		}
		else if (token == "fadeOut")
		{
			setFadeOutFraction(parseWithErrorMsg<float>(tok, "Bad fade out fraction value"));
		}
		else if (token == "fadeIndex")
		{
			setFadeIndexFraction(parseWithErrorMsg<float>(tok, "Bad fade index fraction value"));
		}
		
		token = tok.nextToken();
	}
}

Vector4 ParticleStage::parseVector4(parser::DefTokeniser& tok)
{
	// Read 4 values and assemble to a Vector4
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
