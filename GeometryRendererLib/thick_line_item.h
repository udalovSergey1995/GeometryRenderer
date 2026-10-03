#pragma once

/* -------------------------------------------------------------
   Параметры толстой линии по умолчанию
   ------------------------------------------------------------- */
#define THICK_LINE_DEFAULT_THICKNESS    1.0f
#define THICK_LINE_DEFAULT_MITER_LIMIT  10.0f

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
    ThickLineCap_Flat = 0,    // Butt
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
    FLOAT               Thickness;
    THICK_LINE_JOIN     JoinStyle;
    THICK_LINE_CAP      CapStyle;
    FLOAT               MiterLimit;

    PSLineItem          ThickGeometry;   // результат (обычно замкнутый контур)
} SThickLineItem, * PSThickLineItem;

/* -------------------------------------------------------------
   Создание ThickLine айтема из логического/аппроксимированного пути.
   Исходный LineItem не должен содержать BezierControl точек.
   ------------------------------------------------------------- */
_Check_return_
_Ret_maybenull_
PSThickLineItem
GR_CALL
ThickLineItemInit(
    _In_ PSLineItem         LineItem,
    _In_ FLOAT              Thickness,
    _In_ THICK_LINE_JOIN    JoinStyle,
    _In_ THICK_LINE_CAP     CapStyle,
    _In_ FLOAT              MiterLimit
);

/* -------------------------------------------------------------
   Создание из уже аппроксимированного (Flatten) айтема
   ------------------------------------------------------------- */
_Check_return_
_Ret_maybenull_
PSThickLineItem
GR_CALL
ThickLineItemInitFromFlatten(
    _In_ PSLineItem         FlattenLineItem,
    _In_ FLOAT              Thickness,
    _In_ THICK_LINE_JOIN    JoinStyle,
    _In_ THICK_LINE_CAP     CapStyle,
    _In_ FLOAT              MiterLimit
);

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