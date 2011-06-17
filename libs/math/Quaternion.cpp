#include "Quaternion.h"

#include "Matrix4.h"

Quaternion Quaternion::createForMatrix(const Matrix4& matrix4)
{
	Matrix4 transposed = matrix4.getTransposed();

	float trace = transposed[0] + transposed[5] + transposed[10] + 1.0f;

	if (trace > 0.0001)
	{
		float S = 0.5f / sqrt(trace);

		return Quaternion(
			(transposed[9] - transposed[6]) * S,
			(transposed[2] - transposed[8]) * S,
			(transposed[4] - transposed[1]) * S,
			0.25f / S
		);
	}

	if (transposed[0] >= transposed[5] && transposed[0] >= transposed[10])
	{
		float S = 2.0f * sqrt(1.0f + transposed[0] - transposed[5] - transposed[10]);

		return Quaternion(
			0.25f / S,
			(transposed[1] + transposed[4]) / S,
			(transposed[2] + transposed[8]) / S,
			(transposed[6] + transposed[9]) / S
		);
	}

	if (transposed[5] >= transposed[0] && transposed[5] >= transposed[10])
	{
		float S = 2.0f * sqrt(1.0f + transposed[5] - transposed[0] - transposed[10]);

		return Quaternion(
			(transposed[1] + transposed[4]) / S,
			0.25f / S,
			(transposed[6] + transposed[9]) / S,
			(transposed[2] + transposed[8]) / S
		);
	}

	float S = 2.0f * sqrt(1.0f + transposed[10] - transposed[0] - transposed[5]);
	
	return Quaternion(
		(transposed[2] + transposed[8]) / S,
		(transposed[6] + transposed[9]) / S,
		0.25f / S,
		(transposed[1] + transposed[4]) / S
	);
}
