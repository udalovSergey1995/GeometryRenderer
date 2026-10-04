#include "stdafx.h"

/* -------------------------------------------------------------
   Внутренние константы
   ------------------------------------------------------------- */
#define THICK_LINE_INITIAL_CAPACITY     64
#define THICK_LINE_EPSILON              0.0001f
#define THICK_LINE_PI                   3.14159265358979f
#define THICK_LINE_ARC_TOLERANCE        0.1f    /* макс. отклонение дуги от окружности, px */

   /* -------------------------------------------------------------
      Внутренний буфер результирующего контура
      ------------------------------------------------------------- */
typedef struct _THICK_BUFFER
{
    PFLOAT              Points;
    PLINE_POINT_TYPE    Types;
    UINT                Capacity;
    UINT                Count;
} THICK_BUFFER, * PTHICK_BUFFER;

/* Точка subpath'а */
typedef struct _THICK_PT
{
    FLOAT X;
    FLOAT Y;
} THICK_PT, * PTHICK_PT;

/* -------------------------------------------------------------
   Работа с буфером
   ------------------------------------------------------------- */
static
VOID
GR_CALL
ThickBufferInit(
    _Out_ PTHICK_BUFFER Buffer
)
{
    Buffer->Points = NULL;
    Buffer->Types = NULL;
    Buffer->Capacity = 0;
    Buffer->Count = 0;
}

static
VOID
GR_CALL
ThickBufferUninit(
    _Inout_ PTHICK_BUFFER Buffer
)
{
    free(Buffer->Points);
    free(Buffer->Types);
    Buffer->Points = NULL;
    Buffer->Types = NULL;
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

    newPoints = (PFLOAT)malloc((size_t)newCapacity * 2 * sizeof(FLOAT));
    newTypes = (PLINE_POINT_TYPE)malloc((size_t)newCapacity * sizeof(LINE_POINT_TYPE));

    if (!newPoints || !newTypes)
    {
        free(newPoints);
        free(newTypes);
        return FALSE;   /* старые массивы остаются согласованными */
    }

    if (Buffer->Count)
    {
        memcpy(newPoints, Buffer->Points, (size_t)Buffer->Count * 2 * sizeof(FLOAT));
        memcpy(newTypes, Buffer->Types, (size_t)Buffer->Count * sizeof(LINE_POINT_TYPE));
    }

    free(Buffer->Points);
    free(Buffer->Types);

    Buffer->Points = newPoints;
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
    _In_ FLOAT              Y
)
{
    if (!ThickBufferGrow(Buffer, Buffer->Count + 1))
        return FALSE;

    Buffer->Points[Buffer->Count * 2] = X;
    Buffer->Points[Buffer->Count * 2 + 1] = Y;
    Buffer->Types[Buffer->Count] = LinePointType_Line;
    Buffer->Count++;

    return TRUE;
}

/* Завершить контур, начавшийся с индекса Start: Move в начале, Close в конце */
static
VOID
GR_CALL
ThickBufferFinishContour(
    _Inout_ PTHICK_BUFFER   Buffer,
    _In_ UINT               Start
)
{
    if (Buffer->Count <= Start)
        return;

    Buffer->Types[Start] = LinePointType_Move;
    Buffer->Types[Buffer->Count - 1] =
        MAKE_CLOSE_POINT(Buffer->Types[Buffer->Count - 1]);
}

/* -------------------------------------------------------------
   Вспомогательная математика
   ------------------------------------------------------------- */

   /* Единичный вектор направления P[i] -> P[j]. Точки заранее очищены от дублей. */
static
VOID
GR_CALL
ThickSegDir(
    _In_ const THICK_PT* a,
    _In_ const THICK_PT* b,
    _Out_ PFLOAT pDx,
    _Out_ PFLOAT pDy
)
{
    FLOAT dx = b->X - a->X;
    FLOAT dy = b->Y - a->Y;
    FLOAT len = sqrtf(dx * dx + dy * dy);

    if (len > THICK_LINE_EPSILON)
    {
        *pDx = dx / len;
        *pDy = dy / len;
    }
    else
    {
        *pDx = 1.0f;
        *pDy = 0.0f;
    }
}

static
FLOAT
GR_CALL
ThickSegLen(
    _In_ const THICK_PT* a,
    _In_ const THICK_PT* b
)
{
    FLOAT dx = b->X - a->X;
    FLOAT dy = b->Y - a->Y;
    return sqrtf(dx * dx + dy * dy);
}

