#ifndef MJIT_UTILS_HPP
#define MJIT_UTILS_HPP

#include <cstdio>
#include <cassert>
#include "env/IO.hpp"

inline void MJIT_ASSERT(TR::FilePointer* filePtr, bool condition, const char * const error_string){
	if(!condition){
		trfprintf(filePtr, "%s", error_string);
		assert(condition);	
	}	
}

#endif
