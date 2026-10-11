#include "GeometryRendererLibInclude.h"
#include <stdio.h>
/* Отказы памяти только в тестовой единице трансляции. */
static int allocationsLeft = -1;
static void* FaultRealloc(void* memory, size_t size)
{
    if (allocationsLeft == 0) return NULL;
    if (allocationsLeft > 0) --allocationsLeft;
    return realloc(memory, size);
}
#define realloc FaultRealloc
#include "../GeometryRendererLib/flatten_item.c"
#undef realloc
static int failures;
#define CHECK(c) do { if (!(c)) { printf("Reliability failure at %d\n", __LINE__); ++failures; } } while (0)
int main(void)
{
    SLineItem line;
    FLOAT loop[] = { 0,0,10,0,10,10,0,0 };
    BYTE curve[] = { 0,3,3,0x81 };
    PSLineItem flat;
    UINT i;
    LineItemInitWithPoints(&line, loop, curve, 4, LineItemType_Bezier, FALSE);
    flat = FlattenItemInitEx(&line, 0.01f);
    CHECK(flat && flat->PointCount > 4);
    if (flat) { CHECK(IS_CLOSE_POINT(flat->PointTypes[flat->PointCount-1])); LineItemFreeWithPoints(flat); }
    for (allocationsLeft = 0; allocationsLeft <= 3; )
    {
        int budget = allocationsLeft;
        flat = FlattenItemInitEx(&line, 0.001f);
        CHECK(flat == NULL);
        LineItemFreeWithPoints(flat);
        allocationsLeft = budget + 1;
    }
    allocationsLeft = -1;
    {
        PSPathPipeLine pipe = PathPipeLineCreate();
        FLOAT largeLoop[] = { 0, 0, 1000, 0, 1000, 1000, 0, 0 };
        CHECK(PathPipeLineAddLogicalLine(pipe, largeLoop, curve, 4, TRUE, TRUE));
        allocationsLeft = 2;
        CHECK(!PathPipeLineFlattenizeLogicalLine(pipe));
        CHECK(pipe->m_sPathStages.Count == 1);
        allocationsLeft = -1;
        PathPipeLineDestroy(pipe);
    }
    {
        FLOAT points[] = {0,0,2,0,4,0}; BYTE types[] = {0,1,0x81};
        LineItemInitWithPoints(&line, points, types, 3, LineItemType_Bezier, FALSE);
        flat = FlattenItemInit(&line);
        CHECK(flat && flat->PointCount == 3 && IS_CLOSE_POINT(flat->PointTypes[2]));
        LineItemFreeWithPoints(flat);
        types[2] = 1;
        LineItemInitWithPoints(&line, points, types, 3, LineItemType_Flatten, FALSE);
        {
            FLOAT pattern[] = {2,2};
            PSDashPatternItem dash = DashPatternItemInitFromFlatten(&line, pattern, 2, 0);
            CHECK(dash && dash->DashedLine->PointCount == 2 && dash->DashedLine->Points[2] == 2);
            DashPatternItemFree(dash);
            dash = DashPatternItemInitFromFlatten(&line, pattern, 2, 1e20f);
            CHECK(dash != NULL); DashPatternItemFree(dash);
            CHECK(DashPatternItemInitFromFlatten(&line, pattern, 2, INFINITY) == NULL);
            CHECK(DashPatternItemInitFromFlatten(&line, pattern, 2, NAN) == NULL);
            pattern[0] = 0;
            CHECK(DashPatternItemInitFromFlatten(&line, pattern, 2, 0) == NULL);
        }
        {
            PSPathPipeLine pipe = PathPipeLineCreate();
            BYTE guard[32]; INT length = 4;
            CHECK(PathPipeLineAddLogicalLine(pipe, points, types, 3, FALSE, FALSE));
            memset(guard, 0xcc, sizeof(guard));
            CHECK(!GetObjectProperty(PathPipeLineGetLastStage(pipe), ESP_PATH_DATA, guard, &length));
            CHECK(length == sizeof(points));
            for (i=0; i<sizeof(guard); ++i) CHECK(guard[i] == 0xcc);
            CHECK(GetObjectProperty(PathPipeLineGetLastStage(pipe), ESP_PATH_DATA, guard, &length));
            CHECK(memcmp(guard, points, sizeof(points)) == 0);
            PathPipeLineDestroy(pipe);
        }
    }
    printf("Reliability native tests: %s\n", failures ? "FAIL" : "PASS");
    return failures ? 1 : 0;
}
