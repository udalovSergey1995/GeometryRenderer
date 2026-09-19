#pragma once

/* -------------------------------------------------------------
   Параметры dash pattern по умолчанию
   ------------------------------------------------------------- */
#define DASH_PATTERN_DEFAULT_COUNT  2

   /* -------------------------------------------------------------
      Dash pattern айтем: массив длин сегментов (череда dash/gap)
      + итоговый разбиеный LineItem с разрывами
      ------------------------------------------------------------- */
typedef struct _DashPatternItem
{
    _Field_size_(DashCount) PFLOAT  DashLengths;
    UINT                            DashCount;
    FLOAT                           DashOffset;
    PSLineItem                      DashedLine;
} SDashPatternItem, * PSDashPatternItem;

/* -------------------------------------------------------------
   Создание dash pattern айтема из логического айтема.
   Логический айтем не должен содержать BezierControl точек,
   иначе вернётся NULL (ошибка).
   ------------------------------------------------------------- */
_Check_return_
_Ret_maybenull_
PSDashPatternItem
GR_CALL
DashPatternItemInit(
    _In_ PSLineItem     LineItem,
    _In_reads_(DashCount) PCFLOAT DashLengths,
    _In_ UINT           DashCount,
    _In_ FLOAT          DashOffset
);

/* -------------------------------------------------------------
   Создание dash pattern айтема из аппроксимированного айтема.
   Аппроксимированный айтем должен иметь Type == LineItemType_Flatten,
   иначе вернётся NULL (ошибка).
   ------------------------------------------------------------- */
_Check_return_
_Ret_maybenull_
PSDashPatternItem
GR_CALL
DashPatternItemInitFromFlatten(
    _In_ PSLineItem     FlattenLineItem,
    _In_reads_(DashCount) PCFLOAT DashLengths,
    _In_ UINT           DashCount,
    _In_ FLOAT          DashOffset
);

/* -------------------------------------------------------------
   Освобождение dash pattern айтема
   ------------------------------------------------------------- */
VOID
GR_CALL
DashPatternItemFree(
    _In_ PSDashPatternItem DashItem
);

/* -------------------------------------------------------------
   Callback для освобождения данных этапа в PathStages
   ------------------------------------------------------------- */
VOID
GR_CALL
DashPatternItemFreeCallback(
    _In_ PVOID StageData
);
