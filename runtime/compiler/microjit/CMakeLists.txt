j9jit_files(
	microjit/ExceptionTable.cpp
	microjit/assembly/utils.nasm
)
if(TR_HOST_ARCH STREQUAL "x")
	if(TR_TARGET_ARCH STREQUAL "x")
		add_subdirectory(x)
	endif()
endif()