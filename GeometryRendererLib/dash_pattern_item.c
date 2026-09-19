#include "stdafx.h"

/* -------------------------------------------------------------
   Внутренние константы
   ------------------------------------------------------------- */
#define DASH_PATTERN_INITIAL_CAPACITY   16
#define DASH_PATTERN_EPSILON            0.0001f

   /* -------------------------------------------------------------
      Внутренний буфер для накопления точек
      ------------------------------------------------------------- */
typedef struct _DASH_BUFFER
{
    PFLOAT              Points;
    PLINE_POINT_TYPE    Types;
    UINT                Capacity;
    UINT                Count;
} DASH_BUFFER, * PDASH_BUFFER;

/* -------------------------------------------------------------
   Внутренние функции буфера
   ------------------------------------------------------------- */
static
BOOL
GR_CALL
DashBufferInit(
    _Out_ PDASH_BUFFER Buffer
)
{
    Buffer->Points = NULL;
    Buffer->Types = NULL;
    Buffer->Capacity = 0;
    Buffer->Count = 0;
    return TRUE;
}

static
VOID
GR_CALL
DashBufferUninit(
    _Inout_ PDASH_BUFFER Buffer
)
{
    if (Buffer->Points != NULL)
    {
        free(Buffer->Points);
        Buffer->Points = NULL;
    }

    if (Buffer->Types != NULL)
    {
        free(Buffer->Types);
        Buffer->Types = NULL;
    }

    Buffer->Capacity = 0;
    Buffer->Count = 0;
}

static
BOOL
GR_CALL
DashBufferGrow(
    _Inout_ PDASH_BUFFER Buffer,
    _In_ UINT            MinCapacity
)
{
    UINT newCapacity;
    PFLOAT newPoints;
    PLINE_POINT_TYPE newTypes;

    if (MinCapacity <= Buffer->Capacity)
    {
        return TRUE;
    }

    newCapacity = Buffer->Capacity == 0 ? DASH_PATTERN_INITIAL_CAPACITY : Buffer->Capacity * 2;
    while (newCapacity < MinCapacity)
    {
        newCapacity *= 2;
    }

    newPoints = (PFLOAT)realloc(Buffer->Points, newCapacity * 2 * sizeof(FLOAT));
    if (newPoints == NULL)
    {
        return FALSE;
    }
    Buffer->Points = newPoints;

    newTypes = (PLINE_POINT_TYPE)realloc(Buffer->Types, newCapacity * sizeof(LINE_POINT_TYPE));
    if (newTypes == NULL)
    {
        return FALSE;
    }
    Buffer->Types = newTypes;

    Buffer->Capacity = newCapacity;
    return TRUE;
}

static
BOOL
GR_CALL
DashBufferAppendPoint(
    _Inout_ PDASH_BUFFER    Buffer,
    _In_ FLOAT              X,
    _In_ FLOAT              Y,
    _In_ LINE_POINT_TYPE    Type
)
{
    if (!DashBufferGrow(Buffer, Buffer->Count + 1))
    {
        return FALSE;
    }

    Buffer->Points[Buffer->Count * 2] = X;
    Buffer->Points[Buffer->Count * 2 + 1] = Y;
    Buffer->Types[Buffer->Count] = Type;
    Buffer->Count++;

    return TRUE;
}

/* -------------------------------------------------------------
   Проверка: содержит ли LineItem точки BezierControl
   ------------------------------------------------------------- */
static
BOOL
GR_CALL
DashPatternItemHasBezier(
    _In_ PSLineItem LineItem
)
{
    UINT i;

    if (LineItem == NULL)
    {
        return FALSE;
    }

    for (i = 0; i < LineItem->PointCount; i++)
    {
        if (LineItem->PointTypes[i] == LinePointType_BezierControl)
        {
            return TRUE;
        }
    }

    return FALSE;
}

/* -------------------------------------------------------------
   Вычисление длины отрезка
   ------------------------------------------------------------- */
static
FLOAT
GR_CALL
DashPatternSegmentLength(
    _In_ FLOAT X0,
    _In_ FLOAT Y0,
    _In_ FLOAT X1,
    _In_ FLOAT Y1
)
{
    FLOAT dx;
    FLOAT dy;

    dx = X1 - X0;
    dy = Y1 - Y0;

    return sqrtf(dx * dx + dy * dy);
}

/* -------------------------------------------------------------
   Вычисление общей длины всех dash/gap сегментов
   ------------------------------------------------------------- */
static
FLOAT
GR_CALL
DashPatternTotalLength(
    _In_reads_(DashCount) PCFLOAT DashLengths,
    _In_ UINT               DashCount
)
{
    FLOAT total;
    UINT i;

    total = 0.0f;

    for (i = 0; i < DashCount; i++)
    {
        total += DashLengths[i];
    }

    return total;
}

