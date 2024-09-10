
#include "pch.h"
#include "Vector4f.h"

using namespace Fyuu::math;

Fyuu::math::Vector4f::Vector4f(float x, float y, float z, float w) {

	m_data[0] = x;
	m_data[1] = y;
	m_data[2] = z;
	m_data[3] = w;

}

Fyuu::math::Vector4f::Vector4f(float replicated_value) {

	m_data[0] = replicated_value;
	m_data[1] = replicated_value;
	m_data[2] = replicated_value;
	m_data[3] = replicated_value;

}

Vector4f Fyuu::math::Vector4f::operator+(Vector4f const& other) const {
	
	__m128 v1 = _mm_load_ps(m_data);
	__m128 v2 = _mm_load_ps(other.m_data);
	__m128 v3 = _mm_add_ps(v1, v2);

	Vector4f ret_vector{};
	_mm_store_ps(ret_vector.m_data, v3);

	return ret_vector;

}

Vector4f Fyuu::math::Vector4f::operator-(Vector4f const& other) const {

	__m128 v1 = _mm_load_ps(m_data);
	__m128 v2 = _mm_load_ps(other.m_data);
	__m128 v3 = _mm_sub_ps(v1, v2);

	Vector4f ret_vector{};
	_mm_store_ps(ret_vector.m_data, v3);

	return ret_vector;

}

Vector4f Fyuu::math::Vector4f::operator*(float const& scalar) const {

	__m128 v1 = _mm_load_ps(m_data);
	__m128 _scalar = _mm_set1_ps(scalar);
	__m128 v2 = _mm_mul_ps(v1, _scalar);

	Vector4f ret_vector{};
	_mm_store_ps(ret_vector.m_data, v2);

	return ret_vector;
}

Vector4f& Fyuu::math::Vector4f::operator*=(float const& scalar) {

	__m128 v1 = _mm_load_ps(m_data);
	__m128 _scalar = _mm_set1_ps(scalar);
	__m128 v2 = _mm_mul_ps(v1, _scalar);

	_mm_store_ps(m_data, v2);

	return *this;

}

float Fyuu::math::Vector4f::Dot(Vector4f const& other) const {
	
	__m128 v1 = _mm_load_ps(m_data);
	__m128 v2 = _mm_load_ps(other.m_data);

	__m128 r1 = _mm_mul_ps(v1, v2);

	__m128 shuf = _mm_shuffle_ps(r1, r1, _MM_SHUFFLE(2, 3, 0, 1));
	__m128 sums = _mm_add_ps(r1, shuf);
	shuf = _mm_movehl_ps(shuf, sums);
	sums = _mm_add_ss(sums, shuf);
	float result = _mm_cvtss_f32(sums);

	return result;

}

Vector4f Fyuu::math::Vector4f::VectorPower(Vector4f const& other) const {

	__m128 v1 = _mm_load_ps(m_data);
	__m128 v2 = _mm_load_ps(other.m_data);

	__m128 v3 = _mm_pow_ps(v1, v2);

	Vector4f ret_vector{};
	_mm_store_ps(ret_vector.m_data, v3);

	return ret_vector;
}

Vector4f Fyuu::math::Vector4f::ComponentsLessThan(float const& value) const {

	__m128 v1 = _mm_load_ps(m_data);
	__m128 v2 = _mm_set1_ps(value);

	__m128 v3 = _mm_cmplt_ps(v1, v2);

	Vector4f ret_vector{};
	_mm_store_ps(ret_vector.m_data, v3);

	return ret_vector;

}

Vector4f Fyuu::math::Vector4f::SelectComponents(Vector4f const& other, Vector4f const& control_vec) const {

	__m128 v1 = _mm_load_ps(m_data);
	__m128 v2 = _mm_load_ps(other.m_data);
	__m128 control = _mm_load_ps(control_vec.m_data);

	__m128 result = _mm_blendv_ps(v1, v2, control);

	Vector4f ret_vector{};
	_mm_store_ps(ret_vector.m_data, result);

	return ret_vector;
}

Vector4f Fyuu::math::Vector4f::Saturate() const {

	__m128 zero = _mm_setzero_ps();
	__m128 one = _mm_set1_ps(1.0f);
 
	__m128 v = _mm_load_ps(m_data);

	v = _mm_max_ps(v, zero);
	v = _mm_min_ps(v, one);

	Vector4f ret_vector{};
	_mm_store_ps(ret_vector.m_data, v);
	return ret_vector;
}


Vector4f Fyuu::math::Vector4f::ZeroVector4f() {
	return Vector4f(0.0f, 0.0f, 0.0f, 0.0f);
}

Vector4f Fyuu::math::Vector4f::OneVector4f() {
	return Vector4f(1.0f, 1.0f, 1.0f, 1.0f);
}

Vector4f Fyuu::math::Vector4f::Vector4f1110() {
	return Vector4f(1.0f, 1.0f, 1.0f, 0.0f);
}

float& Fyuu::math::Vector4f::operator[](int const& index) {
	
	if (index < 0 || index > 4) {
		throw std::invalid_argument("Invalid index for vector4f");
	}

	return m_data[index];

}
