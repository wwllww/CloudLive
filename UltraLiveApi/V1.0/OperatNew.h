#ifndef OPERATNEW_H
#define OPERATNEW_H
#include <memory>
#include "SLiveManager.h"

#pragma warning(disable:4291)
//---------------------------------------------------------------
// жидиnew,new[],delete,delete[] 
//---------------------------------------------------------------
void* operator new (size_t size, const char* file, unsigned int line);
void* operator new[](size_t size, const char* file, unsigned int line);
void operator delete(void* ptr);
void operator delete[](void* ptr);

#define OPERATOR_NEW new(__FILE__, __LINE__)

#endif