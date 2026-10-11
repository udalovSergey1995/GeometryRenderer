#include "stdafx.h"

/* Лимиты дают явный отказ вместо неполного результата. */
#define MAX_EDGES 4096u
#define MAX_RECORDS 1048576u
#define MAX_COORDINATE 1000000.0

typedef struct
{
    double ax;
    double ay;
    double bx;
    double by;
    int winding;
} EDGE;
typedef struct
{
    double x;
    UINT edge;
} CROSSING;
typedef struct
{
    double x;
    double y;
} VERTEX;

static
double
EdgeX(
    _In_ const EDGE *e,
    _In_ double y
)
{
    return e->ax + (y - e->ay) * (e->bx - e->ax) / (e->by - e->ay);
}

static
int
CompareY(
    _In_ const void *a,
    _In_ const void *b
)
{
    double x = *(const double *)a;
    double y = *(const double *)b;
    return (x > y) - (x < y);
}

static
int
CompareX(
    _In_ const void *a,
    _In_ const void *b
)
{
    double x = ((const CROSSING *)a)->x;
    double y = ((const CROSSING *)b)->x;
    return (x > y) - (x < y);
}

static
BOOL
AddEvent(
    _Inout_ double **events,
    _Inout_ UINT *count,
    _Inout_ UINT *capacity,
    _In_ double y
)
{
    if (*count == *capacity)
    {
        UINT size = *capacity ? *capacity * 2 : 64;
        double *data;
        if (size > MAX_RECORDS)
        {
            return FALSE;
        }
        data = (double *)realloc(*events, size * sizeof(double));
        if (!data)
        {
            return FALSE;
        }
        *events = data;
        *capacity = size;
    }
    (*events)[(*count)++] = y;
    return TRUE;
}

static
BOOL
AddEdge(
    _Inout_ EDGE *edges,
    _Inout_ UINT *count,
    _In_reads_(2) const FLOAT *a,
    _In_reads_(2) const FLOAT *b
)
{
    EDGE *edge;
    if (a[1] == b[1])
    {
        return TRUE;
    }
    if (*count == MAX_EDGES)
    {
        return FALSE;
    }
    edge = edges + (*count)++;
    edge->ax = a[0];
    edge->ay = a[1];
    edge->bx = b[0];
    edge->by = b[1];
    edge->winding = b[1] > a[1] ? 1 : -1;
    return TRUE;
}

