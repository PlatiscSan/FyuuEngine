#pragma once

#include <utility>
#include "export_macros.h"

#define DECLARE_FYUU_CLASS_C(NAME) typedef struct NAME { struct Implementation; struct Implementation* impl; } NAME;

#define DECLARE_FYUU_DESTROY(TYPE) EXTERN_C DLL_API void DLL_CALL FyuuDestroy##TYPE(Fyuu##TYPE* obj) NO_EXCEPT;

#if defined(__cplusplus)
namespace fyuu_engine {

	template <class Derived>
	struct DisableCopy {
		DisableCopy() = default;
		DisableCopy(DisableCopy const&) = delete;
		DisableCopy& operator=(DisableCopy const&) = delete;
	};

#define DECLARE_FYUU_CLASS(CLASS_NAME) struct CLASS_NAME : public DisableCopy<CLASS_NAME>, public Fyuu##CLASS_NAME

#define FYUU_CLASS_COMMON_PARTS(CLASS_NAME)	 \
		CLASS_NAME(CLASS_NAME&& other) noexcept {	\
			impl = std::exchange(other.impl, nullptr);	\
		}	\
		\
		CLASS_NAME& operator=(CLASS_NAME&& other) noexcept {	\
			if (this != &other) {	\
				FyuuDestroy##CLASS_NAME(this);	\
				impl = std::exchange(other.impl, nullptr);	\
			}	\
			return *this;	\
		}	\
		\
		~CLASS_NAME() noexcept {	\
		FyuuDestroy##CLASS_NAME(this);	\
		}
}
#endif // defined(__cplusplus)
