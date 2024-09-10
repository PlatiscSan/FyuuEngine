#ifndef COLOR_H
#define COLOR_H

#include "Math/Vector4f.h"

namespace Fyuu::core {

	class Color final {

	public:

		enum RGBA {
			R, G, B, A
		};

		Color();
		Color(math::Vector4f const& vec);
		Color(float r, float g, float b, float a = 1.0f);
		Color(std::uint16_t r, std::uint16_t g, std::uint16_t b, std::uint16_t a = 255, std::uint16_t bit_depth = 8);

		Color(std::uint32_t rgba_little_endian);

		float& operator[](RGBA const& rgba);
		float& operator[](int const& index);

		void SetRGB(float r, float g, float b);

		Color TosRGB();
		Color FromsRGB();


	private:

		math::Vector4f m_value;

	};


}

#endif // !COLOR_H
