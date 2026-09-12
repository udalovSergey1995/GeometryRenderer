#include "stdafx.h"

/* -------------------------------------------------------------
   Внутренние константы
   ------------------------------------------------------------- */
#define FLATTEN_MAX_DEPTH           16
#define FLATTEN_INITIAL_CAPACITY    16

/* -------------------------------------------------------------
   Внутренний буфер для накопления точек
   ------------------------------------------------------------- */
typedef struct _FLATTEN_BUFFER
{
    PFLOAT              Points;
    PLINE_POINT_TYPE    Types;
    UINT                Capacity;
    UINT                Count;
} FLATTEN_BUFFER, * PFLATTEN_BUFFER;

/* -------------------------------------------------------------
   Внутренние функции буфера
   ------------------------------------------------------------- */
static
BOOL
GR_CALL
FlattenBufferInit(
    _Out_ PFLATTEN_BUFFER Buffer
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
FlattenBufferUninit(
    _Inout_ PFLATTEN_BUFFER Buffer
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
FlattenBufferGrow(
    _Inout_ PFLATTEN_BUFFER Buffer,
    _In_ UINT               MinCapacity
)
{
    UINT newCapacity;
    PFLOAT newPoints;
    PLINE_POINT_TYPE newTypes;

    if (MinCapacity <= Buffer->Capacity)
    {
        return TRUE;
    }

    newCapacity = Buffer->Capacity == 0 ? FLATTEN_INITIAL_CAPACITY : Buffer->Capacity * 2;
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
FlattenBufferAppendPoint(
    _Inout_ PFLATTEN_BUFFER Buffer,
    _In_ FLOAT              X,
    _In_ FLOAT              Y,
    _In_ LINE_POINT_TYPE    Type
)
{
    if (!FlattenBufferGrow(Buffer, Buffer->Count + 1))
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
   Рекурсивное деление кубической Безье (de Casteljau)
   ------------------------------------------------------------- */
static
VOID
GR_CALL
FlattenBezierRecursive(
    _In_ FLOAT              P0x,
    _In_ FLOAT              P0y,
    _In_ FLOAT              P1x,
    _In_ FLOAT              P1y,
    _In_ FLOAT              P2x,
    _In_ FLOAT              P2y,
    _In_ FLOAT              P3x,
    _In_ FLOAT              P3y,
    _In_ FLOAT              Flatness,
    _In_ UINT               Depth,
    _Inout_ PFLATTEN_BUFFER Buffer
)
{
    FLOAT dx;
    FLOAT dy;
    FLOAT lenSq;
    FLOAT cross1;
    FLOAT cross2;
    FLOAT distSqMax;

    FLOAT m0x;
    FLOAT m0y;
    FLOAT m1x;
    FLOAT m1y;
    FLOAT m2x;
    FLOAT m2y;
    FLOAT m3x;
    FLOAT m3y;
    FLOAT m4x;
    FLOAT m4y;
    FLOAT m5x;
    FLOAT m5y;

    dx = P3x - P0x;
    dy = P3y - P0y;
    lenSq = dx * dx + dy * dy;

    cross1 = (P1x - P0x) * dy - (P1y - P0y) * dx;
    cross2 = (P2x - P0x) * dy - (P2y - P0y) * dx;

    if (cross1 < 0.0f)
    {
        cross1 = -cross1;
    }
    if (cross2 < 0.0f)
    {
        cross2 = -cross2;
    }

    distSqMax = cross1 > cross2 ? cross1 : cross2;
    distSqMax = distSqMax * distSqMax;

    if (Depth >= FLATTEN_MAX_DEPTH ||
        lenSq == 0.0f ||
        distSqMax <= Flatness * Flatness * lenSq)
    {
        FlattenBufferAppendPoint(Buffer, P3x, P3y, LinePointType_Line);
        return;
    }

    m0x = (P0x + P1x) * 0.5f;
    m0y = (P0y + P1y) * 0.5f;
    m1x = (P1x + P2x) * 0.5f;
    m1y = (P1y + P2y) * 0.5f;
    m2x = (P2x + P3x) * 0.5f;
    m2y = (P2y + P3y) * 0.5f;

    m3x = (m0x + m1x) * 0.5f;
    m3y = (m0y + m1y) * 0.5f;
    m4x = (m1x + m2x) * 0.5f;
    m4y = (m1y + m2y) * 0.5f;

    m5x = (m3x + m4x) * 0.5f;
    m5y = (m3y + m4y) * 0.5f;

    FlattenBezierRecursive(P0x, P0y, m0x, m0y, m3x, m3y, m5x, m5y,
        Flatness, Depth + 1, Buffer);
    FlattenBezierRecursive(m5x, m5y, m4x, m4y, m2x, m2y, P3x, P3y,
        Flatness, Depth + 1, Buffer);
}

/* -------------------------------------------------------------
   FlattenItemInitEx
   ------------------------------------------------------------- */
_Check_return_
_Ret_maybenull_
PSLineItem
GR_CALL
FlattenItemInitEx(
    _In_ PSLineItem LineItem,
    _In_ FLOAT      Flatness
)
{
    FLATTEN_BUFFER buffer;
    PSLineItem result;
    UINT i;

    if (LineItem == NULL)
    {
        return NULL;
    }

    if (LineItem->Type == LineItemType_Flatten)
    {
        result = LineItemAllocate(LineItem->PointCount, LineItemType_Flatten, LineItem->ClosePath);
        if (result == NULL)
        {
            return NULL;
        }

        memcpy(result->Points, LineItem->Points, LineItem->PointCount * 2 * sizeof(FLOAT));
        memcpy(result->PointTypes, LineItem->PointTypes, LineItem->PointCount * sizeof(LINE_POINT_TYPE));
        return result;
    }

    if (!FlattenBufferInit(&buffer))
    {
        return NULL;
    }

    for (i = 0; i < LineItem->PointCount; i++)
    {
        switch (LineItem->PointTypes[i])
        {
            case LinePointType_Move:
            {
                if (!FlattenBufferAppendPoint(&buffer,
                    LineItem->Points[i * 2],
                    LineItem->Points[i * 2 + 1],
                    LinePointType_Move))
                {
                    FlattenBufferUninit(&buffer);
                    return NULL;
                }
                break;
            }

            case LinePointType_Line:
            {
                if (!FlattenBufferAppendPoint(&buffer,
                    LineItem->Points[i * 2],
                    LineItem->Points[i * 2 + 1],
                    LinePointType_Line))
                {
                    FlattenBufferUninit(&buffer);
                    return NULL;
                }
                break;
            }

            case LinePointType_BezierControl:
            {
                FLOAT p0x;
                FLOAT p0y;
                FLOAT p1x;
                FLOAT p1y;
                FLOAT p2x;
                FLOAT p2y;
                FLOAT p3x;
                FLOAT p3y;

                if (i == 0 || i + 2 >= LineItem->PointCount)
                {
                    break;
                }

                if (LineItem->PointTypes[i + 1] != LinePointType_BezierControl)
                {
                    if (!FlattenBufferAppendPoint(&buffer,
                        LineItem->Points[i * 2],
                        LineItem->Points[i * 2 + 1],
                        LinePointType_Line))
                    {
                        FlattenBufferUninit(&buffer);
                        return NULL;
                    }
                    break;
                }

                p0x = LineItem->Points[(i - 1) * 2];
                p0y = LineItem->Points[(i - 1) * 2 + 1];
                p1x = LineItem->Points[i * 2];
                p1y = LineItem->Points[i * 2 + 1];
                p2x = LineItem->Points[(i + 1) * 2];
                p2y = LineItem->Points[(i + 1) * 2 + 1];
                p3x = LineItem->Points[(i + 2) * 2];
                p3y = LineItem->Points[(i + 2) * 2 + 1];

                FlattenBezierRecursive(p0x, p0y, p1x, p1y, p2x, p2y, p3x, p3y,
                    Flatness, 0, &buffer);

                i += 2;
                break;
            }

            case LinePointType_Close:
            {
                if (!FlattenBufferAppendPoint(&buffer,
                    LineItem->Points[i * 2],
                    LineItem->Points[i * 2 + 1],
                    LinePointType_Close))
                {
                    FlattenBufferUninit(&buffer);
                    return NULL;
                }
                break;
            }

            default:
                break;
        }
    }

    result = LineItemAllocate(buffer.Count, LineItemType_Flatten, LineItem->ClosePath);
    if (result == NULL)
    {
        FlattenBufferUninit(&buffer);
        return NULL;
    }

    memcpy(result->Points, buffer.Points, buffer.Count * 2 * sizeof(FLOAT));
    memcpy(result->PointTypes, buffer.Types, buffer.Count * sizeof(LINE_POINT_TYPE));

    FlattenBufferUninit(&buffer);

    return result;
}

/* -------------------------------------------------------------
   FlattenItemInit
   ------------------------------------------------------------- */
_Check_return_
_Ret_maybenull_
PSLineItem
GR_CALL
FlattenItemInit(
    _In_ PSLineItem LineItem
)
{
    return FlattenItemInitEx(LineItem, FLATTEN_DEFAULT_TOLERANCE);
}

/* -------------------------------------------------------------
   Callback для интеграции с PathStages
   ------------------------------------------------------------- */
VOID
GR_CALL
FlattenItemFree(
    _In_ PVOID StageData
)
{
    if (StageData != NULL)
    {
        LineItemFreeWithPoints((PSLineItem)StageData);
    }
}