_Check_return_
_Ret_maybenull_
PSRasterSlabItem
GR_CALL
RasterSlabItemCreate(
    _In_ PSLineItem line
)
{
    EDGE *edges = NULL;
    CROSSING *crossings = NULL;
    double *events = NULL;
    UINT edgeCount = 0;
    UINT eventCount = 0;
    UINT eventCapacity = 0;
    UINT capacity = 0;
    UINT i;
    UINT j;
    UINT start = 0;
    BOOL open = FALSE;
    BOOL success = FALSE;
    PSRasterSlabItem item = NULL;
    if (!line || line->Type != LineItemType_Flatten ||
        (line->PointCount && (!line->Points || !line->PointTypes)))
    {
        return NULL;
    }
    item = (PSRasterSlabItem)calloc(1, sizeof(*item));
    edges = (EDGE *)malloc(MAX_EDGES * sizeof(*edges));
    crossings = (CROSSING *)malloc(MAX_EDGES * sizeof(*crossings));
    if (!item || !edges || !crossings)
    {
        goto cleanup;
    }
    /* 1. Разбираем подпути. Горизонтальные рёбра не меняют winding,
       но их вершины задают границы горизонтальных полос. */
    for (i = 0; i < line->PointCount; ++i)
    {
        FLOAT *p = line->Points + i * 2;
        BYTE type = GET_POINT_TYPE(line->PointTypes[i]);
        if (!isfinite(p[0]) || !isfinite(p[1]) || fabs(p[0]) > MAX_COORDINATE ||
            fabs(p[1]) > MAX_COORDINATE ||
            (type != LinePointType_Move && type != LinePointType_Line))
        {
            goto cleanup;
        }
        if (!AddEvent(&events, &eventCount, &eventCapacity, p[1]))
        {
            goto cleanup;
        }
        if (type == LinePointType_Move || !open)
        {
            /* Заполнение неявно замыкает открытые фигуры, как FillPath. */
            if (open &&
                !AddEdge(edges, &edgeCount, line->Points + (i - 1) * 2, line->Points + start * 2))
            {
                goto cleanup;
            }
            start = i;
            open = TRUE;
        }
        else if (!AddEdge(edges, &edgeCount, line->Points + (i - 1) * 2, p))
        {
            goto cleanup;
        }
        if (IS_CLOSE_POINT(line->PointTypes[i]))
        {
            if (!AddEdge(edges, &edgeCount, p, line->Points + start * 2))
            {
                goto cleanup;
            }
            open = FALSE;
        }
    }
    if (open &&
        !AddEdge(
            edges, &edgeCount, line->Points + (line->PointCount - 1) * 2, line->Points + start * 2))
    {
        goto cleanup;
    }
    if (!edgeCount)
    {
        success = TRUE;
        goto cleanup;
    }
    {
        /* 2. Добавляем границы пиксельных строк. Вершины и пересечения
           внутри строки разбивают её на слэбы высотой меньше единицы. */
        double minY = events[0];
        double maxY = events[0];
        double y;
        for (i = 1; i < eventCount; ++i)
        {
            if (events[i] < minY)
            {
                minY = events[i];
            }
            if (events[i] > maxY)
            {
                maxY = events[i];
            }
        }
        for (y = floor(minY) + 1; y < maxY; y += 1)
        {
            if (!AddEvent(&events, &eventCount, &eventCapacity, y))
            {
                goto cleanup;
            }
        }
    }
    /* Пересечения делят полосы до изменения порядка активных рёбер. */
    for (i = 0; i < edgeCount; ++i)
    {
        for (j = i + 1; j < edgeCount; ++j)
        {
            const EDGE *a = edges + i;
            const EDGE *b = edges + j;
            double dx = a->bx - a->ax;
            double dy = a->by - a->ay;
            double ex = b->bx - b->ax;
            double ey = b->by - b->ay;
            double den = dx * ey - dy * ex;
            double t;
            double u;
            if (den == 0)
            {
                continue;
            }
            t = ((b->ax - a->ax) * ey - (b->ay - a->ay) * ex) / den;
            u = ((b->ax - a->ax) * dy - (b->ay - a->ay) * dx) / den;
            if (t > 0 && t < 1 && u > 0 && u < 1 &&
                !AddEvent(&events, &eventCount, &eventCapacity, a->ay + t * dy))
            {
                goto cleanup;
            }
        }
    }
    /* 3. Между событиями порядок рёбер постоянен. Winding проверяется
       в середине полосы; повторяющиеся Y не создают пустых слэбов. */
    qsort(events, eventCount, sizeof(double), CompareY);
    for (i = 1; i < eventCount; ++i)
    {
        double top = events[i - 1];
        double bottom = events[i];
        double mid = (top + bottom) / 2;
        UINT n = 0;
        UINT k;
        UINT left = 0;
        int winding = 0;
        if (top == bottom || (FLOAT)top == (FLOAT)bottom)
        {
            continue;
        }
        for (j = 0; j < edgeCount; ++j)
        {
            if (mid >= fmin(edges[j].ay, edges[j].by) && mid < fmax(edges[j].ay, edges[j].by))
            {
                crossings[n].x = EdgeX(edges + j, mid);
                crossings[n++].edge = j;
            }
        }
        qsort(crossings, n, sizeof(*crossings), CompareX);
        for (k = 0; k < n; ++k)
        {
            int before = winding;
            winding += edges[crossings[k].edge].winding;
            if (!before && winding)
            {
                left = crossings[k].edge;
            }
            /* Переход в ноль закрывает интервал. Изменение ненулевой
               глубины winding не приводит к повторной заливке перекрытия. */
            if (before && !winding)
            {
                SRasterSlab slab;
                EDGE *l = edges + left;
                EDGE *r = edges + crossings[k].edge;
                if (EdgeX(r, mid) <= EdgeX(l, mid))
                {
                    continue;
                }
                slab.Y = (FLOAT)top;
                slab.Height = (FLOAT)bottom - slab.Y;
                slab.XLeftTop = (FLOAT)EdgeX(l, top);
                slab.XLeftBottom = (FLOAT)EdgeX(l, bottom);
                slab.XRightTop = (FLOAT)EdgeX(r, top);
                slab.XRightBottom = (FLOAT)EdgeX(r, bottom);
                if (item->Count == capacity)
                {
                    UINT size = capacity ? capacity * 2 : 64;
                    PSRasterSlab data;
                    if (size > MAX_RECORDS)
                    {
                        goto cleanup;
                    }
                    data = (PSRasterSlab)realloc(item->Slabs, size * sizeof(*data));
                    if (!data)
                    {
                        goto cleanup;
                    }
                    item->Slabs = data;
                    capacity = size;
                }
                item->Slabs[item->Count++] = slab;
            }
        }
    }
    success = TRUE;
cleanup:
    /* Временные буферы не принадлежат айтему. При ошибке уничтожаем
       также частичный результат, чтобы не публиковать неполную геометрию. */
    free(edges);
    free(crossings);
    free(events);
    if (!success)
    {
        RasterSlabItemFree(item);
        return NULL;
    }
    return item;
}

