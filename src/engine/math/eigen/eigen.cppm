export module math.eigen;
export import math;
import <Eigen/Dense>;
import <Eigen/Geometry>;

import std;

namespace fyuu_engine::math {

    export template <class DerivedVector, std::size_t dimension> class BaseEigenVector
        : public BaseVector<DerivedVector> {
    public:
        using EigenType = Eigen::Matrix<float, dimension, 1>;
        using BaseVectorDerived = DerivedVector;

    protected:
        EigenType m_data;

        DerivedVector AddImpl(DerivedVector const& other) const {
            return DerivedVector(m_data + other.m_data);
        }

        DerivedVector SubImpl(DerivedVector const& other) const {
            return DerivedVector(m_data - other.m_data);
        }

        DerivedVector HadamardImpl(DerivedVector const& other) const {
            return DerivedVector(m_data.array() * other.m_data.array());
        }

        DerivedVector ScaleImpl(float scalar) const {
            return DerivedVector(m_data * scalar);
        }

        DerivedVector DivScaleImpl(float scalar) const {
            return DerivedVector(m_data / scalar);
        }

        float DotImpl(DerivedVector const& other) const {
            return m_data.dot(other.m_data);
        }

        DerivedVector CrossImpl(DerivedVector const& other) const {
            static_assert(dimension == 3, "Cross product is only defined for 3D vectors");
            return DerivedVector(m_data.cross(other.m_data));
        }

        DerivedVector NormalizeImpl() const {
            return DerivedVector(m_data.normalized());
        }

        float LengthImpl() const {
            return m_data.norm();
        }

        bool EqualsImpl(DerivedVector const& other) const {
            return m_data.isApprox(other.m_data);
        }

        static DerivedVector ZeroImpl() {
            return DerivedVector(EigenType::Zero());
        }

        static DerivedVector OneImpl() {
            return DerivedVector(EigenType::Ones());
        }

    public:
        BaseEigenVector() : m_data(EigenType::Zero()) {}

        explicit BaseEigenVector(EigenType const& eigen_vec) : m_data(eigen_vec) {}

        template <class... Args, typename = std::enable_if_t<sizeof...(Args) == dimension>>
        BaseEigenVector(Args... args) : m_data{ static_cast<float>(args)... } {}

        float* Data() noexcept { return m_data.data(); }
        float const* Data() const noexcept { return m_data.data(); }

        float& At(std::size_t index) { 
            return m_data(static_cast<Eigen::Index>(index));
        }

        float const& At(std::size_t index) const { 
            return m_data(static_cast<Eigen::Index>(index));
        }

        std::size_t Size() const { return dimension; }

        EigenType const& GetEigenType() const { return m_data; }

        EigenType& GetEigenType() { return m_data; }

    };

	export class EigenVector2 : public BaseEigenVector<EigenVector2, 2> {
	public:
		using Base = BaseEigenVector<EigenVector2, 2>;

		EigenVector2() = default;
		EigenVector2(EigenType const& eigen_vec)
			: Base(eigen_vec) {
		}
		EigenVector2(float x, float y)
			: Base(x, y) {
		}
	};

	export class EigenVector3 : public BaseEigenVector<EigenVector3, 3> {
	public:
		using Base = BaseEigenVector<EigenVector3, 3>;

		EigenVector3() = default;
		EigenVector3(EigenType const& eigen_vec) 
			: Base(eigen_vec) {}
		EigenVector3(float x, float y, float z) 
			: Base(x, y, z) {}
	};

	export class EigenVector4 : public BaseEigenVector<EigenVector4, 4> {
	public:
		using Base = BaseEigenVector<EigenVector4, 4>;

		EigenVector4() = default;
		EigenVector4(EigenType const& eigen_vec)
			: Base(eigen_vec) {
		}
		EigenVector4(float x, float y, float z, float t)
			: Base(x, y, z, t) {
		}
	};

