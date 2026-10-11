#pragma once

/* Неперекрывающаяся трапеция; 0 < Height <= 1, координаты FLOAT.
   Все контуры обрабатываются вместе по Winding в координатах вывода. */
typedef struct _RasterSlab
{
    FLOAT Y, Height;
    FLOAT XLeftTop, XLeftBottom;
    FLOAT XRightTop, XRightBottom;
} SRasterSlab, *PSRasterSlab;

typedef struct _RasterSlabItem
{
    PSRasterSlab Slabs;
    UINT Count;
} SRasterSlabItem, *PSRasterSlabItem;

PSRasterSlabItem GR_CALL RasterSlabItemCreate(PSLineItem Line);
VOID GR_CALL RasterSlabItemFree(PVOID Data);
BOOL GR_CALL RasterSlabItemGetProperty(PSPathStageEntry Entry, EStageProps Property,
    PVOID Output, PINT Length);
/* Площадь пересечения с единичным пикселем [X,X+1] x [Y,Y+1]. */
GR_EXPORT FLOAT GR_CALL RasterSlabGetCoverage(PSPathStageEntry Entry, FLOAT X, FLOAT Y);