VOID
GR_CALL
RasterSlabItemFree(
    _In_opt_ PVOID data
)
{
    PSRasterSlabItem item = (PSRasterSlabItem)data;
    if (item)
    {
        free(item->Slabs);
        free(item);
    }
}

BOOL
GR_CALL
RasterSlabItemGetProperty(
    _In_ PSPathStageEntry entry,
    _In_ EStageProps property,
    _Maybenull_ PVOID output,
    _Out_ PINT length
)
{
    PSRasterSlabItem item;
    if (!length)
    {
        return FALSE;
    }
    *length = 0;
    if (!entry || !(item = (PSRasterSlabItem)entry->StageData))
    {
        return FALSE;
    }
    switch (property)
    {
    case ESP_TYPE:
        return CopyObjectProperty(&entry->StageType, sizeof(entry->StageType), output, length);
    case ESP_SLAB_COUNT:
        return CopyObjectProperty(&item->Count, sizeof(item->Count), output, length);
    case ESP_SLAB_DATA:
        return CopyObjectProperty(
            item->Slabs, (INT)(item->Count * sizeof(SRasterSlab)), output, length);
    default:
        return FALSE;
    }
}

/* Sutherland-Hodgman: пересечение трапеции с единичным пикселем. */
static
double
PixelArea(
    _In_ const SRasterSlab *s,
    _In_ double x,
    _In_ double y
)
{
    VERTEX a[12];
    VERTEX b[12];
    UINT n = 4;
    UINT pass;
    UINT i;
    double area = 0;
    a[0].x = s->XLeftTop - x;
    a[0].y = s->Y - y;
    a[1].x = s->XRightTop - x;
    a[1].y = a[0].y;
    a[2].x = s->XRightBottom - x;
    a[2].y = (double)s->Y + s->Height - y;
    a[3].x = s->XLeftBottom - x;
    a[3].y = a[2].y;
    /* Отсекаем многоугольник слева, справа, сверху и снизу.
       Координаты локальны относительно единичного пикселя. */
    for (pass = 0; pass < 4 && n; ++pass)
    {
        UINT out = 0;
        for (i = 0; i < n; ++i)
        {
            VERTEX p = a[i];
            VERTEX q = a[(i + 1) % n];
            double pv = pass < 2 ? p.x : p.y;
            double qv = pass < 2 ? q.x : q.y;
            double boundary = pass % 2 ? 1 : 0;
            BOOL pin = pass % 2 ? pv <= boundary : pv >= boundary;
            BOOL qin = pass % 2 ? qv <= boundary : qv >= boundary;
            if (pin)
            {
                b[out++] = p;
            }
            if (pin != qin)
            {
                double t = (boundary - pv) / (qv - pv);
                b[out].x = p.x + t * (q.x - p.x);
                b[out++].y = p.y + t * (q.y - p.y);
            }
        }
        n = out;
        memcpy(a, b, n * sizeof(VERTEX));
    }
    /* Формула площади многоугольника даёт геометрическое покрытие,
       а не эвристический градиент прозрачности вокруг ребра. */
    for (i = 0; i < n; ++i)
    {
        area += a[i].x * a[(i + 1) % n].y - a[i].y * a[(i + 1) % n].x;
    }
    return fabs(area) * 0.5;
}

