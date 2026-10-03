#include "stdafx.h"

/* -------------------------------------------------------------
   Внутренние константы
   ------------------------------------------------------------- */
#define THICK_LINE_INITIAL_CAPACITY     32
#define THICK_LINE_EPSILON              0.0001f

   /* -------------------------------------------------------------
      Внутренний буфер
      ------------------------------------------------------------- */
typedef struct _THICK_BUFFER
{
    PFLOAT              Points;
    PLINE_POINT_TYPE    Types;
    UINT                Capacity;
    UINT                Count;
} THICK_BUFFER, * PTHICK_BUFFER;

/* -------------------------------------------------------------
   Работа с буфером
   ------------------------------------------------------------- */
static
BOOL
GR_CALL
ThickBufferInit(
    _Out_ PTHICK_BUFFER Buffer
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
ThickBufferUninit(
    _Inout_ PTHICK_BUFFER Buffer
)
{
    if (Buffer->Points)
    {
        free(Buffer->Points);
        Buffer->Points = NULL;
    }
    if (Buffer->Types)
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
ThickBufferGrow(
    _Inout_ PTHICK_BUFFER Buffer,
    _In_ UINT             MinCapacity
)
{
    UINT newCapacity;
    PFLOAT newPoints;
    PLINE_POINT_TYPE newTypes;

    if (MinCapacity <= Buffer->Capacity)
        return TRUE;

    newCapacity = Buffer->Capacity == 0 ? THICK_LINE_INITIAL_CAPACITY : Buffer->Capacity * 2;
    while (newCapacity < MinCapacity)
        newCapacity *= 2;

    newPoints = (PFLOAT)realloc(Buffer->Points, newCapacity * 2 * sizeof(FLOAT));
    if (!newPoints)
        return FALSE;
    Buffer->Points = newPoints;

    newTypes = (PLINE_POINT_TYPE)realloc(Buffer->Types, newCapacity * sizeof(LINE_POINT_TYPE));
    if (!newTypes)
        return FALSE;
    Buffer->Types = newTypes;

    Buffer->Capacity = newCapacity;
    return TRUE;
}

static
BOOL
GR_CALL
ThickBufferAppendPoint(
    _Inout_ PTHICK_BUFFER   Buffer,
    _In_ FLOAT              X,
    _In_ FLOAT              Y,
    _In_ LINE_POINT_TYPE    Type
)
{
    if (!ThickBufferGrow(Buffer, Buffer->Count + 1))
        return FALSE;

    Buffer->Points[Buffer->Count * 2] = X;
    Buffer->Points[Buffer->Count * 2 + 1] = Y;
    Buffer->Types[Buffer->Count] = Type;
    Buffer->Count++;

    return TRUE;
}

/* -------------------------------------------------------------
   Вспомогательные геометрические функции
   ------------------------------------------------------------- */
static
FLOAT
GR_CALL
ThickVecLength(
    _In_ FLOAT X,
    _In_ FLOAT Y
)
{
    return sqrtf(X * X + Y * Y);
}

static
VOID
GR_CALL
ThickNormalize(
    _Inout_ PFLOAT pX,
    _Inout_ PFLOAT pY
)
{
    FLOAT len = ThickVecLength(*pX, *pY);
    if (len > THICK_LINE_EPSILON)
    {
        *pX /= len;
        *pY /= len;
    }
    else
    {
        *pX = 0.0f;
        *pY = 0.0f;
    }
}

/* -------------------------------------------------------------
   Смещение точки по нормали
   ------------------------------------------------------------- */
static
VOID
GR_CALL
ThickOffsetPoint(
    _In_ FLOAT      X,
    _In_ FLOAT      Y,
    _In_ FLOAT      Nx,
    _In_ FLOAT      Ny,
    _In_ FLOAT      Distance,
    _Out_ PFLOAT    pOutX,
    _Out_ PFLOAT    pOutY
)
{
    *pOutX = X + Nx * Distance;
    *pOutY = Y + Ny * Distance;
}

/* -------------------------------------------------------------
   Обработка одного subpath — построение толстого контура
   ------------------------------------------------------------- */
static
BOOL
GR_CALL
ThickLineProcessSubPath(
    _In_ PSLineItem         LineItem,
    _In_ UINT               StartIdx,
    _In_ UINT               EndIdx,
    _In_ BOOL               isClosed,
    _In_ FLOAT              Thickness,
    _In_ THICK_LINE_JOIN    JoinStyle,
    _In_ THICK_LINE_CAP     CapStyle,
    _In_ FLOAT              MiterLimit,
    _Inout_ PTHICK_BUFFER   Buffer
)
{
    UINT i;
    FLOAT half = Thickness * 0.5f;
    UINT pointCount = EndIdx - StartIdx + 1;

    if (pointCount < 2)
        return TRUE;

    //----------------------------------------------------------
    // 1. Правая сторона (вперёд)
    //----------------------------------------------------------
    for (i = StartIdx; i <= EndIdx; ++i)
    {
        FLOAT x = LineItem->Points[i * 2];
        FLOAT y = LineItem->Points[i * 2 + 1];

        FLOAT nx = 0.0f, ny = 0.0f;

        // Считаем нормаль
        if (i < EndIdx)
        {
            FLOAT dx = LineItem->Points[(i + 1) * 2] - x;
            FLOAT dy = LineItem->Points[(i + 1) * 2 + 1] - y;
            nx = -dy;
            ny = dx;
            ThickNormalize(&nx, &ny);
        }
        else if (i > StartIdx)
        {
            // Последняя точка — берём направление предыдущего сегмента
            FLOAT dx = x - LineItem->Points[(i - 1) * 2];
            FLOAT dy = y - LineItem->Points[(i - 1) * 2 + 1];
            nx = -dy;
            ny = dx;
            ThickNormalize(&nx, &ny);
        }

        FLOAT ox, oy;
        ThickOffsetPoint(x, y, nx, ny, half, &ox, &oy);

        LINE_POINT_TYPE type = (i == StartIdx) ? LinePointType_Move : LinePointType_Line;

        if (!ThickBufferAppendPoint(Buffer, ox, oy, type))
            return FALSE;
    }

    //----------------------------------------------------------
    // 2. Левая сторона (назад)
    //----------------------------------------------------------
    for (i = EndIdx + 1; i-- > StartIdx; )
    {
        FLOAT x = LineItem->Points[i * 2];
        FLOAT y = LineItem->Points[i * 2 + 1];

        FLOAT nx = 0.0f, ny = 0.0f;

        if (i < EndIdx)
        {
            FLOAT dx = LineItem->Points[(i + 1) * 2] - x;
            FLOAT dy = LineItem->Points[(i + 1) * 2 + 1] - y;
            nx = -dy;
            ny = dx;
            ThickNormalize(&nx, &ny);
        }
        else if (i > StartIdx)
        {
            FLOAT dx = x - LineItem->Points[(i - 1) * 2];
            FLOAT dy = y - LineItem->Points[(i - 1) * 2 + 1];
            nx = -dy;
            ny = dx;
            ThickNormalize(&nx, &ny);
        }

        FLOAT ox, oy;
        ThickOffsetPoint(x, y, nx, ny, -half, &ox, &oy);

        if (!ThickBufferAppendPoint(Buffer, ox, oy, LinePointType_Line))
            return FALSE;
    }

    //----------------------------------------------------------
    // 3. Закрываем контур этого subpath
    //----------------------------------------------------------
    if (Buffer->Count > 0)
    {
        Buffer->Types[Buffer->Count - 1] =
            MAKE_CLOSE_POINT(Buffer->Types[Buffer->Count - 1]);
    }

    return TRUE;
}

/* -------------------------------------------------------------
   Основная функция построения геометрии толстой линии
   с поддержкой нескольких subpath’ов
   ------------------------------------------------------------- */
static
PSLineItem
GR_CALL
ThickLineBuildGeometry(
    _In_ PSLineItem         LineItem,
    _In_ FLOAT              Thickness,
    _In_ THICK_LINE_JOIN    JoinStyle,
    _In_ THICK_LINE_CAP     CapStyle,
    _In_ FLOAT              MiterLimit
)
{
    THICK_BUFFER buffer;
    PSLineItem result;
    UINT i;
    UINT subPathStart = 0;
    BOOL inSubPath = FALSE;

    if (!LineItem || LineItem->PointCount < 2)
        return NULL;

    if (!ThickBufferInit(&buffer))
        return NULL;

    for (i = 0; i < LineItem->PointCount; ++i)
    {
        BYTE rawType = LineItem->PointTypes[i];
        BYTE pointType = GET_POINT_TYPE(rawType);

        if (pointType == LinePointType_Move)
        {
            // Завершаем предыдущий subpath (если был)
            if (inSubPath)
            {
                if (!ThickLineProcessSubPath(
                    LineItem,
                    subPathStart,
                    i - 1,
                    FALSE,                  // не закрыт явно
                    Thickness,
                    JoinStyle,
                    CapStyle,
                    MiterLimit,
                    &buffer))
                {
                    ThickBufferUninit(&buffer);
                    return NULL;
                }
            }

            subPathStart = i;
            inSubPath = TRUE;
        }
        else if (IS_CLOSE_POINT(rawType))
        {
            if (inSubPath)
            {
                if (!ThickLineProcessSubPath(
                    LineItem,
                    subPathStart,
                    i,
                    TRUE,                   // закрыт
                    Thickness,
                    JoinStyle,
                    CapStyle,
                    MiterLimit,
                    &buffer))
                {
                    ThickBufferUninit(&buffer);
                    return NULL;
                }
                inSubPath = FALSE;
            }
        }
    }

    // Последний открытый subpath
    if (inSubPath)
    {
        BOOL closed = LineItem->ClosePath;

        // Дополнительно можно проверить последнюю точку
        if (IS_CLOSE_POINT(LineItem->PointTypes[LineItem->PointCount - 1]))
            closed = TRUE;

        if (!ThickLineProcessSubPath(
            LineItem,
            subPathStart,
            LineItem->PointCount - 1,
            closed,
            Thickness,
            JoinStyle,
            CapStyle,
            MiterLimit,
            &buffer))
        {
            ThickBufferUninit(&buffer);
            return NULL;
        }
    }

    // Создаём результирующий LineItem
    result = LineItemAllocate(buffer.Count, LineItemType_Flatten, FALSE);
    if (!result)
    {
        ThickBufferUninit(&buffer);
        return NULL;
    }

    if (buffer.Count > 0)
    {
        memcpy(result->Points, buffer.Points, buffer.Count * 2 * sizeof(FLOAT));
        memcpy(result->PointTypes, buffer.Types, buffer.Count * sizeof(LINE_POINT_TYPE));
    }

    ThickBufferUninit(&buffer);
    return result;
}

/* -------------------------------------------------------------
   Внутренняя функция создания
   ------------------------------------------------------------- */
static
PSThickLineItem
GR_CALL
ThickLineItemCreateInternal(
    _In_ PSLineItem         LineItem,
    _In_ FLOAT              Thickness,
    _In_ THICK_LINE_JOIN    JoinStyle,
    _In_ THICK_LINE_CAP     CapStyle,
    _In_ FLOAT              MiterLimit
)
{
    PSThickLineItem item;
    PSLineItem geometry;

    if (Thickness <= 0.0f)
        return NULL;

    geometry = ThickLineBuildGeometry(LineItem, Thickness, JoinStyle, CapStyle, MiterLimit);
    if (!geometry)
        return NULL;

    item = (PSThickLineItem)malloc(sizeof(SThickLineItem));
    if (!item)
    {
        LineItemFreeWithPoints(geometry);
        return NULL;
    }

    item->Thickness = Thickness;
    item->JoinStyle = JoinStyle;
    item->CapStyle = CapStyle;
    item->MiterLimit = MiterLimit;
    item->ThickGeometry = geometry;

    return item;
}

/* -------------------------------------------------------------
   ThickLineItemInit
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
)
{
    if (!LineItem || LineItem->Type != LineItemType_Flatten)
        return NULL;

    // Можно добавить проверку на наличие BezierControl при необходимости

    return ThickLineItemCreateInternal(LineItem, Thickness, JoinStyle, CapStyle, MiterLimit);
}

/* -------------------------------------------------------------
   ThickLineItemInitFromFlatten
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
)
{
    if (!FlattenLineItem || FlattenLineItem->Type != LineItemType_Flatten)
        return NULL;

    return ThickLineItemCreateInternal(FlattenLineItem, Thickness, JoinStyle, CapStyle, MiterLimit);
}

/* -------------------------------------------------------------
   ThickLineItemFree
   ------------------------------------------------------------- */
VOID
GR_CALL
ThickLineItemFree(
    _In_ PSThickLineItem ThickItem
)
{
    if (ThickItem)
    {
        if (ThickItem->ThickGeometry)
        {
            LineItemFreeWithPoints(ThickItem->ThickGeometry);
            ThickItem->ThickGeometry = NULL;
        }
        free(ThickItem);
    }
}

/* -------------------------------------------------------------
   Callback
   ------------------------------------------------------------- */
VOID
GR_CALL
ThickLineItemFreeCallback(
    _In_ PVOID StageData
)
{
    if (StageData)
        ThickLineItemFree((PSThickLineItem)StageData);
}
