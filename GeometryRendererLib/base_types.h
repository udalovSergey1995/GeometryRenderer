#pragma once

/* -------------------------------------------------------------
   Базовые скаляры в стиле Win32
   ------------------------------------------------------------- */
typedef unsigned char BYTE;
typedef BYTE          *PBYTE;
typedef unsigned char UINT8;
typedef unsigned int  UINT;
typedef unsigned int  UINT32;
typedef int           INT;
typedef int           INT32;
typedef int           BOOL;
typedef float         FLOAT;
typedef INT           *PINT;
typedef UINT          *PUINT;
typedef FLOAT         *PFLOAT;
typedef const FLOAT   *PCFLOAT;
                      
typedef void          VOID;
typedef VOID          *PVOID;
typedef const VOID    *PCVOID;

#ifndef FALSE
    #define FALSE     0
#endif

#ifndef TRUE
    #define TRUE      1
#endif

#ifndef CONST
    #define CONST     const
#endif

/* -------------------------------------------------------------
   Тип отдельной точки в пути (для подлиний и примитивов)
   ------------------------------------------------------------- */
typedef BYTE LINE_POINT_TYPE, * PLINE_POINT_TYPE;
typedef const LINE_POINT_TYPE* PCLINE_POINT_TYPE;

#define GDI_PLUS




#ifndef GDI_PLUS

#define LinePointType_Move ((LINE_POINT_TYPE)0x0)    /* Начало новой подлинии      */
#define LinePointType_Line ((LINE_POINT_TYPE)0x1)    /* Отрезок линии до след.точки */
#define LinePointType_BezierControl ((LINE_POINT_TYPE)0x2)    /* Опорная точка Безье */
#define LinePointType_Close ((LINE_POINT_TYPE)0x4)/* Замкнуть текущую подлинию  */

// Маска типа (нижние 2 бита)
#define LinePointType_TypeMask      ((LINE_POINT_TYPE)0x02)

// Удобные макросы
#define GET_POINT_TYPE(t)           ((t) & LinePointType_TypeMask)
#define IS_CLOSE_POINT(t)           (((t) & LinePointType_Close) != 0)
#define MAKE_CLOSE_POINT(t)         ((LINE_POINT_TYPE)((t) | LinePointType_Close))

#else

// Основные типы (совпадают с GDI+)
#define LinePointType_Move          ((LINE_POINT_TYPE)0x00)  // Start
#define LinePointType_Line          ((LINE_POINT_TYPE)0x01)  // Line
#define LinePointType_BezierControl ((LINE_POINT_TYPE)0x03)  // Bezier (в GDI+ именно 3!)
// Флаги
#define LinePointType_Close         ((LINE_POINT_TYPE)0x80)  // CloseSubpath
                                  // Маска типа (нижние 3 бита)
#define LinePointType_TypeMask      ((LINE_POINT_TYPE)0x07)

// Удобные макросы
#define GET_POINT_TYPE(t)           ((t) & LinePointType_TypeMask)
#define IS_CLOSE_POINT(t)           (((t) & LinePointType_Close) != 0)
#define MAKE_CLOSE_POINT(t)         ((LINE_POINT_TYPE)((t) | LinePointType_Close))

#endif
