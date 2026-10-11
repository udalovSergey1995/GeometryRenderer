#include "GeometryRendererLibInclude.h"
#include <stdio.h>

static int failures;
#define CHECK(condition, name) do { \
    if (!(condition)) { printf("FAIL: %s (line %d)\n", name, __LINE__); ++failures; } \
} while (0)

static void CheckValue(PSPathStageEntry entry, EStageProps property, PCVOID expected, INT size)
{
    BYTE output[32];
    INT length = -1;
    CHECK(GetObjectProperty(entry, property, NULL, &length) == TRUE && length == size,
        "size query returns byte count");
    memset(output, 0xcc, sizeof(output));
    CHECK(size <= sizeof(output), "test buffer capacity");
    CHECK(GetObjectProperty(entry, property, output, &length) == TRUE && length == size,
        "value query succeeds");
    CHECK(memcmp(output, expected, size) == 0, "property value matches");
    CHECK(output[size] == 0xcc, "property copy ends at declared length");
}

static void CheckGeometry(PSPathStageEntry entry, PSLineItem geometry,
    GET_PATH_STAGE_PROPERTY callback)
{
    BOOL isBezier = geometry->Type == LineItemType_Bezier;
    INT length;
    PFLOAT points;
    PBYTE flags;
    CHECK(entry->GetPropertyCallback == callback, "stage owns correct callback");
    CheckValue(entry, ESP_TYPE, &entry->StageType, sizeof(entry->StageType));
    CheckValue(entry, ESP_PATH_LEN, &geometry->PointCount, sizeof(geometry->PointCount));
    CheckValue(entry, ESP_IS_BESIERE, &isBezier, sizeof(isBezier));
    CheckValue(entry, ESP_IS_CLOSED, &geometry->ClosePath, sizeof(geometry->ClosePath));

    length = -1;
    CHECK(GetObjectProperty(entry, ESP_PATH_DATA, NULL, &length) &&
        length == (INT)(geometry->PointCount * 2 * sizeof(FLOAT)), "points size");
    points = (PFLOAT)malloc(length ? (size_t)length : 1);
    CHECK(points != NULL, "points buffer");
    if (points)
    {
        CHECK(GetObjectProperty(entry, ESP_PATH_DATA, points, &length), "points query");
        if (length > 0) CHECK(memcmp(points, geometry->Points, length) == 0, "points copied");
        free(points);
    }
    CHECK(GetObjectProperty(entry, ESP_PATH_FLAGS, NULL, &length) &&
        length == (INT)geometry->PointCount, "flags size");
    flags = (PBYTE)malloc(length ? (size_t)length : 1);
    CHECK(flags != NULL, "flags buffer");
    if (flags)
    {
        CHECK(GetObjectProperty(entry, ESP_PATH_FLAGS, flags, &length), "flags query");
        if (length > 0) CHECK(memcmp(flags, geometry->PointTypes, length) == 0, "flags copied");
        free(flags);
    }
    length = 123;
    CHECK(!GetObjectProperty(entry, (EStageProps)999, NULL, &length) && length == 0,
        "unknown property fails with zero length");
}

static void TestPipeline(void)
{
    FLOAT points[] = { 0, 0, 10, 0 };
    BYTE flags[] = { 0, 1 };
    FLOAT pattern[] = { 3, 2 };
    PSPathPipeLine pipeline = PathPipeLineCreate();
    PSPathStageEntry entry;
    CHECK(pipeline != NULL, "pipeline allocation");
    if (!pipeline) return;
    CHECK(PathPipeLineAddLogicalLine(pipeline, points, flags, 2, FALSE, FALSE), "logical stage");
    entry = PathPipeLineGetLastStage(pipeline);
    CheckGeometry(entry, (PSLineItem)entry->StageData, LineItemGetProperty);
    CHECK(PathPipeLineFlattenizeLogicalLine(pipeline), "flatten stage");
    entry = PathPipeLineGetLastStage(pipeline);
    CheckGeometry(entry, (PSLineItem)entry->StageData, FlattenItemGetProperty);
    CHECK(PathPipeLineApplyDashPattern(pipeline, pattern, 2, 1.25f), "dash stage");
    entry = PathPipeLineGetLastStage(pipeline);
    {
        PSDashPatternItem dash = (PSDashPatternItem)entry->StageData;
        CheckGeometry(entry, dash->DashedLine, DashPatternItemGetProperty);
        CheckValue(entry, ESP_DASH_COUNT, &dash->DashCount, sizeof(dash->DashCount));
        CheckValue(entry, ESP_DASH_OFFSET, &dash->DashOffset, sizeof(dash->DashOffset));
        CHECK(dash->DashOffset == 1.25f, "fractional dash offset");
    }
    CHECK(PathPipeLineApplyThickLine(pipeline, 4, ThickLineJoin_Bevel, ThickLineCap_Flat, 3.5f),
        "thick stage");
    entry = PathPipeLineGetLastStage(pipeline);
    {
        PSThickLineItem thick = (PSThickLineItem)entry->StageData;
        CheckGeometry(entry, thick->ThickGeometry, ThickLineItemGetProperty);
        CheckValue(entry, ESP_THICKNESS, &thick->Thickness, sizeof(thick->Thickness));
        CheckValue(entry, ESP_JOIN_STYLE, &thick->JoinStyle, sizeof(thick->JoinStyle));
        CheckValue(entry, ESP_CAP_STYLE, &thick->CapStyle, sizeof(thick->CapStyle));
        CheckValue(entry, ESP_MITER_LIMIT, &thick->MiterLimit, sizeof(thick->MiterLimit));
    }
    CHECK(pipeline->m_sPathStages.Count == 4, "pipeline stage count preserved");
    PathPipeLineDestroy(pipeline);
}

