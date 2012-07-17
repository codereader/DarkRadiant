#include "Quaternion.h"

#include "Matrix4.h"

Quaternion Quaternion::createForMatrix(const Matrix4& matrix4)
{
	Matrix4 transposed = matrix4.getTransposed();

	double trace = transposed[0] + transposed[5] + transposed[10] + 1.0;

	if (trace > 0.0001)
	{
		double S = 0.5 / sqrt(trace);

		return Quaternion(
			(transposed[9] - transposed[6]) * S,
			(transposed[2] - transposed[8]) * S,
			(transposed[4] - transposed[1]) * S,
			0.25f / S
		);
	}

	if (transposed[0] >= transposed[5] && transposed[0] >= transposed[10])
	{
		double S = 2.0 * sqrt(1.0 + transposed[0] - transposed[5] - transposed[10]);

		return Quaternion(
			0.25 / S,
			(transposed[1] + transposed[4]) / S,
			(transposed[2] + transposed[8]) / S,
			(transposed[6] + transposed[9]) / S
		);
	}

	if (transposed[5] >= transposed[0] && transposed[5] >= transposed[10])
	{
		double S = 2.0 * sqrt(1.0 + transposed[5] - transposed[0] - transposed[10]);

		return Quaternion(
			(transposed[1] + transposed[4]) / S,
			0.25 / S,
			(transposed[6] + transposed[9]) / S,
			(transposed[2] + transposed[8]) / S
		);
	}

	double S = 2.0 * sqrt(1.0 + transposed[10] - transposed[0] - transposed[5]);
	
	return Quaternion(
		(transposed[2] + transposed[8]) / S,
		(transposed[6] + transposed[9]) / S,
		0.25 / S,
		(transposed[1] + transposed[4]) / S
	);
}
