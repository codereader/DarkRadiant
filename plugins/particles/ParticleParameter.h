#ifndef _PARTICLE_PARAMETER_H_
#define _PARTICLE_PARAMETER_H_

#include "iparticlestage.h"

#include "parser/DefTokeniser.h"
#include <boost/shared_ptr.hpp>

namespace particles
{

class StageDef;

/**
 * greebo: A particle parameter represents a bounded member value
 * of a particle stage (e.g. speed or size).
 *
 * Use the evaluate() method to retrieve a particular value.
 *
 * It is modeled after the idParticleParam class in the D3 SDK.
 */
class ParticleParameter :
	public IParticleParameter
{
	// The owner stage
	StageDef& _stage;

	float _from;	// lower bound
	float _to;		// upper bound

public:

	ParticleParameter(StageDef& stage) :
		_stage(stage),
		_from(0),
		_to(0)
	{}

	ParticleParameter(StageDef& stage, float constantValue) :
		_stage(stage),
		_from(constantValue),
		_to(constantValue)
	{}

	ParticleParameter(StageDef& stage, float from, float to) :
		_stage(stage),
		_from(from),
		_to(to)
	{}

	float getFrom() const
	{
		return _from;
	}

	float getTo() const
	{
		return _to;
	}

	void setFrom(float value);

	void setTo(float value);

	float evaluate(float fraction) const
	{
		return _from + fraction * (_to - _from);
	}

	float integrate(float fraction) const
	{
		return (_to - _from) * 0.5f * (fraction*fraction) + _from * fraction;
	}

	ParticleParameter& operator=(const IParticleParameter& other)
	{
		setFrom(other.getFrom());
		setTo(other.getTo());

		return *this;
	}

	bool operator==(const IParticleParameter& other) const
	{
		return getFrom() == other.getFrom() && getTo() == other.getTo();
	}

	bool operator!=(const IParticleParameter& other) const
	{
		return !operator==(other);
	}

	void parseFromTokens(parser::DefTokeniser& tok);
};
typedef boost::shared_ptr<ParticleParameter> ParticleParameterPtr;

// Stream insertion
std::ostream& operator<<(std::ostream& stream, const ParticleParameter& param);

} // namespace

#endif /* _PARTICLE_PARAMETER_H_ */