static int customCalls;
static BOOL GR_CALL CustomGetProperty(PSPathStageEntry entry, EStageProps property,
    PVOID output, PINT length)
{
    ++customCalls;
    if (property == (EStageProps)1000)
        return CopyObjectProperty(entry->StageData, sizeof(INT), output, length);
    if (length) *length = 0;
    return FALSE;
}

static void TestCustomStageAndErrors(void)
{
    SPathStages stages;
    SPathStageEntry broken = { 0 };
    INT value = 42, output = 0, length = 123;
    PathStagesInitialize(&stages);
    CHECK(!PathStagesAddStage(&stages, PathStageTypeLogicalCurve, &value, NULL, NULL),
        "missing callback rejected");
    CHECK(stages.Count == 0, "failed addition leaves container unchanged");
    CHECK(PathStagesAddStage(&stages, (EPathStageType)100, &value, NULL, CustomGetProperty),
        "custom stage without central registration");
    CHECK(GetObjectProperty(PathStagesGetLastStage(&stages), 1000, NULL, &length) && length == sizeof(INT),
        "custom size query routed");
    CHECK(GetObjectProperty(PathStagesGetLastStage(&stages), 1000, &output, &length) && output == value,
        "custom value query routed");
    CHECK(customCalls == 2, "custom callback actually called");
    PathStagesUninitialize(&stages);

    CHECK(!GetObjectProperty(NULL, ESP_TYPE, NULL, &length) && length == 0, "null object");
    CHECK(!GetObjectProperty(&broken, ESP_TYPE, NULL, NULL), "null length");
    broken.Base.type = EOT_PipeStageEntry;
    broken.StageData = &value;
    length = 123;
    CHECK(!GetObjectProperty(&broken, ESP_TYPE, NULL, &length) && length == 0, "missing callback");
    length = 123;
    CHECK(!GetObjectProperty(&stages, ESP_TYPE, NULL, &length) && length == 0,
        "unsupported container property fails without debug break");
}

static void TestBezierAndEmpty(void)
{
    FLOAT points[] = { 0, 0, 1, 2, 3, 2, 4, 0 };
    BYTE flags[] = { 0, 3, 3, 1 };
    PSPathPipeLine pipeline = PathPipeLineCreate();
    PSPathStageEntry entry;
    CHECK(PathPipeLineAddLogicalLine(pipeline, points, flags, 4, TRUE, TRUE), "Bezier source");
    entry = PathPipeLineGetLastStage(pipeline);
    CheckGeometry(entry, (PSLineItem)entry->StageData, LineItemGetProperty);
    CHECK(PathPipeLineFlattenizeLogicalLine(pipeline), "Bezier flatten");
    entry = PathPipeLineGetLastStage(pipeline);
    CheckGeometry(entry, (PSLineItem)entry->StageData, FlattenItemGetProperty);
    PathPipeLineDestroy(pipeline);
    {
        SPathStages stages;
        PSLineItem empty = LineItemAllocate(0, LineItemType_Flatten, FALSE);
        PathStagesInitialize(&stages);
        CHECK(PathStagesAddStage(&stages, PathStageTypeLogicalCurve, empty,
            LineItemFreeWithPoints, LineItemGetProperty), "empty geometry stage");
        CheckGeometry(PathStagesGetLastStage(&stages), empty, LineItemGetProperty);
        PathStagesUninitialize(&stages);
    }
}

int main(void)
{
    TestPipeline();
    TestCustomStageAndErrors();
    TestBezierAndEmpty();
    printf("Stage property tests: %s (%d failures)\n", failures ? "FAIL" : "PASS", failures);
    return failures ? 1 : 0;
}