/* Угловой шаг дуги радиуса r, обеспечивающий заданную точность */
static
FLOAT
GR_CALL
ThickArcStep(
    _In_ FLOAT Radius
)
{
    FLOAT step;

    if (Radius <= THICK_LINE_ARC_TOLERANCE)
        return THICK_LINE_PI / 2.0f;

    step = 2.0f * acosf(1.0f - THICK_LINE_ARC_TOLERANCE / Radius);

    if (step > THICK_LINE_PI / 4.0f)  step = THICK_LINE_PI / 4.0f;
    if (step < THICK_LINE_PI / 32.0f) step = THICK_LINE_PI / 32.0f;

    return step;
}

/* -------------------------------------------------------------
   Join в вершине P между входящим направлением d1 и исходящим d2.
   Строится смещение на ЛЕВУЮ сторону пути (нормаль n = (-dy, dx)).

   cross = d1 x d2:
     > 0  поворот налево  -> левая сторона внутренняя
     < 0  поворот направо -> левая сторона внешняя
   ------------------------------------------------------------- */
static
BOOL
GR_CALL
ThickEmitJoin(
    _Inout_ PTHICK_BUFFER   Buffer,
    _In_ const THICK_PT* P,
    _In_ FLOAT d1x, _In_ FLOAT d1y, _In_ FLOAT len1,
    _In_ FLOAT d2x, _In_ FLOAT d2y, _In_ FLOAT len2,
    _In_ FLOAT              Half,
    _In_ THICK_LINE_JOIN    JoinStyle,
    _In_ FLOAT              MiterLimit
)
{
    FLOAT n1x = -d1y, n1y = d1x;
    FLOAT n2x = -d2y, n2y = d2x;
    FLOAT cross = d1x * d2y - d1y * d2x;
    FLOAT dot = d1x * d2x + d1y * d2y;

    FLOAT ax = P->X + n1x * Half;
    FLOAT ay = P->Y + n1y * Half;
    FLOAT bx = P->X + n2x * Half;
    FLOAT by = P->Y + n2y * Half;

    /* --- Почти прямой участок: join не нужен --- */
    if (fabsf(cross) < 1e-6f && dot > 0.0f)
        return ThickBufferAppendPoint(Buffer, ax, ay);

    /* --- Внутренняя сторона поворота --- */
    if (cross > 0.0f)
    {
        FLOAT onePlusDot = 1.0f + dot;

        if (onePlusDot > 1e-4f)
        {
            /* Расстояние от вершины до точки пересечения смещённых прямых
               вдоль каждого из сегментов равно Half * tan(phi/2). Если оба
               сегмента не короче — пересечение лежит на них. */
            FLOAT need = Half * sqrtf((1.0f - dot) / onePlusDot);

            if (len1 >= need && len2 >= need)
            {
                FLOAT k = Half / onePlusDot;
                return ThickBufferAppendPoint(Buffer,
                    P->X + (n1x + n2x) * k,
                    P->Y + (n1y + n2y) * k);
            }
        }

        /* Короткие сегменты / разворот: проходим через вершину
           (при заливке Winding лишние петли попадают внутрь штриха) */
        return ThickBufferAppendPoint(Buffer, ax, ay)
            && ThickBufferAppendPoint(Buffer, P->X, P->Y)
            && ThickBufferAppendPoint(Buffer, bx, by);
    }

    /* --- Внешняя сторона поворота --- */
    if (JoinStyle == ThickLineJoin_Miter)
    {
        FLOAT onePlusDot = 1.0f + dot;

        if (onePlusDot > 1e-4f)
        {
            /* |miter| / Half = 1 / cos(phi/2) = sqrt(2 / (1 + dot)) */
            FLOAT ratio = sqrtf(2.0f / onePlusDot);

            if (ratio <= MiterLimit)
            {
                FLOAT k = Half / onePlusDot;
                return ThickBufferAppendPoint(Buffer,
                    P->X + (n1x + n2x) * k,
                    P->Y + (n1y + n2y) * k);
            }
        }
        /* лимит превышен -> bevel */
    }
    else if (JoinStyle == ThickLineJoin_Round)
    {
        FLOAT da = atan2f(cross, dot);              /* знаковый угол n1 -> n2 */
        FLOAT a0 = atan2f(n1y, n1x);
        FLOAT step = ThickArcStep(Half);
        int   steps = (int)ceilf(fabsf(da) / step);
        int   i;

        if (!ThickBufferAppendPoint(Buffer, ax, ay))
            return FALSE;

        for (i = 1; i < steps; ++i)
        {
            FLOAT a = a0 + da * ((FLOAT)i / (FLOAT)steps);
            if (!ThickBufferAppendPoint(Buffer,
                P->X + cosf(a) * Half,
                P->Y + sinf(a) * Half))
                return FALSE;
        }

        return ThickBufferAppendPoint(Buffer, bx, by);
    }

    /* Bevel (и fallback для Miter) */
    return ThickBufferAppendPoint(Buffer, ax, ay)
        && ThickBufferAppendPoint(Buffer, bx, by);
}

