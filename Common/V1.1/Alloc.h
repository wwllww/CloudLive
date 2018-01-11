/********************************************************************************
 Copyright (C) 2001-2012 Hugh Bailey <BLive.jim@gmail.com>

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
********************************************************************************/


#pragma once

//========================================================
//Allocation class
class  Alloc
{
public:
    virtual ~Alloc()  {}

    virtual void * __restrict _Allocate(size_t dwSize)=0;
    virtual void * _ReAllocate(LPVOID lpData, size_t dwSize)=0;
    virtual void   _Free(LPVOID lpData)=0;
    virtual void   ErrorTermination()=0;

#ifdef _DEBUG
    inline void *operator new(size_t dwSize, TCHAR *lpFile, unsigned int lpLine)
    {
        return malloc(dwSize);
    }

    inline void operator delete(void *lpData, TCHAR *lpFile, unsigned int lpLine)
    {
        free(lpData);
    }
#endif

};


//========================================================
// extern   Alloc       *MainAllocator;

 extern unsigned int  dwAllocCurLine;
 extern TCHAR *       lpAllocCurFile;


 inline void Free(void *lpData)   { _aligned_free(lpData); }


//========================================================
#ifdef _DEBUG

    inline void* __restrict _debug_Allocate(size_t size, TCHAR *lpFile, unsigned int dwLine)                    {dwAllocCurLine = dwLine;lpAllocCurFile = lpFile;return MainAllocator->_Allocate(size);}
    inline void* _debug_ReAllocate(void* lpData, size_t size, TCHAR *lpFile, unsigned int dwLine)    {dwAllocCurLine = dwLine;lpAllocCurFile = lpFile;return MainAllocator->_ReAllocate(lpData, size);}

#define Allocate_Bak(size)              _debug_Allocate(size, TEXT(__FILE__), __LINE__)
    #define ReAllocate(lpData, size)    _debug_ReAllocate(lpData, size, TEXT(__FILE__), __LINE__)

//========================================================
#else //!_DEBUG

#define Allocate_Bak(size)             _aligned_malloc(size, 16);
#define ReAllocate(lpData, size)   ((!lpData) ? _aligned_malloc(size, 16) : _aligned_realloc(lpData, size, 16))

#endif


// inline void* operator new(size_t dwSize)
// {
// 	void* val = Allocate_Bak(dwSize);
//     zero(val, dwSize);
// 
//     return val;
// }
// 
// inline void operator delete(void* lpData)
// {
//     Free(lpData);
// }
// 
// inline void* operator new[](size_t dwSize)
// {
// 	void* val = Allocate_Bak(dwSize);
//     zero(val, dwSize);
// 
//     return val;
// }
// 
// inline void operator delete[](void* lpData)
// {
//     Free(lpData);
// }
