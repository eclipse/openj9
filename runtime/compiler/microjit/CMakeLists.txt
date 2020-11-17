if(J9VM_OPT_MICROJIT)
	j9jit_files(
		microjit/ExceptionTable.cpp
	)
    if(TR_HOST_ARCH STREQUAL "x")
        if(TR_TARGER_ARCH STREQUAL "x")
	        add_subdirectory(x)
        endif()
    endif()
endif()