/* -------------------------------------------------------------
   Cap в конце открытого пути. Вызывается сразу после EmitSide:
   последняя точка буфера = P + n*Half, следующая точка (начало
   обратной стороны) = P - n*Half.
   (dx,dy) — единичное направление наружу.
   ------------------------------------------------------------- */
static
BOOL
GR_CALL
ThickEmitCap(
    _Inout_ PTHICK_BUFFER   Buffer,
    _In_ const THICK_PT* P,
    _In_ FLOAT dx, _In_ FLOAT dy,
    _In_ FLOAT              Half,
    _In_ THICK_LINE_CAP     CapStyle
)
{
    FLOAT nx = -dy, ny = dx;

    if (CapStyle == ThickLineCap_Square)
    {
        return ThickBufferAppendPoint(Buffer,
            P->X + nx * Half + dx * Half,
            P->Y + ny * Half + dy * Half)
            && ThickBufferAppendPoint(Buffer,
                P->X - nx * Half + dx * Half,
                P->Y - ny * Half + dy * Half);
    }

    if (CapStyle == ThickLineCap_Round)
    {
        /* От +n к -n через +d: угол убывает на PI */
        FLOAT a0 = atan2f(ny, nx);
        FLOAT step = ThickArcStep(Half);
        int   steps = (int)ceilf(THICK_LINE_PI / step);
        int   i;

        for (i = 1; i < steps; ++i)
        {
            FLOAT a = a0 - THICK_LINE_PI * ((FLOAT)i / (FLOAT)steps);
            if (!ThickBufferAppendPoint(Buffer,
                P->X + cosf(a) * Half,
                P->Y + sinf(a) * Half))
                return FALSE;
        }
    }

    return TRUE;    /* Flat: ничего добавлять не нужно */
}

/* -------------------------------------------------------------
   Смещение пути на левую сторону с join'ами во всех внутренних
   вершинах (для замкнутого пути — во всех вершинах).
   ------------------------------------------------------------- */
static
BOOL
GR_CALL
ThickEmitSide(
    _Inout_ PTHICK_BUFFER   Buffer,
    _In_reads_(N) const THICK_PT* Pts,
    _In_ UINT               N,
    _In_ BOOL               IsClosed,
    _In_ FLOAT              Half,
    _In_ THICK_LINE_JOIN    JoinStyle,
    _In_ FLOAT              MiterLimit
)
{
    UINT i;
    FLOAT d1x, d1y, d2x, d2y, len1, len2;

    if (!IsClosed)
    {
        /* Первая точка */
        ThickSegDir(&Pts[0], &Pts[1], &d1x, &d1y);
        if (!ThickBufferAppendPoint(Buffer, Pts[0].X - d1y * Half, Pts[0].Y + d1x * Half))
            return FALSE;

        /* Внутренние вершины */
        for (i = 1; i + 1 < N; ++i)
        {
            ThickSegDir(&Pts[i - 1], &Pts[i], &d1x, &d1y);
            ThickSegDir(&Pts[i], &Pts[i + 1], &d2x, &d2y);
            len1 = ThickSegLen(&Pts[i - 1], &Pts[i]);
            len2 = ThickSegLen(&Pts[i], &Pts[i + 1]);

            if (!ThickEmitJoin(Buffer, &Pts[i],
                d1x, d1y, len1, d2x, d2y, len2,
                Half, JoinStyle, MiterLimit))
                return FALSE;
        }

        /* Последняя точка */
        ThickSegDir(&Pts[N - 2], &Pts[N - 1], &d1x, &d1y);
        return ThickBufferAppendPoint(Buffer, Pts[N - 1].X - d1y * Half, Pts[N - 1].Y + d1x * Half);
    }

    /* Замкнутый путь: все вершины имеют join */
    for (i = 0; i < N; ++i)
    {
        UINT prev = (i + N - 1) % N;
        UINT next = (i + 1) % N;

        ThickSegDir(&Pts[prev], &Pts[i], &d1x, &d1y);
        ThickSegDir(&Pts[i], &Pts[next], &d2x, &d2y);
        len1 = ThickSegLen(&Pts[prev], &Pts[i]);
        len2 = ThickSegLen(&Pts[i], &Pts[next]);

        if (!ThickEmitJoin(Buffer, &Pts[i],
            d1x, d1y, len1, d2x, d2y, len2,
            Half, JoinStyle, MiterLimit))
            return FALSE;
    }

    return TRUE;
}

