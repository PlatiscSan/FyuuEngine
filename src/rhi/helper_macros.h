#pragma once

#define DECLARE_IMPLEMENTATION(TYPE) \
struct Fyuu##TYPE::Implementation {	\
	fyuu_rhi::##TYPE rhi_obj;	\
};

#ifdef DECLARE_FYUU_DESTROY
#undef DECLARE_FYUU_DESTROY
#endif // DECLARE_FYUU_DESTROY

#define DECLARE_FYUU_DESTROY(TYPE) EXTERN_C DLL_API void DLL_CALL FyuuDestroy##TYPE(Fyuu##TYPE* obj) NO_EXCEPT {	\
	delete obj->impl;	\
	obj->impl = nullptr;	\
}

#define CATCH_COMMON_EXCEPTION(OBJ) \
catch (std::bad_alloc const& ex) {	\
	OBJ->impl = nullptr;	\
	fyuu_engine::rhi::SetLastRHIErrorMessage(ex.what());	\
	return FyuuErrorCode::FyuuErrorCode_BadAllocation;	\
}	\
catch (std::exception const& ex) {	\
	delete OBJ->impl;	\
	OBJ->impl = nullptr;	\
	fyuu_engine::rhi::SetLastRHIErrorMessage(ex.what());	\
	return FyuuErrorCode::FyuuErrorCode_UnknownError;	\
}
