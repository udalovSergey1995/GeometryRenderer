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
   Вспомогательные функции
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

static
VOID
GR_CALL
ThickPerp(
    _In_ FLOAT dx, _In_ FLOAT dy,
    _Out_ PFLOAT pNx, _Out_ PFLOAT pNy
)
{
    *pNx = -dy;
    *pNy = dx;
    ThickNormalize(pNx, pNy);
}

static
FLOAT
GR_CALL
ThickCross(
    _In_ FLOAT ax, _In_ FLOAT ay,
    _In_ FLOAT bx, _In_ FLOAT by
)
{
    return ax * by - ay * bx;
}

static
FLOAT
GR_CALL
ThickDot(
    _In_ FLOAT ax, _In_ FLOAT ay,
    _In_ FLOAT bx, _In_ FLOAT by
)
{
    return ax * bx + ay * by;
}

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
   Добавление круглой крышки (Cap Round)
   ------------------------------------------------------------- */
static
BOOL
GR_CALL
ThickAddRoundCap(
    _Inout_ PTHICK_BUFFER Buffer,
    _In_ FLOAT cx, _In_ FLOAT cy,       // центр
    _In_ FLOAT nx, _In_ FLOAT ny,       // направление наружу
    _In_ FLOAT radius,
    _In_ BOOL  startFromLeft            // направление обхода
)
{
    const int segments = 8;
    FLOAT angleStart, angleStep;
    int i;

    // Угол нормали
    FLOAT baseAngle = atan2f(ny, nx);

    if (startFromLeft)
    {
        angleStart = baseAngle + (FLOAT)(3.14159265f * 0.5f);
        angleStep = (FLOAT)(3.14159265f / segments);
    }
    else
    {
        angleStart = baseAngle - (FLOAT)(3.14159265f * 0.5f);
        angleStep = -(FLOAT)(3.14159265f / segments);
    }

    for (i = 1; i < segments; ++i)
    {
        FLOAT a = angleStart + angleStep * i;
        FLOAT x = cx + cosf(a) * radius;
        FLOAT y = cy + sinf(a) * radius;

        if (!ThickBufferAppendPoint(Buffer, x, y, LinePointType_Line))
            return FALSE;
    }

    return TRUE;
}

/* Пересечение двух прямых, заданных точкой + направлением */
static
BOOL
GR_CALL
ThickLineIntersect(
    _In_ FLOAT p1x, _In_ FLOAT p1y,
    _In_ FLOAT d1x, _In_ FLOAT d1y,
    _In_ FLOAT p2x, _In_ FLOAT p2y,
    _In_ FLOAT d2x, _In_ FLOAT d2y,
    _Out_ PFLOAT pOutX, _Out_ PFLOAT pOutY
)
{
    FLOAT det = d1x * d2y - d1y * d2x;

    if (fabsf(det) < THICK_LINE_EPSILON)
        return FALSE;   // параллельны

    FLOAT dx = p2x - p1x;
    FLOAT dy = p2y - p1y;

    FLOAT t = (dx * d2y - dy * d2x) / det;

    *pOutX = p1x + d1x * t;
    *pOutY = p1y + d1y * t;
    return TRUE;
}

/* -------------------------------------------------------------
   Добавление круглой крышки / join (дуга)
   ------------------------------------------------------------- */
static
BOOL
GR_CALL
ThickAddArc(
    _Inout_ PTHICK_BUFFER Buffer,
    _In_ FLOAT cx, _In_ FLOAT cy,
    _In_ FLOAT fromX, _In_ FLOAT fromY,
    _In_ FLOAT toX, _In_ FLOAT toY,
    _In_ FLOAT radius,
    _In_ BOOL  clockwise
)
{
    FLOAT a1 = atan2f(fromY - cy, fromX - cx);
    FLOAT a2 = atan2f(toY - cy, toX - cx);

    FLOAT da = a2 - a1;

    if (clockwise)
    {
        if (da > 0.0f) da -= 2.0f * 3.14159265f;
    }
    else
    {
        if (da < 0.0f) da += 2.0f * 3.14159265f;
    }

    int segments = (int)(fabsf(da) / (3.14159265f / 8.0f)) + 1;
    if (segments < 2) segments = 2;
    if (segments > 16) segments = 16;

    for (int i = 1; i < segments; ++i)
    {
        FLOAT t = (FLOAT)i / (FLOAT)segments;
        FLOAT a = a1 + da * t;

        FLOAT x = cx + cosf(a) * radius;
        FLOAT y = cy + sinf(a) * radius;

        if (!ThickBufferAppendPoint(Buffer, x, y, LinePointType_Line))
            return FALSE;
    }

    return TRUE;
}