/* -------------------------------------------------------------
   Нормализация offset в пределах [0, TotalLength)
   ------------------------------------------------------------- */
static
FLOAT
GR_CALL
DashPatternNormalizeOffset(
    _In_ FLOAT Offset,
    _In_ FLOAT TotalLength
)
{
    FLOAT result;

    if (TotalLength <= 0.0f)
    {
        return 0.0f;
    }

    result = Offset;

    while (result < 0.0f)
    {
        result += TotalLength;
    }

    while (result >= TotalLength)
    {
        result -= TotalLength;
    }

    return result;
}

/* -------------------------------------------------------------
   Определение индекса dash-сегмента и остатка в нём для данного offset
   ------------------------------------------------------------- */
static
VOID
GR_CALL
DashPatternLocateOffset(
    _In_reads_(DashCount) PCFLOAT DashLengths,
    _In_ UINT DashCount,
    _In_ FLOAT Offset,
    _Out_ PUINT pSegmentIndex,
    _Out_ PFLOAT pSegmentRemain
)
{
    UINT i;
    FLOAT acc;

    acc = 0.0f;

    for (i = 0; i < DashCount; i++)
    {
        if (Offset < acc + DashLengths[i])
        {
            *pSegmentIndex = i;
            *pSegmentRemain = Offset - acc;
            return;
        }

        acc += DashLengths[i];
    }

    *pSegmentIndex = 0;
    *pSegmentRemain = 0.0f;
}

/* -------------------------------------------------------------
   Интерполяция точки на отрезке на расстоянии t от (X0,Y0)
   ------------------------------------------------------------- */
static
VOID
GR_CALL
DashPatternInterpolate(
    _In_ FLOAT  X0,
    _In_ FLOAT  Y0,
    _In_ FLOAT  X1,
    _In_ FLOAT  Y1,
    _In_ FLOAT  T,
    _Out_ PFLOAT pOutX,
    _Out_ PFLOAT pOutY
)
{
    *pOutX = X0 + (X1 - X0) * T;
    *pOutY = Y0 + (Y1 - Y0) * T;
}

/* -------------------------------------------------------------
   Разбиение одного отрезка по dash pattern.
   Текущее состояние (сегмент, остаток) передаётся и обновляется.
   ------------------------------------------------------------- */
static
BOOL
GR_CALL
DashPatternSplitSegment(
    _In_ FLOAT              X0,
    _In_ FLOAT              Y0,
    _In_ FLOAT              X1,
    _In_ FLOAT              Y1,
    _In_reads_(DashCount) PCFLOAT DashLengths,
    _In_ UINT               DashCount,
    _Inout_ PUINT           pSegmentIndex,
    _Inout_ PFLOAT          pSegmentRemain,
    _Inout_ PDASH_BUFFER    Buffer
)
{
    FLOAT segLen;
    FLOAT curX;
    FLOAT curY;
    FLOAT remainInSeg;
    FLOAT remainInDash;
    FLOAT step;
    UINT segIdx;
    BOOL isDash;
    FLOAT ix;
    FLOAT iy;

    segLen = DashPatternSegmentLength(X0, Y0, X1, Y1);

    if (segLen < DASH_PATTERN_EPSILON)
    {
        return TRUE;
    }

    curX = X0;
    curY = Y0;
    remainInSeg = segLen;
    segIdx = *pSegmentIndex;
    remainInDash = DashLengths[segIdx] - *pSegmentRemain;

    if (remainInDash < DASH_PATTERN_EPSILON)
    {
        segIdx = (segIdx + 1) % DashCount;
        remainInDash = DashLengths[segIdx];
    }

    isDash = ((segIdx % 2) == 0);

    while (remainInSeg > DASH_PATTERN_EPSILON)
    {
        step = remainInSeg < remainInDash ? remainInSeg : remainInDash;

        DashPatternInterpolate(X0, Y0, X1, Y1, step / segLen, &ix, &iy);

        if (isDash)
        {
            if (Buffer->Count == 0 ||
                Buffer->Types[Buffer->Count - 1] == LinePointType_Move ||
                Buffer->Types[Buffer->Count - 1] == LinePointType_Close)
            {
                if (!DashBufferAppendPoint(Buffer, curX, curY, LinePointType_Move))
                {
                    return FALSE;
                }
            }

            if (!DashBufferAppendPoint(Buffer, ix, iy, LinePointType_Line))
            {
                return FALSE;
            }
        }
        else
        {
            if (Buffer->Count > 0 &&
                Buffer->Types[Buffer->Count - 1] != LinePointType_Move &&
                Buffer->Types[Buffer->Count - 1] != LinePointType_Close)
            {
                if (!DashBufferAppendPoint(Buffer, curX, curY, LinePointType_Move))
                {
                    return FALSE;
                }
            }
        }

        curX += ix;
        curY += iy;
        remainInSeg -= step;
        remainInDash -= step;

        if (remainInDash <= DASH_PATTERN_EPSILON && remainInSeg > DASH_PATTERN_EPSILON)
        {
            segIdx = (segIdx + 1) % DashCount;
            remainInDash = DashLengths[segIdx];
            isDash = ((segIdx % 2) == 0);
        }
    }

    *pSegmentIndex = segIdx;

    if (remainInDash <= DASH_PATTERN_EPSILON)
    {
        *pSegmentRemain = 0.0f;
    }
    else
    {
        *pSegmentRemain = DashLengths[segIdx] - remainInDash;
    }

    return TRUE;
}

