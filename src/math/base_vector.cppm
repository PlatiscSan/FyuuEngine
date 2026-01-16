export module math:base_vector;
import std;
namespace fyuu_engine::math {

	export template <class Derived, std::size_t dim> class BaseVector {
	public:
		float operator[](size_t index) const {
			return static_cast<Derived const*>(this)->At(index);
		}

		float& operator[](size_t index) {
			return static_cast<Derived*>(this)->At(index);
		}

		bool operator==(Derived const& other) const {
			return static_cast<Derived*>(this)->IsEqualTo(other);
		}

		bool operator!=(Derived const& other) const {
			return !((*this) == other);
		}

		Derived operator+(Derived const& other) const {
			return static_cast<Derived const*>(this)->Add(other);
		}

		Derived operator-(Derived const& other) const {
			return static_cast<Derived const*>(this)->Sub(other);
		}

		Derived operator*(float scalar) const {
			return static_cast<Derived const*>(this)->Scale(scalar);
		}

		Derived operator/(float scalar) const {
			return static_cast<Derived const*>(this)->Scale(1.0f / scalar);
		}

		Derived& operator+=(Derived const& other) {
			return static_cast<Derived*>(this)->AddedBy(other);
		}

		Derived& operator-=(Derived const& other) {
			return static_cast<Derived*>(this)->SubtractedFrom(other);
		}

		float Dot(Derived const& other) const {
			return static_cast<Derived const*>(this)->DotImpl(other);
		}

		Derived Hadamard(Derived const& other) const {
			return static_cast<Derived const*>(this)->HadamardImpl(other);
		}

		Derived Cross(Derived const& other) const {
			static_assert(dim == 3, "Cross product is only defined for 3D vectors.");
			return static_cast<Derived const*>(this)->CrossImpl(other);
		}

		float Norm() const {
			return std::sqrt(this->Dot(*static_cast<Derived const*>(this)));
		}

	};

}