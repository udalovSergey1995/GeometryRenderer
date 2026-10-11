#include "GeometryRendererLibInclude.h"
#include <stdio.h>
#include <float.h>

static int failures;
#define CHECK(c, name) do { if (!(c)) { printf("FAIL: %s (%d)\n", name, __LINE__); ++failures; } } while (0)

static PSThickLineItem Build(FLOAT* pts, BYTE* flags, UINT n, FLOAT width,
    THICK_LINE_JOIN join, THICK_LINE_CAP cap, FLOAT limit, FLOAT tolerance)
{
    SLineItem input;
    LineItemInitWithPoints(&input, pts, flags, n, LineItemType_Flatten, FALSE);
    return ThickLineItemInitEx(&input, width, join, cap, limit, tolerance);
}

static int Edge(double ax, double ay, double bx, double by, double x, double y)
{
    double cross = (bx - ax) * (y - ay) - (by - ay) * (x - ax);
    if (ay <= y && by > y && cross > 0) return 1;
    if (ay > y && by <= y && cross < 0) return -1;
    return 0;
}

static int Winding(PSLineItem path, double x, double y)
{
    UINT i, start = 0;
    int result = 0;
    for (i = 0; i < path->PointCount; ++i)
    {
        if (GET_POINT_TYPE(path->PointTypes[i]) == LinePointType_Move) start = i;
        else if (i > 0)
            result += Edge(path->Points[(i - 1) * 2], path->Points[(i - 1) * 2 + 1],
                path->Points[i * 2], path->Points[i * 2 + 1], x, y);
        if (IS_CLOSE_POINT(path->PointTypes[i]))
            result += Edge(path->Points[i * 2], path->Points[i * 2 + 1],
                path->Points[start * 2], path->Points[start * 2 + 1], x, y);
    }
    return result;
}

static double Distance(double x, double y, const FLOAT* a, const FLOAT* b)
{
    double dx = (double)b[0] - a[0], dy = (double)b[1] - a[1];
    double t = ((x - a[0]) * dx + (y - a[1]) * dy) / (dx * dx + dy * dy);
    if (t < 0) t = 0;
    if (t > 1) t = 1;
    return hypot(x - a[0] - t * dx, y - a[1] - t * dy);
}

/* Независимая проверка: круглая обводка — объединение дисков вдоль отрезков. */
static void RoundRegion(FLOAT* pts, UINT n, FLOAT width, BOOL closed, const char* name)
{
    BYTE flags[8] = { 0, 1, 1, 1, 1, 1, 1, 1 };
    PSThickLineItem item;
    int ix, iy, mismatches = 0;
    if (closed) flags[n - 1] = MAKE_CLOSE_POINT(flags[n - 1]);
    item = Build(pts, flags, n, width, ThickLineJoin_Round, ThickLineCap_Round, 10, 0.001f);
    CHECK(item != NULL, name);
    if (!item) return;
    for (ix = -40; ix <= 80; ++ix)
        for (iy = -40; iy <= 80; ++iy)
        {
            double x = ix * 0.37 + 0.031, y = iy * 0.37 + 0.017, distance = DBL_MAX;
            UINT i;
            for (i = 1; i < n; ++i)
            {
                double d = Distance(x, y, pts + (i - 1) * 2, pts + i * 2);
                if (d < distance) distance = d;
            }
            if (closed)
            {
                double d = Distance(x, y, pts + (n - 1) * 2, pts);
                if (d < distance) distance = d;
            }
            if (fabs(distance - width * 0.5) > 0.003 &&
                ((Winding(item->ThickGeometry, x, y) != 0) != (distance < width * 0.5))) ++mismatches;
        }
    if (mismatches) printf("%s: %d region mismatches\n", name, mismatches);
    CHECK(mismatches == 0, name);
    ThickLineItemFree(item);
}

