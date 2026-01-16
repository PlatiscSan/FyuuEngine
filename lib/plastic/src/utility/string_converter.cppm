export module plastic.string_converter;
import std;

namespace plastic::utility {

    export class StringConverter {
    private:
        static bool IsBoolString(std::string const& str);

        static std::string ToLower(std::string const& str);

        static bool ConvertToBool(std::string const& str);

        static std::int64_t ConvertToInt(std::string const& str);

        static std::uint64_t ConvertToUint(std::string const& str);

        static double ConvertToFloat(std::string const& str);

    public:
        enum class ValueType {
            Bool,
            Int,
            Uint,
            Float,
            String
        };


        static ValueType InferType(std::string const& str);

        template <typename T>
        static T Convert(std::string const& str) {
            ValueType inferred = InferType(str);

            switch (inferred) {
            case ValueType::Bool:
                return static_cast<T>(ConvertToBool(str));
            case ValueType::Int:
                return static_cast<T>(ConvertToInt(str));
            case ValueType::Uint:
                return static_cast<T>(ConvertToUint(str));
            case ValueType::Float:
                return static_cast<T>(ConvertToFloat(str));
            default:
                throw std::invalid_argument("Cannot convert string to arithmetic type");
            }
        }

    };

}