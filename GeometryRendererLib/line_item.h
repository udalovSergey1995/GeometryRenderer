#pragma once

/* -------------------------------------------------------------
   Тип геометрии айтема (flatten / bezier)
   ------------------------------------------------------------- */
typedef enum _LINE_ITEM_TYPE {
    LineItemType_Flatten = 0,
    LineItemType_Bezier  = 1
} LINE_ITEM_TYPE, *PLINE_ITEM_TYPE;

/* -------------------------------------------------------------
   Логический айтем: точки + их типы + метаданные
   ------------------------------------------------------------- */
typedef struct _LineItem 
{
    _Field_size_(PointCount * 2) PFLOAT Points;
    _Field_size_(PointCount) PLINE_POINT_TYPE PointTypes;
    UINT                PointCount;
    LINE_ITEM_TYPE      Type;
    BOOL                ClosePath;
} SLineItem, *PSLineItem;

/* -------------------------------------------------------------
   Алокация нового айтема вместе с собственными массивами
   ------------------------------------------------------------- */
_Check_return_
_Ret_maybenull_
PSLineItem
GR_CALL
LineItemAllocate(
    _In_ UINT           PointCount,
    _In_ LINE_ITEM_TYPE Type,
    _In_ BOOL           ClosePath
    );

/* -------------------------------------------------------------
   Инициализация существующей структуры внешними массивами.
   Точки и типы НЕ копируются — только захватываются указатели.
   ------------------------------------------------------------- */
_Check_return_
_Ret_maybenull_
BOOL
GR_CALL
LineItemInitWithPoints(
    _Inout_ PSLineItem      Item,
    _In_reads_(PointCount * 2) PCFLOAT Points,
    _In_reads_(PointCount) PCLINE_POINT_TYPE PointTypes,
    _In_ UINT               PointCount,
    _In_ LINE_ITEM_TYPE     Type,
    _In_ BOOL               ClosePath
    );

/* -------------------------------------------------------------
   Освобождение только самой структуры.
   Внешние массивы Points / PointTypes НЕ трогаются.
   ------------------------------------------------------------- */
VOID
GR_CALL
LineItemFree(
    _In_ PSLineItem  Item
    );

/* -------------------------------------------------------------
   Освобождение структуры вместе с её собственными массивами
   (выделенными внутри LineItemAllocate)
   ------------------------------------------------------------- */
VOID
GR_CALL
LineItemFreeWithPoints(
    _In_ PSLineItem  Item
    );