static void TestCapsAndJoins(void)
{
    FLOAT lineAndDot[] = { 0, 0, 10, 0, 5, 0 };
    BYTE dotFlags[] = { 0, 1, 0 };
    FLOAT corner[] = { 0, 0, 10, 0, 10, 10 };
    FLOAT reversal[] = { 0, 0, 10, 0, 0, 0 };
    BYTE flags[] = { 0, 1, 1 };
    int style;
    for (style = 0; style < 3; ++style)
    {
        PSThickLineItem item = Build(lineAndDot, dotFlags, 3, 2, ThickLineJoin_Miter,
            (THICK_LINE_CAP)style, 10, 0.001f);
        CHECK(item != NULL, "caps and degenerate subpath");
        if (item)
        {
            CHECK(Winding(item->ThickGeometry, 5.1, 0.1) == (style ? -2 : -1), "dot orientation and overlap");
            CHECK((Winding(item->ThickGeometry, -0.5, 0.1) != 0) == (style != ThickLineCap_Flat), "cap extension");
            CHECK((Winding(item->ThickGeometry, -0.8, 0.8) != 0) == (style == ThickLineCap_Square), "cap shape");
            ThickLineItemFree(item);
        }
        item = Build(corner, flags, 3, 2, (THICK_LINE_JOIN)style, ThickLineCap_Flat, 10, 0.001f);
        CHECK(item != NULL, "join construction");
        if (item)
        {
            CHECK(Winding(item->ThickGeometry, 9.5, 0.5) != 0, "inner corner filled");
            CHECK((Winding(item->ThickGeometry, 10.8, -0.8) != 0) == (style == ThickLineJoin_Miter), "join shape");
            ThickLineItemFree(item);
        }
        item = Build(reversal, flags, 3, 2, (THICK_LINE_JOIN)style, ThickLineCap_Flat, 10, 0.001f);
        CHECK(item != NULL, "exact reversal");
        if (item)
        {
            CHECK(Winding(item->ThickGeometry, 5, 0.2) != 0, "retraced stroke");
            CHECK((Winding(item->ThickGeometry, 10.5, 0.1) != 0) == (style == ThickLineJoin_Round), "round reversal points outward");
            ThickLineItemFree(item);
        }
    }
    {
        PSThickLineItem item = Build(corner, flags, 3, 2, ThickLineJoin_Miter, ThickLineCap_Flat, 1, 0.001f);
        CHECK(item != NULL, "limited miter");
        if (item)
        {
            CHECK(Winding(item->ThickGeometry, 10.8, -0.8) == 0, "miter fallback to bevel");
            ThickLineItemFree(item);
        }
    }
}

static void TestToleranceAndErrors(void)
{
    FLOAT pt[] = { 0, 0 };
    BYTE flag[] = { 0 };
    FLOAT tolerances[] = { 0.1f, 0.01f };
    UINT t, previousCount = 0;
    for (t = 0; t < 2; ++t)
    {
        PSThickLineItem item = Build(pt, flag, 1, 20000, ThickLineJoin_Round, ThickLineCap_Round, 10, tolerances[t]);
        UINT i;
        CHECK(item != NULL, "large radius");
        if (!item) continue;
        CHECK(item->ThickGeometry->PointCount > previousCount, "precision increases subdivision");
        previousCount = item->ThickGeometry->PointCount;
        for (i = 0; i < previousCount; ++i)
        {
            UINT next = (i + 1) % previousCount;
            double mx = ((double)item->ThickGeometry->Points[i * 2] + item->ThickGeometry->Points[next * 2]) / 2;
            double my = ((double)item->ThickGeometry->Points[i * 2 + 1] + item->ThickGeometry->Points[next * 2 + 1]) / 2;
            CHECK(fabs(10000 - hypot(mx, my)) <= tolerances[t] + 0.002, "chord deviation plus float rounding");
        }
        ThickLineItemFree(item);
    }
    {
        FLOAT invalid[] = { 0, -1, NAN, INFINITY, 1e-12f };
        for (t = 0; t < sizeof(invalid) / sizeof(invalid[0]); ++t)
        {
            PSThickLineItem item = Build(pt, flag, 1, 20000, ThickLineJoin_Round, ThickLineCap_Round, 10, invalid[t]);
            CHECK(item == NULL, "invalid tolerance or resource limit");
            ThickLineItemFree(item);
        }
        pt[0] = NAN;
        CHECK(Build(pt, flag, 1, 2, ThickLineJoin_Round, ThickLineCap_Round, 10, 0.1f) == NULL, "invalid coordinate");
    }
}

int main(void)
{
    FLOAT crossing[] = { 0, 0, 10, 10, 0, 10, 10, 0 };
    FLOAT shortTurn[] = { 0, 0, 0.1f, 0, 0.1f, 0.1f, 10, 0.1f };
    FLOAT square[] = { 0, 0, 10, 0, 10, 10, 0, 10 };
    FLOAT reverseSquare[] = { 0, 10, 10, 10, 10, 0, 0, 0 };
    FLOAT hairpin[] = { 0, 0, 10, 0, 0, 0.1f, 10, 0.1f };
    TestCapsAndJoins();
    TestToleranceAndErrors();
    RoundRegion(crossing, 4, 2, FALSE, "self intersection");
    RoundRegion(shortTurn, 4, 2, FALSE, "short segments");
    RoundRegion(square, 4, 2, TRUE, "closed ring");
    RoundRegion(reverseSquare, 4, 2, TRUE, "opposite source direction");
    RoundRegion(square, 4, 30, TRUE, "collapsed inner ring");
    RoundRegion(hairpin, 4, 2, FALSE, "near reversals");
    printf("Thick geometry tests: %s (%d failures)\n", failures ? "FAIL" : "PASS", failures);
    return failures ? 1 : 0;
}