GR_EXPORT
FLOAT
GR_CALL
RasterSlabGetCoverage(
    _In_ PSPathStageEntry entry,
    _In_ FLOAT x,
    _In_ FLOAT y
)
{
    PSRasterSlabItem item;
    UINT i;
    double area = 0;
    if (!entry || entry->StageType != PathStageTypeRasterSlab || !isfinite(x) || !isfinite(y) ||
        !(item = (PSRasterSlabItem)entry->StageData))
    {
        return 0;
    }
    for (i = 0; i < item->Count; ++i)
    {
        if (item->Slabs[i].Y < (double)y + 1 &&
            (double)item->Slabs[i].Y + item->Slabs[i].Height > y)
        {
            area += PixelArea(item->Slabs + i, x, y);
        }
    }
    return (FLOAT)fmin(1.0, area);
}

BOOL
GR_CALL
RasterSlabItemDraw(
    _In_ PSRasterSlabItem item,
    _Inout_ PBYTE pixels,
    _In_ INT width,
    _In_ INT height,
    _In_ INT stride,
    _In_ UINT color
)
{
    double *coverage;
    UINT first = 0;
    if (!item || !pixels || width <= 0 || height <= 0)
    {
        return FALSE;
    }
    coverage = (double *)calloc((size_t)width, sizeof(double));
    if (!coverage)
    {
        return FALSE;
    }
    /* Слэбы отсортированы по Y: накопление покрытия и смешивание один раз на строку. */
    while (first < item->Count)
    {
        INT y = (INT)floorf(item->Slabs[first].Y);
        INT x;
        UINT end = first + 1;
        UINT i;
        while (end < item->Count && (INT)floorf(item->Slabs[end].Y) == y)
        {
            ++end;
        }
        if (y >= 0 && y < height)
        {
            memset(coverage, 0, (size_t)width * sizeof(double));
            for (i = first; i < end; ++i)
            {
                SRasterSlab *s = item->Slabs + i;
                double left = fmin(s->XLeftTop, s->XLeftBottom);
                double right = fmax(s->XRightTop, s->XRightBottom);
                INT startX = (INT)fmax(0, fmin(width, floor(left)));
                INT endX = (INT)fmax(0, fmin(width, ceil(right)));
                for (x = startX; x < endX; ++x)
                {
                    coverage[x] += PixelArea(s, x, y);
                }
            }
            for (x = 0; x < width; ++x)
            {
                if (coverage[x] > 0)
                {
                    /* Цвет ARGB обычный, назначение BGRA premultiplied.
                       Покрытие умножается на альфу перед source-over;
                       смешивание выполняется один раз для каждого пикселя. */
                    double alpha = ((color >> 24) & 255) / 255.0 * fmin(1, coverage[x]);
                    PBYTE p = pixels + (size_t)y * stride + (size_t)x * 4;
                    UINT c;
                    for (c = 0; c < 3; ++c)
                    {
                        p[c] = (BYTE)fmin(
                            255,
                            floor(((color >> (c * 8)) & 255) * alpha + p[c] * (1 - alpha) + 0.5));
                    }
                    p[3] = (BYTE)fmin(255, floor(255 * alpha + p[3] * (1 - alpha) + 0.5));
                }
            }
        }
        first = end;
    }
    free(coverage);
    return TRUE;
}
