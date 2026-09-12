#ifndef _BASE_TYPES_H_
#define _BASE_TYPES_H_

/* -------------------------------------------------------------
   Базовые скаляры в стиле Win32
   ------------------------------------------------------------- */
typedef unsigned char    BYTE;
typedef BYTE    *PBYTE;
typedef unsigned char    UINT8;
typedef unsigned int    UINT;
typedef unsigned int    UINT32;
typedef int    INT;
typedef int    INT32;
typedef int             BOOL;
typedef float           FLOAT;
typedef INT            *PINT;
typedef FLOAT          *PFLOAT;
typedef const FLOAT    *PCFLOAT;
typedef void            VOID;
typedef VOID           *PVOID;
typedef const VOID     *PCVOID;

#ifndef FALSE
    #define FALSE       0
#endif

#ifndef TRUE
    #define TRUE        1
#endif

#ifndef CONST
    #define CONST       const
#endif

/* -------------------------------------------------------------
   Тип отдельной точки в пути (для подлиний и примитивов)
   ------------------------------------------------------------- */
typedef BYTE LINE_POINT_TYPE, * PLINE_POINT_TYPE;
typedef const LINE_POINT_TYPE* PCLINE_POINT_TYPE;

#define LinePointType_Move ((LINE_POINT_TYPE)0)    /* Начало новой подлинии      */
#define LinePointType_Line ((LINE_POINT_TYPE)1)    /* Отрезок линии до след.точки */
#define LinePointType_BezierControl ((LINE_POINT_TYPE)2)    /* Опорная точка Безье        */
#define LinePointType_Close ((LINE_POINT_TYPE)3)/* Замкнуть текущую подлинию  */

#endif /* _BASE_TYPES_H_ */