/* -------------------------------------------------------------
   Вырожденный subpath из одной точки: рисуется только при
   Round (круг) и Square (квадрат) cap.
   ------------------------------------------------------------- */
static
BOOL
GR_CALL
ThickEmitDot(
    _Inout_ PTHICK_BUFFER   Buffer,
    _In_ const THICK_PT* P,
    _In_ FLOAT              Half,
    _In_ THICK_LINE_CAP     CapStyle
)
{
    UINT start = Buffer->Count;

    if (CapStyle == ThickLineCap_Round)
    {
        FLOAT step = ThickArcStep(Half);
        int   steps = (int)ceilf(2.0f * THICK_LINE_PI / step);
        int   i;

        for (i = 0; i < steps; ++i)
        {
            FLOAT a = 2.0f * THICK_LINE_PI * ((FLOAT)i / (FLOAT)steps);
            if (!ThickBufferAppendPoint(Buffer,
                P->X + cosf(a) * Half,
                P->Y + sinf(a) * Half))
                return FALSE;
        }
    }
    else if (CapStyle == ThickLineCap_Square)
    {
        if (!ThickBufferAppendPoint(Buffer, P->X - Half, P->Y - Half) ||
            !ThickBufferAppendPoint(Buffer, P->X + Half, P->Y - Half) ||
            !ThickBufferAppendPoint(Buffer, P->X + Half, P->Y + Half) ||
            !ThickBufferAppendPoint(Buffer, P->X - Half, P->Y + Half))
            return FALSE;
    }
    else
    {
        return TRUE;
    }

    ThickBufferFinishContour(Buffer, start);
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
    _In_ BOOL               IsClosed,
    _In_ FLOAT              Thickness,
    _In_ THICK_LINE_JOIN    JoinStyle,
    _In_ THICK_LINE_CAP     CapStyle,
    _In_ FLOAT              MiterLimit,
    _Inout_ PTHICK_BUFFER   Buffer
)
{
    const FLOAT half = Thickness * 0.5f;
    const UINT  srcCount = EndIdx - StartIdx + 1;
    PTHICK_PT   pts = NULL;
    PTHICK_PT   rev = NULL;
    UINT        n = 0;
    UINT        i;
    UINT        start;
    BOOL        ok = FALSE;

    pts = (PTHICK_PT)malloc((size_t)srcCount * sizeof(THICK_PT));
    rev = (PTHICK_PT)malloc((size_t)srcCount * sizeof(THICK_PT));
    if (!pts || !rev)
        goto cleanup;

    /* --- Копируем точки, выбрасывая подряд идущие дубликаты --- */
    for (i = 0; i < srcCount; ++i)
    {
        FLOAT x = LineItem->Points[(StartIdx + i) * 2];
        FLOAT y = LineItem->Points[(StartIdx + i) * 2 + 1];

        if (n > 0)
        {
            FLOAT dx = x - pts[n - 1].X;
            FLOAT dy = y - pts[n - 1].Y;
            if (dx * dx + dy * dy < THICK_LINE_EPSILON * THICK_LINE_EPSILON)
                continue;
        }

        pts[n].X = x;
        pts[n].Y = y;
        n++;
    }

    /* Замкнутый путь: если последняя точка совпала с первой — убираем её */
    if (IsClosed && n > 1)
    {
        FLOAT dx = pts[n - 1].X - pts[0].X;
        FLOAT dy = pts[n - 1].Y - pts[0].Y;
        if (dx * dx + dy * dy < THICK_LINE_EPSILON * THICK_LINE_EPSILON)
            n--;
    }

    if (n == 0)
    {
        ok = TRUE;
        goto cleanup;
    }

    if (n == 1)
    {
        ok = ThickEmitDot(Buffer, &pts[0], half, CapStyle);
        goto cleanup;
    }

    /* Замкнутый путь из двух точек вырожден в линию туда-обратно */
    if (IsClosed && n < 3)
        IsClosed = FALSE;

    /* Реверс-копия: левая сторона реверсного пути = правая сторона исходного */
    for (i = 0; i < n; ++i)
        rev[i] = pts[n - 1 - i];

    if (IsClosed)
    {
        /* Два контура противоположной ориентации: кольцо при заливке Winding */
        start = Buffer->Count;
        if (!ThickEmitSide(Buffer, pts, n, TRUE, half, JoinStyle, MiterLimit))
            goto cleanup;
        ThickBufferFinishContour(Buffer, start);

        start = Buffer->Count;
        if (!ThickEmitSide(Buffer, rev, n, TRUE, half, JoinStyle, MiterLimit))
            goto cleanup;
        ThickBufferFinishContour(Buffer, start);
    }
    else
    {
        FLOAT dx, dy;

        /* Один контур: левая сторона -> cap на конце -> правая сторона -> cap в начале */
        start = Buffer->Count;

        if (!ThickEmitSide(Buffer, pts, n, FALSE, half, JoinStyle, MiterLimit))
            goto cleanup;

        ThickSegDir(&pts[n - 2], &pts[n - 1], &dx, &dy);
        if (!ThickEmitCap(Buffer, &pts[n - 1], dx, dy, half, CapStyle))
            goto cleanup;

        if (!ThickEmitSide(Buffer, rev, n, FALSE, half, JoinStyle, MiterLimit))
            goto cleanup;

        ThickSegDir(&rev[n - 2], &rev[n - 1], &dx, &dy);
        if (!ThickEmitCap(Buffer, &rev[n - 1], dx, dy, half, CapStyle))
            goto cleanup;

        ThickBufferFinishContour(Buffer, start);
    }

    ok = TRUE;

cleanup:
    free(pts);
    free(rev);
    return ok;
}