/* -------------------------------------------------------------
   Основная функция обработки одного subpath
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
    UINT count = EndIdx - StartIdx + 1;

    if (count < 2)
        return TRUE;

    //----------------------------------------------------------
    // Временные массивы
    //----------------------------------------------------------
    PFLOAT px = (PFLOAT)malloc(count * sizeof(FLOAT));
    PFLOAT py = (PFLOAT)malloc(count * sizeof(FLOAT));
    PFLOAT nx = (PFLOAT)malloc(count * sizeof(FLOAT)); // нормали сегментов
    PFLOAT ny = (PFLOAT)malloc(count * sizeof(FLOAT));

    if (!px || !py || !nx || !ny)
    {
        free(px); free(py); free(nx); free(ny);
        return FALSE;
    }

    // Копируем точки subpath
    for (i = 0; i < count; ++i)
    {
        px[i] = LineItem->Points[(StartIdx + i) * 2];
        py[i] = LineItem->Points[(StartIdx + i) * 2 + 1];
    }

    // Считаем нормали сегментов
    for (i = 0; i < count - 1; ++i)
    {
        FLOAT dx = px[i + 1] - px[i];
        FLOAT dy = py[i + 1] - py[i];
        ThickPerp(dx, dy, &nx[i], &ny[i]);
    }
    // Для последней точки копируем предыдущую нормаль
    if (count >= 2)
    {
        nx[count - 1] = nx[count - 2];
        ny[count - 1] = ny[count - 2];
    }

    //----------------------------------------------------------
    // Правая сторона
    //----------------------------------------------------------
    for (i = 0; i < count; ++i)
    {
        FLOAT rx = px[i] + nx[i] * half;
        FLOAT ry = py[i] + ny[i] * half;

        LINE_POINT_TYPE type = (i == 0) ? LinePointType_Move : LinePointType_Line;

        // --- Join ---
        if (i > 0 && i < count - 1)
        {
            FLOAT n1x = nx[i - 1], n1y = ny[i - 1];
            FLOAT n2x = nx[i], n2y = ny[i];

            FLOAT cross = ThickCross(n1x, n1y, n2x, n2y);
            FLOAT dot = ThickDot(n1x, n1y, n2x, n2y);

            // Угол поворота
            BOOL isLeftTurn = (cross > 0.0f);

            if (JoinStyle == ThickLineJoin_Miter)
            {
                // Вычисляем точку miter
                FLOAT miterX, miterY;
                FLOAT d1x = -n1y, d1y = n1x;   // направление предыдущего сегмента
                FLOAT d2x = -n2y, d2y = n2x;

                FLOAT p1x = px[i] + n1x * half;
                FLOAT p1y = py[i] + n1y * half;
                FLOAT p2x = px[i] + n2x * half;
                FLOAT p2y = py[i] + n2y * half;

                if (ThickLineIntersect(p1x, p1y, d1x, d1y, p2x, p2y, d2x, d2y, &miterX, &miterY))
                {
                    // Проверяем MiterLimit
                    FLOAT mx = miterX - px[i];
                    FLOAT my = miterY - py[i];
                    FLOAT miterLen = ThickVecLength(mx, my);

                    if (miterLen <= half * MiterLimit)
                    {
                        // Используем miter-точку
                        rx = miterX;
                        ry = miterY;
                    }
                    // иначе падаем в Bevel (просто используем обычную точку)
                }
            }
            else if (JoinStyle == ThickLineJoin_Round)
            {
                // Сначала ставим точку предыдущего сегмента
                FLOAT prevRx = px[i] + n1x * half;
                FLOAT prevRy = py[i] + n1y * half;

                if (i == 1)
                    type = LinePointType_Move;

                if (!ThickBufferAppendPoint(Buffer, prevRx, prevRy, type))
                {
                    free(px); free(py); free(nx); free(ny);
                    return FALSE;
                }

                // Добавляем дугу
                if (!ThickAddArc(Buffer, px[i], py[i],
                    prevRx, prevRy,
                    px[i] + n2x * half, py[i] + n2y * half,
                    half, !isLeftTurn))
                {
                    free(px); free(py); free(nx); free(ny);
                    return FALSE;
                }

                continue; // точку текущего сегмента добавим на следующей итерации логики
            }
        }

        if (!ThickBufferAppendPoint(Buffer, rx, ry, type))
        {
            free(px); free(py); free(nx); free(ny);
            return FALSE;
        }
    }

    //----------------------------------------------------------
    // Cap в конце (если открытый)
    //----------------------------------------------------------
    if (!isClosed && count >= 2)
    {
        FLOAT dx = px[count - 1] - px[count - 2];
        FLOAT dy = py[count - 1] - py[count - 2];
        FLOAT nlx, nly;
        ThickPerp(dx, dy, &nlx, &nly);

        if (CapStyle == ThickLineCap_Square)
        {
            FLOAT len = ThickVecLength(dx, dy);
            if (len > THICK_LINE_EPSILON)
            {
                dx /= len; dy /= len;
                FLOAT sx = px[count - 1] + dx * half;
                FLOAT sy = py[count - 1] + dy * half;

                if (!ThickBufferAppendPoint(Buffer, sx + nlx * half, sy + nly * half, LinePointType_Line) ||
                    !ThickBufferAppendPoint(Buffer, sx - nlx * half, sy - nly * half, LinePointType_Line))
                {
                    free(px); free(py); free(nx); free(ny);
                    return FALSE;
                }
            }
        }
        else if (CapStyle == ThickLineCap_Round)
        {
            FLOAT fromX = px[count - 1] + nlx * half;
            FLOAT fromY = py[count - 1] + nly * half;
            FLOAT toX = px[count - 1] - nlx * half;
            FLOAT toY = py[count - 1] - nly * half;

            if (!ThickAddArc(Buffer, px[count - 1], py[count - 1],
                fromX, fromY, toX, toY, half, FALSE))
            {
                free(px); free(py); free(nx); free(ny);
                return FALSE;
            }
        }
    }

    //----------------------------------------------------------
    // Левая сторона (в обратном порядке)
    //----------------------------------------------------------
    for (i = count; i-- > 0; )
    {
        FLOAT lx = px[i] - nx[i] * half;
        FLOAT ly = py[i] - ny[i] * half;

        if (!ThickBufferAppendPoint(Buffer, lx, ly, LinePointType_Line))
        {
            free(px); free(py); free(nx); free(ny);
            return FALSE;
        }
    }

    //----------------------------------------------------------
    // Cap в начале (если открытый)
    //----------------------------------------------------------
    if (!isClosed && count >= 2)
    {
        FLOAT dx = px[1] - px[0];
        FLOAT dy = py[1] - py[0];
        FLOAT nlx, nly;
        ThickPerp(dx, dy, &nlx, &nly);

        if (CapStyle == ThickLineCap_Square)
        {
            FLOAT len = ThickVecLength(dx, dy);
            if (len > THICK_LINE_EPSILON)
            {
                dx /= len; dy /= len;
                FLOAT sx = px[0] - dx * half;
                FLOAT sy = py[0] - dy * half;

                if (!ThickBufferAppendPoint(Buffer, sx - nlx * half, sy - nly * half, LinePointType_Line) ||
                    !ThickBufferAppendPoint(Buffer, sx + nlx * half, sy + nly * half, LinePointType_Line))
                {
                    free(px); free(py); free(nx); free(ny);
                    return FALSE;
                }
            }
        }
        else if (CapStyle == ThickLineCap_Round)
        {
            FLOAT fromX = px[0] - nlx * half;
            FLOAT fromY = py[0] - nly * half;
            FLOAT toX = px[0] + nlx * half;
            FLOAT toY = py[0] + nly * half;

            if (!ThickAddArc(Buffer, px[0], py[0],
                fromX, fromY, toX, toY, half, FALSE))
            {
                free(px); free(py); free(nx); free(ny);
                return FALSE;
            }
        }
    }

    //----------------------------------------------------------
    // Закрываем контур
    //----------------------------------------------------------
    if (Buffer->Count > 0)
    {
        Buffer->Types[Buffer->Count - 1] =
            MAKE_CLOSE_POINT(Buffer->Types[Buffer->Count - 1]);
    }

    free(px);
    free(py);
    free(nx);
    free(ny);

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

