export module disable_copy;
import std;

export namespace util {

    template <class T>
    class DisableCopy {
    public:
        DisableCopy() = default;

        DisableCopy(DisableCopy const&) = delete;
        DisableCopy& operator=(DisableCopy const&) = delete;

        DisableCopy(DisableCopy&&) noexcept = default;
        DisableCopy& operator=(DisableCopy&&) noexcept = default;

        ~DisableCopy() noexcept = default;
    };

}