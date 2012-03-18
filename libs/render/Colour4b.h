#pragma once

/// 4-element colour in single-byte precision (0 - 255)
struct Colour4b
{
	unsigned char r, g, b, a;

	Colour4b() {}

	Colour4b(unsigned char _r, unsigned char _g,
			 unsigned char _b, unsigned char _a) :
		r(_r), g(_g), b(_b), a(_a)
	{}

	bool operator<(const Colour4b& other) const {
		if (r != other.r) {
			return r < other.r;
		}
		if (g != other.g) {
			return g < other.g;
		}
		if (b != other.b) {
			return b < other.b;
		}
		if (a != other.a) {
			return a < other.a;
		}
		return false;
	}

	bool operator==(const Colour4b& other) const {
		return r == other.r && g == other.g && b == other.b && a == other.a;
	}

	bool operator!=(const Colour4b& other) const {
		return !operator==(other);
	}
};