    export template <class DerivedMatrix, std::size_t rows, std::size_t cols>  class BaseEigenMatrix
        : public BaseMatrix<DerivedMatrix> {
    public:
        using EigenType = Eigen::Matrix<float, rows, cols>;
        using BaseMatrixDerived = DerivedMatrix;

    protected:
        EigenType m_data;

        DerivedMatrix AddImpl(DerivedMatrix const& other) const {
            return DerivedMatrix(m_data + other.m_data);
        }

        DerivedMatrix SubImpl(DerivedMatrix const& other) const {
            return DerivedMatrix(m_data - other.m_data);
        }

        DerivedMatrix ScaleImpl(float scalar) const {
            return DerivedMatrix(m_data * scalar);
        }

        DerivedMatrix DivScaleImpl(float scalar) const {
            return DerivedMatrix(m_data / scalar);
        }

        DerivedMatrix MulImpl(DerivedMatrix const& other) const {
            return DerivedMatrix(m_data * other.m_data);
        }

        DerivedMatrix HadamardImpl(DerivedMatrix const& other) const {
            return DerivedMatrix(m_data.array() * other.m_data.array());
        }

        DerivedMatrix TransposeImpl() const {
            return DerivedMatrix(m_data.transpose());
        }

        DerivedMatrix InverseImpl() const {
            if constexpr (rows == cols) {
                return DerivedMatrix(m_data.inverse());
            }
            else {
                static_assert(rows == cols, "Inverse is only defined for square matrices");
                return DerivedMatrix();
            }
        }

        bool EqualsImpl(DerivedMatrix const& other) const {
            return m_data.isApprox(other.m_data);
        }

        static DerivedMatrix IdentityImpl() {
            if constexpr (rows == cols) {
                return DerivedMatrix(EigenType::Identity());
            }
            else {
                return DerivedMatrix();
            }
        }

        static DerivedMatrix ZeroImpl() {
            return DerivedMatrix(EigenType::Zero());
        }

    public:
        BaseEigenMatrix() : m_data(EigenType::Zero()) {}
        explicit BaseEigenMatrix(EigenType const& eigen_mat) : m_data(eigen_mat) {}

        float* Data() noexcept { return m_data.data(); }
        float const* Data() const noexcept { return m_data.data(); }

        float& At(std::size_t row, std::size_t col) {
            return m_data(static_cast<Eigen::Index>(row), static_cast<Eigen::Index>(col));
        }

        float const& At(std::size_t row, std::size_t col) const {
            return m_data(static_cast<Eigen::Index>(row), static_cast<Eigen::Index>(col));
        }

        EigenType const& GetEigenType() const { return m_data; }
        EigenType& GetEigenType() { return m_data; }

        std::size_t Rows() const { return rows; }
        std::size_t Cols() const { return cols; }
    };

    export class EigenMatrix3x3 : public BaseEigenMatrix<EigenMatrix3x3, 3, 3> {
    public:
        using Base = BaseEigenMatrix<EigenMatrix3x3, 3, 3>;
        using Base::Base;

        EigenMatrix3x3() = default;
        EigenMatrix3x3(EigenType const& eigen_mat) : Base(eigen_mat) {}
    };

    export class EigenMatrix4x4 : public BaseEigenMatrix<EigenMatrix4x4, 4, 4> {
    public:
        using Base = BaseEigenMatrix<EigenMatrix4x4, 4, 4>;
        using Base::Base;

        EigenMatrix4x4() = default;
        EigenMatrix4x4(EigenType const& eigen_mat) : Base(eigen_mat) {}
    };

    export class EigenQuaternion : public BaseQuaternion<EigenQuaternion> {
    public:
        using EigenType = Eigen::Quaternionf;

    protected:
        EigenType m_data;

        EigenQuaternion MulImpl(EigenQuaternion const& other) const {
            return EigenQuaternion(m_data * other.m_data);
        }

        EigenQuaternion ScaleImpl(float scalar) const {
            return EigenQuaternion(EigenType(m_data.coeffs() * scalar));
        }

        EigenQuaternion NormalizeImpl() const {
            return EigenQuaternion(m_data.normalized());
        }

        EigenQuaternion ConjugateImpl() const {
            return EigenQuaternion(m_data.conjugate());
        }

        EigenQuaternion InverseImpl() const {
            return EigenQuaternion(m_data.inverse());
        }

        template <class Matrix>
        Matrix ToMatrixImpl() const {
            if constexpr (std::is_same_v<Matrix, EigenMatrix3x3>) {
                return EigenMatrix3x3(m_data.toRotationMatrix());
            }
            else if constexpr (std::is_same_v<Matrix, EigenMatrix4x4>) {
                Eigen::Matrix4f mat = Eigen::Matrix4f::Identity();
                mat.block<3, 3>(0, 0) = m_data.toRotationMatrix();
                return EigenMatrix4x4(mat);
            }
        }

        static EigenQuaternion FromAxisAngleImpl(EigenVector3 const& axis, float angle) {
            return EigenQuaternion(Eigen::AngleAxisf(angle, axis.GetEigenType()));
        }

        static EigenQuaternion FromEulerAnglesImpl(float pitch, float yaw, float roll) {
            return EigenQuaternion(
                Eigen::AngleAxisf(roll, Eigen::Vector3f::UnitZ()) *
                Eigen::AngleAxisf(yaw, Eigen::Vector3f::UnitY()) *
                Eigen::AngleAxisf(pitch, Eigen::Vector3f::UnitX())
            );
        }

        static EigenQuaternion IdentityImpl() {
            return EigenQuaternion(EigenType::Identity());
        }

    public:
        EigenQuaternion() : m_data(EigenType::Identity()) {}
        explicit EigenQuaternion(EigenType const& eigen_quat) : m_data(eigen_quat) {}
        EigenQuaternion(float w, float x, float y, float z) : m_data(w, x, y, z) {}
        explicit EigenQuaternion(Eigen::AngleAxisf const& angle_axis) : m_data(angle_axis) {}

        EigenType const& GetEigenType() const { return m_data; }
        EigenType& GetEigenType() { return m_data; }

        EigenVector3 ToEulerAngles() const {
            auto euler = m_data.toRotationMatrix().eulerAngles(2, 1, 0);
            return EigenVector3(euler[2], euler[1], euler[0]); // pitch, yaw, roll
        }
    };

}
