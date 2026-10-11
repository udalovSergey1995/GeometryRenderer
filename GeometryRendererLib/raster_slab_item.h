#pragma once

/* Неперекрывающаяся трапеция; 0 < Height <= 1, координаты FLOAT.
   Все контуры обрабатываются вместе по Winding в координатах вывода. */
typedef struct _RasterSlab
{
    FLOAT Y;
    FLOAT Height;
    FLOAT XLeftTop;
    FLOAT XLeftBottom;
    FLOAT XRightTop;
    FLOAT XRightBottom;
} SRasterSlab, *PSRasterSlab;

typedef struct _RasterSlabItem
{
    PSRasterSlab Slabs;
    UINT Count;
} SRasterSlabItem, *PSRasterSlabItem;

_Check_return_
_Ret_maybenull_
PSRasterSlabItem
GR_CALL
RasterSlabItemCreate(
    _In_ PSLineItem Line
);
/* Буфер BGRA32 с premultiplied alpha, положительный Stride, цвет ARGB. */
BOOL
GR_CALL
RasterSlabItemDraw(
    _In_ PSRasterSlabItem Item,
    _Inout_ PBYTE Pixels,
    _In_ INT Width,
    _In_ INT Height,
    _In_ INT Stride,
    _In_ UINT Color
);
VOID
GR_CALL
RasterSlabItemFree(
    _In_opt_ PVOID Data
);
BOOL
GR_CALL
RasterSlabItemGetProperty(
    _In_ PSPathStageEntry Entry,
    _In_ EStageProps Property,
    _Maybenull_ PVOID Output,
    _Out_ PINT Length
);
/* Площадь пересечения с единичным пикселем [X,X+1] x [Y,Y+1]. */
GR_EXPORT
FLOAT
GR_CALL
RasterSlabGetCoverage(
    _In_ PSPathStageEntry Entry,
    _In_ FLOAT X,
    _In_ FLOAT Y
);
