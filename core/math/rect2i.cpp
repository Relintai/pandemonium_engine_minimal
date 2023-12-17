
/*  rect2.cpp                                                            */


#include "core/math/transform_2d.h" // Includes rect2.h but Rect2 needs Transform2D

Rect2i::operator String() const {
	return "[P: " + position.operator String() + ", S: " + size + "]";
}

