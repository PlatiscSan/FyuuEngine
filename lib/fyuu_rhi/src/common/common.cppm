module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <utility>
#endif // defined(__cpp_lib_modules)
export module fyuu_rhi:common;
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import plastic.registrable;
import plastic.disable_copy;

namespace fyuu_rhi {

	export template <class Derived> class RHICommon
		: public plastic::utility::Registrable<Derived>,
		public plastic::utility::DisableCopy {
	private:
		using RegistrableDerived = plastic::utility::Registrable<Derived>;

	public:
		RHICommon(Derived* derived)
			: RegistrableDerived(derived),
			DisableCopy() {

		}

		RHICommon(RHICommon&& other) noexcept = default;

	};

}