/* -------------------------------------------------------------
   Обработка одной подлинии (subpath) — разбиение всех её сегментов
   ------------------------------------------------------------- */
static
BOOL
GR_CALL
DashPatternProcessSubPath(
    _In_ PSLineItem         LineItem,
    _In_ UINT               StartIdx,
    _In_ UINT               EndIdx,
    _In_reads_(DashCount) PCFLOAT DashLengths,
    _In_ UINT               DashCount,
    _Inout_ PUINT           pSegmentIndex,
    _Inout_ PFLOAT          pSegmentRemain,
    _Inout_ PDASH_BUFFER    Buffer
)
{
    UINT i;
    FLOAT x0;
    FLOAT y0;
    FLOAT x1;
    FLOAT y1;

    if (StartIdx >= EndIdx)
    {
        return TRUE;
    }

    x0 = LineItem->Points[StartIdx * 2];
    y0 = LineItem->Points[StartIdx * 2 + 1];

    for (i = StartIdx + 1; i <= EndIdx; i++)
    {
        if (LineItem->PointTypes[i] == LinePointType_Move)
        {
            break;
        }

        x1 = LineItem->Points[i * 2];
        y1 = LineItem->Points[i * 2 + 1];

        if (!DashPatternSplitSegment(
            x0, y0, x1, y1,
            DashLengths, DashCount,
            pSegmentIndex, pSegmentRemain,
            Buffer))
        {
            return FALSE;
        }

        x0 = x1;
        y0 = y1;
    }

    return TRUE;
}

/* -------------------------------------------------------------
   Основная функция разбиения LineItem по dash pattern.
   Разбиение на подпути осуществляется за счёт типов точек:
   каждая новая dash-сегмент начинается с LinePointType_Move.
   ------------------------------------------------------------- */
static
PSLineItem
GR_CALL
DashPatternBuildDashedLine(
    _In_ PSLineItem         LineItem,
    _In_reads_(DashCount) PCFLOAT DashLengths,
    _In_ UINT               DashCount,
    _In_ FLOAT              DashOffset
)
{
    DASH_BUFFER buffer;
    PSLineItem result;
    FLOAT totalLen;
    FLOAT normOffset;
    UINT segIdx;
    FLOAT segRemain;
    UINT i;
    UINT subPathStart;
    BOOL inSubPath;

    if (LineItem == NULL || DashLengths == NULL || DashCount == 0)
    {
        return NULL;
    }

    totalLen = DashPatternTotalLength(DashLengths, DashCount);
    if (totalLen <= DASH_PATTERN_EPSILON)
    {
        return NULL;
    }

    normOffset = DashPatternNormalizeOffset(DashOffset, totalLen);
    DashPatternLocateOffset(DashLengths, DashCount, normOffset, &segIdx, &segRemain);

    if (!DashBufferInit(&buffer))
    {
        return NULL;
    }

    subPathStart = 0;
    inSubPath = FALSE;

    for (i = 0; i < LineItem->PointCount; i++)
    {
        if (LineItem->PointTypes[i] == LinePointType_Move)
        {
            if (inSubPath)
            {
                if (!DashPatternProcessSubPath(
                    LineItem, subPathStart, i - 1,
                    DashLengths, DashCount,
                    &segIdx, &segRemain,
                    &buffer))
                {
                    DashBufferUninit(&buffer);
                    return NULL;
                }
            }

            subPathStart = i;
            inSubPath = TRUE;
        }
        else if (LineItem->PointTypes[i] == LinePointType_Close)
        {
            if (inSubPath)
            {
                if (!DashPatternProcessSubPath(
                    LineItem, subPathStart, i,
                    DashLengths, DashCount,
                    &segIdx, &segRemain,
                    &buffer))
                {
                    DashBufferUninit(&buffer);
                    return NULL;
                }

                inSubPath = FALSE;
            }
        }
    }

    if (inSubPath)
    {
        if (!DashPatternProcessSubPath(
            LineItem, subPathStart, LineItem->PointCount - 1,
            DashLengths, DashCount,
            &segIdx, &segRemain,
            &buffer))
        {
            DashBufferUninit(&buffer);
            return NULL;
        }
    }

    result = LineItemAllocate(buffer.Count, LineItemType_Flatten, LineItem->ClosePath);
    if (result == NULL)
    {
        DashBufferUninit(&buffer);
        return NULL;
    }

    if (buffer.Count > 0)
    {
        memcpy(result->Points, buffer.Points, buffer.Count * 2 * sizeof(FLOAT));
        memcpy(result->PointTypes, buffer.Types, buffer.Count * sizeof(LINE_POINT_TYPE));
    }

    DashBufferUninit(&buffer);

    return result;
}

