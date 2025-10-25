export module math;
import std;

namespace fyuu_engine::math {

	export template <class DerivedVector> class BaseVector {
	protected:
		struct EqualFlag {};

	public:
		float const& operator[](std::size_t index) const noexcept {
			return static_cast<DerivedVector*>(this)->At(index);
		}

		float& operator[](std::size_t index) noexcept {
			return static_cast<DerivedVector*>(this)->At(index);
		}

		DerivedVector operator+(DerivedVector const& other) {
			return static_cast<DerivedVector*>(this)->AddImpl(other);
		}

		DerivedVector operator-(DerivedVector const& other) {
			return static_cast<DerivedVector*>(this)->SubImpl(other);
		}

		DerivedVector& operator+=(DerivedVector const& other) {
			return static_cast<DerivedVector*>(this)->AddImpl(other, EqualFlag());
		}

		DerivedVector& operator-=(DerivedVector const& other) {
			return static_cast<DerivedVector*>(this)->SubImpl(other, EqualFlag());
		}

		DerivedVector Hadamard(DerivedVector const& other) {
			return static_cast<DerivedVector*>(this)->HadamardImpl(other);
		}

		DerivedVector operator*(float scalar) {
			return static_cast<DerivedVector*>(this)->ScaleImpl(scalar);
		}

		DerivedVector operator*=(float scalar) {
			return static_cast<DerivedVector*>(this)->ScaleImpl(scalar, EqualFlag());
		}

		DerivedVector operator/(float scalar) {
			return static_cast<DerivedVector*>(this)->DivScaleImpl(scalar);
		}

		DerivedVector operator/=(float scalar) {
			return static_cast<DerivedVector*>(this)->DivScaleImpl(scalar, EqualFlag());
		}

		float Dot(DerivedVector const& other) {
			return static_cast<DerivedVector*>(this)->DotImpl(other);
		}

		DerivedVector Cross(DerivedVector const& other) {
			return static_cast<DerivedVector*>(this)->CrossImpl(other);
		}

		DerivedVector Normalized() {
			return static_cast<DerivedVector*>(this)->NormalizedImpl();
		}

		DerivedVector& Normalize() {
			return static_cast<DerivedVector*>(this)->NormalizeImpl();
		}

		float Norm() const {
			return static_cast<DerivedVector*>(this)->NormImpl();
		}

		bool operator==(DerivedVector const& other) const {
			return static_cast<DerivedVector const*>(this)->EqualsImpl(other);
		}

		bool operator!=(DerivedVector const& other) const {
			return !(*this == other);
		}

		static DerivedVector Zero() {
			return DerivedVector::ZeroImpl();
		}

		static DerivedVector One() {
			return DerivedVector::OneImpl();
		}

	};

	export template <class DerivedMatrix> class BaseMatrix {
	public:
		float const& operator()(std::size_t row, std::size_t col) const noexcept {
			return static_cast<DerivedMatrix*>(this)->At(row, col);
		}

		float& operator()(std::size_t row, std::size_t col) noexcept {
			return static_cast<DerivedMatrix*>(this)->At(row, col);
		}

		DerivedMatrix& operator+(DerivedMatrix const& other) {
			return static_cast<DerivedMatrix*>(this)->AddImpl(other);
		}

		DerivedMatrix operator-(DerivedMatrix const& other) {
			return static_cast<DerivedMatrix*>(this)->SubImpl(other);
		}

		DerivedMatrix operator*(float scalar) {
			return static_cast<DerivedMatrix*>(this)->ScaleImpl(scalar);
		}

		DerivedMatrix operator/(float scalar) {
			return static_cast<DerivedMatrix*>(this)->DivScaleImpl(scalar);
		}

		DerivedMatrix operator*(DerivedMatrix const& other) {
			return static_cast<DerivedMatrix*>(this)->MulImpl(other);
		}

		DerivedMatrix Hadamard(DerivedMatrix const& other) {
			return static_cast<DerivedMatrix*>(this)->HadamardImpl(other);
		}

		DerivedMatrix Transpose() {
			return static_cast<DerivedMatrix*>(this)->TransposeImpl();
		}

		DerivedMatrix Inverse() {
			return static_cast<DerivedMatrix*>(this)->InverseImpl();
		}

		bool operator==(DerivedMatrix const& other) const {
			return static_cast<DerivedMatrix const*>(this)->IsEqual(other);
		}

		bool operator!=(DerivedMatrix const& other) const {
			return !(*this == other);
		}

		static DerivedMatrix Identity() {
			return DerivedMatrix::IdentityImpl();
		}

		static DerivedMatrix Zero() {
			return DerivedMatrix::ZeroImpl();
		}

		template <class... Args>
		static DerivedMatrix Diagonal(Args&&... args) requires ((std::integral<Args> || std::floating_point<Args>)&&...) {
			return DerivedMatrix::DiagonalImpl(std::forward<Args>(args)...);
		}

	};

