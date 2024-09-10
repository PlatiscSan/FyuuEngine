#ifndef VECTOR4F_H
#define VECTOR4F_H

#include <nmmintrin.h>

namespace Fyuu::math {

	class Vector4f final {

	public:

		Vector4f() = default;
		~Vector4f() noexcept = default;

		Vector4f(float x, float y, float z, float w);
		Vector4f(float replicated_value);

		Vector4f operator+(Vector4f const& other) const;
		Vector4f operator-(Vector4f const& other) const;
		Vector4f operator*(float const& scalar) const;
		Vector4f& operator*=(float const& scalar);
		float Dot(Vector4f const& other) const;
		Vector4f VectorPower(Vector4f const& other) const;
		Vector4f ComponentsLessThan(float const& value) const;
		Vector4f SelectComponents(Vector4f const& other, Vector4f const& control_vec) const;
		Vector4f Saturate() const;

		static Vector4f ZeroVector4f();
		static Vector4f OneVector4f();
		static Vector4f Vector4f1110();

		float& operator[](int const& index);

	private:

		float alignas(16) m_data[4];

	};

	

}

#endif // !VECTOR4F_H