/* -------------------------------------------------------------
   Внутренняя функция создания DashPatternItem
   ------------------------------------------------------------- */
static
PSDashPatternItem
GR_CALL
DashPatternItemCreateInternal(
    _In_ PSLineItem         LineItem,
    _In_reads_(DashCount) PCFLOAT DashLengths,
    _In_ UINT               DashCount,
    _In_ FLOAT              DashOffset
)
{
    PSDashPatternItem dashItem;
    PSLineItem dashedLine;
    size_t dashSize;

    dashedLine = DashPatternBuildDashedLine(LineItem, DashLengths, DashCount, DashOffset);
    if (dashedLine == NULL)
    {
        return NULL;
    }

    for (size_t i = 0; i < dashedLine->PointCount; i+=2)
    {
        FLOAT X = dashedLine->Points[i];
        FLOAT Y = dashedLine->Points[i + 1];

        FLOAT t = X + Y;
    }

    dashItem = (PSDashPatternItem)malloc(sizeof(SDashPatternItem));
    if (dashItem == NULL)
    {
        LineItemFreeWithPoints(dashedLine);
        return NULL;
    }

    dashSize = (size_t)DashCount * sizeof(FLOAT);
    dashItem->DashLengths = (PFLOAT)malloc(dashSize);
    if (dashItem->DashLengths == NULL)
    {
        free(dashItem);
        LineItemFreeWithPoints(dashedLine);
        return NULL;
    }

    memcpy(dashItem->DashLengths, DashLengths, dashSize);
    dashItem->DashCount = DashCount;
    dashItem->DashOffset = DashOffset;
    dashItem->DashedLine = dashedLine;

    return dashItem;
}

/* -------------------------------------------------------------
   DashPatternItemInit — из логического айтема (без Bezier точек)
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
)
{
    if (LineItem == NULL || DashLengths == NULL || DashCount == 0)
    {
        return NULL;
    }

    if (LineItem->Type != LineItemType_Flatten)
    {
        return NULL;
    }

    if (DashPatternItemHasBezier(LineItem))
    {
        return NULL;
    }

    return DashPatternItemCreateInternal(LineItem, DashLengths, DashCount, DashOffset);
}

/* -------------------------------------------------------------
   DashPatternItemInitFromFlatten — из аппроксимированного айтема
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
)
{
    if (FlattenLineItem == NULL || DashLengths == NULL || DashCount == 0)
    {
        return NULL;
    }

    if (FlattenLineItem->Type != LineItemType_Flatten)
    {
        return NULL;
    }

    return DashPatternItemCreateInternal(FlattenLineItem, DashLengths, DashCount, DashOffset);
}

/* -------------------------------------------------------------
   DashPatternItemFree
   ------------------------------------------------------------- */
VOID
GR_CALL
DashPatternItemFree(
    _In_ PSDashPatternItem DashItem
)
{
    if (DashItem != NULL)
    {
        if (DashItem->DashLengths != NULL)
        {
            free(DashItem->DashLengths);
            DashItem->DashLengths = NULL;
        }

        if (DashItem->DashedLine != NULL)
        {
            LineItemFreeWithPoints(DashItem->DashedLine);
            DashItem->DashedLine = NULL;
        }

        DashItem->DashCount = 0;
        DashItem->DashOffset = 0.0f;

        free(DashItem);
    }
}

/* -------------------------------------------------------------
   Callback для интеграции с PathStages
   ------------------------------------------------------------- */
VOID
GR_CALL
DashPatternItemFreeCallback(
    _In_ PVOID StageData
)
{
    if (StageData != NULL)
    {
        DashPatternItemFree((PSDashPatternItem)StageData);
    }
}

