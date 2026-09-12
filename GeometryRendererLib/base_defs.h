#pragma once
/*=============================================================================
 *  SAL2 аннотации (fallback если нет <sal.h>)
 *===========================================================================*/

#ifndef _In_
#define _In_
#endif

#ifndef _In_opt_
#define _In_opt_
#endif

#ifndef _Out_
#define _Out_
#endif

#ifndef _Outptr_
#define _Outptr_
#endif

#ifndef _Inout_
#define _Inout_
#endif

#ifndef _Check_return_
#define _Check_return_
#endif

#ifndef _Success_
#define _Success_(expr)
#endif

#ifndef _Ret_maybenull_
#define _Ret_maybenull_
#endif

#ifndef _Ret_notnull_
#define _Ret_notnull_
#endif

#ifndef _Post_writable_byte_size_
#define _Post_writable_byte_size_(size)
#endif

#ifndef _Field_size_
#define _Field_size_(size)
#endif

#ifndef _In_reads_
#define _In_reads_(size)
#endif

#ifndef _In_reads_opt_
#define _In_reads_opt_(size)
#endif

/*=============================================================================
 *  Соглашение о вызовах
 *===========================================================================*/

#ifndef GR_CALL
    #if defined(_WIN32) && !defined(_WIN64)
        #define GR_CALL __stdcall
    #else
        #define GR_CALL
    #endif
#endif

#ifndef GR_EXPORT
    #define GR_EXPORT __declspec(dllexport)
#endif

/*=============================================================================
 *  Макрос CONTAINING_RECORD
 *===========================================================================*/

#define CONTAINING_RECORD(Address, Type, Field) ((Type*)((UINT8*)(Address) - offsetof(Type, Field)))
