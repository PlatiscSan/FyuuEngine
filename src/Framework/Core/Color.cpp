
#include "pch.h"
#include "Color.h"

using namespace Fyuu::core;
using namespace Fyuu::math;

Fyuu::core::Color::Color() 
	:m_value(math::Vector4f::OneVector4f()) {}

Fyuu::core::Color::Color(math::Vector4f const& vec)
	:m_value(vec) {}

Fyuu::core::Color::Color(float r, float g, float b, float a) 
	:m_value(r, g, b, a) {}

Fyuu::core::Color::Color(std::uint16_t r, std::uint16_t g, std::uint16_t b, std::uint16_t a, std::uint16_t bit_depth)
	:m_value(r, g, b, a) {
	m_value *= static_cast<float>((1 << bit_depth) - 1);
}

Fyuu::core::Color::Color(std::uint32_t rgba_little_endian) {

	m_value[0] = (float)((rgba_little_endian >> 0) & 0xFF);
	m_value[1] = (float)((rgba_little_endian >> 8) & 0xFF);
	m_value[2] = (float)((rgba_little_endian >> 16) & 0xFF);
	m_value[3] = (float)((rgba_little_endian >> 24) & 0xFF);

	m_value *= 1.0f / 255.0f;

}

float& Fyuu::core::Color::operator[](RGBA const& rgba) {

	switch (rgba) {

	case RGBA::R:
		return m_value[0];

	case RGBA::G:
		return m_value[1];

	case RGBA::B:
		return m_value[2];

	case RGBA::A:
		return m_value[3];

	default:
		throw std::invalid_argument("Invalid Index");

	}
}

float& Fyuu::core::Color::operator[](int const& index) {
	return m_value[index];
}

void Fyuu::core::Color::SetRGB(float r, float g, float b) {



}

Color Fyuu::core::Color::TosRGB() {

	Vector4f v = m_value.Saturate();

	// large than 0.0031308, C_srgb = 1.055 * pow(C_linear, 1/2.4) - 0.055

	Vector4f result = v.VectorPower(Vector4f(1.0f / 2.4f)) * 1.055f - Vector4f(0.055f);

	// less than 0.0031308, C_srgb = 12.92 * C_linear

	Vector4f control = v.ComponentsLessThan(0.0031308f);
	Vector4f scaled = v * 12.92f;
	result = result.SelectComponents(scaled, control);

	result[3] = m_value[3];

	return Color(result);

}

Color Fyuu::core::Color::FromsRGB() {

	Vector4f v = m_value.Saturate();

	Vector4f scaled = (v + Vector4f(0.055f)) * (1.0f / 1.055f);
	Vector4f result = scaled.VectorPower(Vector4f(2.4f));
	Vector4f control = v.ComponentsLessThan(0.0031308f);

	scaled = v * (1.0f / 12.92f);
	result = result.SelectComponents(scaled, control);

	result[3] = m_value[3];

	return Color(result);

}
