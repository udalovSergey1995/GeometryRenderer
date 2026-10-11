#pragma once

/* -------------------------------------------------------------
   Параметры толстой линии по умолчанию
   ------------------------------------------------------------- */
#define THICK_LINE_DEFAULT_THICKNESS 1.0f
#define THICK_LINE_DEFAULT_MITER_LIMIT 10.0f
#define THICK_LINE_DEFAULT_ARC_TOLERANCE 0.1f
/* Превышение лимита означает отказ без снижения заданной точности. */
#define THICK_LINE_MAX_ARC_SEGMENTS 65536u
#define THICK_LINE_MAX_POINTS 1048576u

/* -------------------------------------------------------------
   Стили соединений (Join)
   ------------------------------------------------------------- */
typedef enum _THICK_LINE_JOIN
{
    ThickLineJoin_Miter = 0,
    ThickLineJoin_Round = 1,
    ThickLineJoin_Bevel = 2
} THICK_LINE_JOIN;

/* -------------------------------------------------------------
   Стили окончаний (Cap)
   ------------------------------------------------------------- */
typedef enum _THICK_LINE_CAP
{
    ThickLineCap_Flat = 0, // Butt
    ThickLineCap_Round = 1,
    ThickLineCap_Square = 2
} THICK_LINE_CAP;

/* -------------------------------------------------------------
   ThickLine айтем:
   - параметры штриха
   - результирующая геометрия (контур толстой линии)
   ------------------------------------------------------------- */
typedef struct _ThickLineItem
{
    FLOAT Thickness;
    THICK_LINE_JOIN JoinStyle;
    THICK_LINE_CAP CapStyle;
    FLOAT MiterLimit;

    PSLineItem ThickGeometry; // результат (обычно замкнутый контур)
    FLOAT ArcTolerance;       // допуск дуг в единицах координат пути
} SThickLineItem, *PSThickLineItem;

BOOL
GR_CALL
ThickLineItemGetProperty(
    _In_ PSPathStageEntry pEntry,
    _In_ EStageProps eProp,
    _Maybenull_ PVOID pOutData,
    _Out_ PINT pOutLen
);

/* -------------------------------------------------------------
   Создание ThickLine айтема из логического/аппроксимированного пути.
   Исходный LineItem не должен содержать BezierControl точек.
   ------------------------------------------------------------- */
_Check_return_ _Ret_maybenull_ PSThickLineItem GR_CALL
ThickLineItemInit(_In_ PSLineItem LineItem,
                  _In_ FLOAT Thickness,
                  _In_ THICK_LINE_JOIN JoinStyle,
                  _In_ THICK_LINE_CAP CapStyle,
                  _In_ FLOAT MiterLimit);

/* -------------------------------------------------------------
   Создание из уже аппроксимированного (Flatten) айтема
   ------------------------------------------------------------- */
_Check_return_ _Ret_maybenull_ PSThickLineItem GR_CALL
ThickLineItemInitFromFlatten(_In_ PSLineItem FlattenLineItem,
                             _In_ FLOAT Thickness,
                             _In_ THICK_LINE_JOIN JoinStyle,
                             _In_ THICK_LINE_CAP CapStyle,
                             _In_ FLOAT MiterLimit);

/* Контуры предназначены для Winding. NULL при ошибке или превышении лимитов. */
_Check_return_ _Ret_maybenull_ PSThickLineItem GR_CALL
ThickLineItemInitEx(_In_ PSLineItem LineItem,
                    _In_ FLOAT Thickness,
                    _In_ THICK_LINE_JOIN JoinStyle,
                    _In_ THICK_LINE_CAP CapStyle,
                    _In_ FLOAT MiterLimit,
                    _In_ FLOAT ArcTolerance);

_Check_return_ _Ret_maybenull_ PSThickLineItem GR_CALL
ThickLineItemInitFromFlattenEx(_In_ PSLineItem FlattenLineItem,
                               _In_ FLOAT Thickness,
                               _In_ THICK_LINE_JOIN JoinStyle,
                               _In_ THICK_LINE_CAP CapStyle,
                               _In_ FLOAT MiterLimit,
                               _In_ FLOAT ArcTolerance);

/* -------------------------------------------------------------
   Освобождение
   ------------------------------------------------------------- */
VOID
GR_CALL
ThickLineItemFree(
    _In_ PSThickLineItem ThickItem
);

/* -------------------------------------------------------------
   Callback для PathStages
   ------------------------------------------------------------- */
VOID
GR_CALL
ThickLineItemFreeCallback(
    _In_ PVOID StageData
);