	export template <class DerivedQuaternion> class BaseQuaternion {
	public:
		DerivedQuaternion operator*(DerivedQuaternion const& other) {
			return static_cast<DerivedQuaternion*>(this)->MulImpl(other);
		}

		DerivedQuaternion operator*(float scalar) {
			return static_cast<DerivedQuaternion*>(this)->ScaleImpl(scalar);
		}

		DerivedQuaternion Normalized() {
			return static_cast<DerivedQuaternion*>(this)->NormalizeImpl();
		}

		DerivedQuaternion Conjugate() {
			return static_cast<DerivedQuaternion*>(this)->ConjugateImpl();
		}

		DerivedQuaternion Inverse() {
			return static_cast<DerivedQuaternion*>(this)->InverseImpl();
		}

		template <class Matrix>
		Matrix ToMatrix() {
			return static_cast<DerivedQuaternion*>(this)->ToMatrixImpl();
		}

		static DerivedQuaternion FromAxisAngle(DerivedQuaternion const& axis, float angle) {
			return DerivedQuaternion::FromAxisAngleImpl(axis, angle);
		}

		static DerivedQuaternion FromEulerAngles(float pitch, float yaw, float roll) {
			return DerivedQuaternion::FromEulerAnglesImpl(pitch, yaw, roll);
		}

		static DerivedQuaternion Identity() {
			return DerivedQuaternion::IdentityImpl();
		}

	};

	export template <class T> concept VectorConcept = requires (T vec1, T vec2) {
		{ vec1[std::decay_t<std::size_t>()] } -> std::convertible_to<std::reference_wrapper<float>>;
		{ vec1[std::decay_t<std::size_t>()] } -> std::convertible_to<std::add_const_t<std::reference_wrapper<float>>>;
		{ vec1 += vec2 } -> std::convertible_to<T>;
		{ vec1 + vec2 } -> std::convertible_to<T>;
		{ vec1 *= std::declval<float>() } -> std::convertible_to<T>;
		{ vec1 * std::declval<float>() } -> std::convertible_to<T>;
		{ vec1 /= std::declval<float>() } -> std::convertible_to<T>;
		{ vec1 / std::declval<float>() } -> std::convertible_to<T>;
		{ vec1.Dot(vec2) } -> std::same_as<float>;
		{ vec1.Hadamard(vec2) } -> std::convertible_to<T>;
		{ T::Zero() } -> std::convertible_to<T>;
		{ T::One() } -> std::convertible_to<T>;
	};

	export template <class T> concept QuaternionConcept = requires (T q1, T q2) {
		{ q1 * q2 } -> std::convertible_to<T>;
		{ q1 * std::declval<float>() } -> std::convertible_to<T>;
		{ T::Identity() } -> std::convertible_to<T>;
	};
	
	export template <class T> concept MatrixConcept = requires(T m1, T m2) {
		{ m1(std::decay_t<std::size_t>(), std::decay_t<std::size_t>()) } -> std::convertible_to<std::reference_wrapper<float>>;
		{ m1(std::decay_t<std::size_t>(), std::decay_t<std::size_t>()) } -> std::convertible_to<std::add_const_t<std::reference_wrapper<float>>>;
		{ m1 + m2 } -> std::convertible_to<T>;
		{ m1 * m2 } -> std::convertible_to<T>;
		{ m1 * std::declval<float>() } -> std::convertible_to<T>;
		{ m1.Hadamard(m2) } -> std::convertible_to<T>;
		{ m1.Transpose() } -> std::convertible_to<T>;
		{ m1.Inverse() } -> std::convertible_to<T>;
		{ m1 == m2 } -> std::same_as<bool>;
		{ m1 != m2 } -> std::same_as<bool>;
		{ T::Identity() } -> std::convertible_to<T>;
		{ T::Zero() } -> std::convertible_to<T>;
	};

	/// @brief				3D transform matrix interface
	/// @tparam Vector		Should be 3-dim vector
	/// @tparam Quaternion	quaternion that satisfies the concept 
	/// @tparam Matrix		Should be 4*4 matrix
	export template <VectorConcept Vector, MatrixConcept Matrix, QuaternionConcept Quaternion> class Transform {
	protected:
		Matrix m_transform_matrix;

		static Matrix CreateScaleMatrix(Vector const& scale) {

			/*
			*	Scale matrix:
			*	⎡	Sx	0	0	0	⎤
			*	|	0	Sy	0	0	|
			*	|	0	0	Sz	0	|
			*	⎣	0	0	0	1	⎦
			*/

			return Matrix::Diagonal(scale[0], scale[1], scale[2], 1.0f);

		}

		static Matrix CreateTranslationMatrix(Vector const& translation) {

			/*
			*	Traslation matrix:
			*	⎡	1	0	0	Tx	⎤
			*	|	0	1	0	Ty	|
			*	|	0	0	1	Tz	|
			*	⎣	0	0	0	1	⎦
			* 
			*/

			Matrix translation_matrix = Matrix::Identity();
			for (std::size_t i = 0; i < 3; ++i) {
				translation_matrix(i, 3) = translation[i];
			}

			return translation_matrix;

		}

	public:
		Transform(Vector const& position, Vector const& scale, Quaternion const& rotation) {
			/*
			* 
			*	Constructing an affine transformation matrix M = TRS
			*	where T is for Translation matrix, R is for rotation matrix, S is for scale matrix
			* 
			*/

			Matrix translation_matrix = CreateTranslationMatrix(position);
			Matrix scale_matrix = CreateScaleMatrix(scale);


		}

		Vector operator()(Vector const& position) const {
			return m_transform_matrix * position;
		}

	};

	export template <VectorConcept Vector> class AxisAlignedBox {
	protected:
		Vector m_center = Vector::Zero();
		Vector m_half_extent = Vector::Zero();
		Vector m_min_corner = { 
			std::numeric_limits<float>::max(),
			std::numeric_limits<float>::max(),
			std::numeric_limits<float>::max()
		};
		Vector m_max_corner = { 
			-std::numeric_limits<float>::max(),
			-std::numeric_limits<float>::max(), 
			-std::numeric_limits<float>::max() 
		};
	};


}