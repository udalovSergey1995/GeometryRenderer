#include "GeometryRendererLibInclude.h"
#include <stdio.h>

static int CheckPath(const char* name, FLOAT* points, LINE_POINT_TYPE* types,
    UINT count, const FLOAT* pattern, FLOAT offset,
    const FLOAT* expectedPoints, const LINE_POINT_TYPE* expectedTypes, UINT expectedCount)
{
    SLineItem input;
    PSDashPatternItem dashed;
    UINT i;
    int passed = 1;

    LineItemInitWithPoints(&input, points, types, count, LineItemType_Flatten, FALSE);
    dashed = DashPatternItemInitFromFlatten(&input, pattern, 2, offset);
    if (!dashed)
    {
        printf("FAIL: %s (no result)\n", name);
        return 1;
    }

    if (dashed->DashedLine->PointCount != expectedCount)
        passed = 0;
    else
        for (i = 0; i < expectedCount; ++i)
            if (dashed->DashedLine->PointTypes[i] != expectedTypes[i] ||
                fabsf(dashed->DashedLine->Points[i * 2] - expectedPoints[i * 2]) > 0.001f ||
                fabsf(dashed->DashedLine->Points[i * 2 + 1] - expectedPoints[i * 2 + 1]) > 0.001f)
                passed = 0;

    printf("%s: %s\n", passed ? "PASS" : "FAIL", name);
    DashPatternItemFree(dashed);
    return !passed;
}

int main(void)
{
    FLOAT solid[] = { 1000.0f, 1000.0f };
    FLOAT points[] = { 0, 0, 2, 0, 10, 0, 12, 0 };
    LINE_POINT_TYPE implicit[] = { 0, 0x81, 1, 1 };
    LINE_POINT_TYPE explicitMove[] = { 0, 0x81, 0, 1 };
    FLOAT closedExpected[] = { 0, 0, 2, 0, 0, 0, 10, 0, 12, 0 };
    LINE_POINT_TYPE closedTypes[] = { 0, 1, 1, 0, 1 };
    LINE_POINT_TYPE open[] = { 0, 1, 0, 1 };
    LINE_POINT_TYPE openTypes[] = { 0, 1, 0, 1 };
    LINE_POINT_TYPE bothClosed[] = { 0, 0x81, 1, 0x81 };
    FLOAT bothExpected[] = { 0, 0, 2, 0, 0, 0, 10, 0, 12, 0, 10, 0 };
    LINE_POINT_TYPE bothTypes[] = { 0, 1, 1, 0, 1, 1 };
    LINE_POINT_TYPE closedMove[] = { 0x80, 1, 1, 1 };
    FLOAT moveExpected[] = { 2, 0, 10, 0, 12, 0 };
    LINE_POINT_TYPE moveTypes[] = { 0, 1, 1 };
    FLOAT polyline[] = { 0, 0, 3, 0, 6, 0 };
    LINE_POINT_TYPE polyTypes[] = { 0, 1, 1 };
    FLOAT gapPattern[] = { 2, 3 };
    FLOAT gapExpected[] = { 0, 0, 2, 0, 5, 0, 6, 0 };
    LINE_POINT_TYPE gapTypes[] = { 0, 1, 0, 1 };
    FLOAT offsetPoints[] = { 0, 0, 2, 0, 10, 0, 15, 0 };
    FLOAT offsetPattern[] = { 10, 10 };
    FLOAT offsetExpected[] = { 0, 0, 2, 0, 0, 0, 10, 0, 13, 0 };
    int failures = 0;

    failures += CheckPath("Close then implicit subpath", points, implicit, 4, solid, 0,
        closedExpected, closedTypes, 5);
    failures += CheckPath("Close then Move", points, explicitMove, 4, solid, 1,
        closedExpected, closedTypes, 5);
    failures += CheckPath("Separate open subpaths", points, open, 4, solid, 0,
        points, openTypes, 4);
    failures += CheckPath("Two closed subpaths", points, bothClosed, 4, solid, 0,
        bothExpected, bothTypes, 6);
    failures += CheckPath("Move combined with Close", points, closedMove, 4, solid, 0,
        moveExpected, moveTypes, 3);
    failures += CheckPath("Continuous visible stroke", polyline, polyTypes, 3, solid, 0,
        polyline, polyTypes, 3);
    failures += CheckPath("Gap crossing a vertex", polyline, polyTypes, 3, gapPattern, 0,
        gapExpected, gapTypes, 4);
    failures += CheckPath("Pattern phase and offset preserved across Close", offsetPoints, implicit, 4,
        offsetPattern, 3, offsetExpected, closedTypes, 5);
    return failures ? 1 : 0;
}