/* -------------------------------------------------------------
   Построение геометрии толстой линии с поддержкой нескольких subpath'ов
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

    if (!LineItem || LineItem->PointCount < 1)
        return NULL;

    ThickBufferInit(&buffer);

    for (i = 0; i < LineItem->PointCount; ++i)
    {
        BYTE rawType = LineItem->PointTypes[i];
        BYTE pointType = GET_POINT_TYPE(rawType);

        if (pointType == LinePointType_Move)
        {
            /* Завершаем предыдущий незамкнутый subpath */
            if (inSubPath)
            {
                if (!ThickLineProcessSubPath(LineItem, subPathStart, i - 1, FALSE,
                    Thickness, JoinStyle, CapStyle, MiterLimit, &buffer))
                {
                    ThickBufferUninit(&buffer);
                    return NULL;
                }
            }

            subPathStart = i;
            inSubPath = TRUE;
        }
        else if (!inSubPath)
        {
            /* Точка без Move после закрытого subpath — начинаем новый */
            subPathStart = i;
            inSubPath = TRUE;
        }

        if (IS_CLOSE_POINT(rawType))
        {
            if (!ThickLineProcessSubPath(LineItem, subPathStart, i, TRUE,
                Thickness, JoinStyle, CapStyle, MiterLimit, &buffer))
            {
                ThickBufferUninit(&buffer);
                return NULL;
            }
            inSubPath = FALSE;
        }
    }

    /* Последний незакрытый subpath */
    if (inSubPath)
    {
        if (!ThickLineProcessSubPath(LineItem, subPathStart, LineItem->PointCount - 1,
            LineItem->ClosePath,
            Thickness, JoinStyle, CapStyle, MiterLimit, &buffer))
        {
            ThickBufferUninit(&buffer);
            return NULL;
        }
    }

    result = LineItemAllocate(buffer.Count, LineItemType_Flatten, FALSE);
    if (!result)
    {
        ThickBufferUninit(&buffer);
        return NULL;
    }

    if (buffer.Count > 0)
    {
        memcpy(result->Points, buffer.Points, (size_t)buffer.Count * 2 * sizeof(FLOAT));
        memcpy(result->PointTypes, buffer.Types, (size_t)buffer.Count * sizeof(LINE_POINT_TYPE));
    }

    ThickBufferUninit(&buffer);
    return result;
}

/* -------------------------------------------------------------
   Проверка: содержит ли LineItem точки BezierControl
   ------------------------------------------------------------- */
static
BOOL
GR_CALL
ThickLineHasBezier(
    _In_ PSLineItem LineItem
)
{
    UINT i;

    for (i = 0; i < LineItem->PointCount; ++i)
    {
        if (GET_POINT_TYPE(LineItem->PointTypes[i]) == LinePointType_BezierControl)
            return TRUE;
    }

    return FALSE;
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

    if (ThickLineHasBezier(LineItem))
        return NULL;

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

