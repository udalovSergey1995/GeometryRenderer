#include "GeometryRendererLibInclude.h"
#include <stdio.h>
static int failures;
#define CHECK(c) do { if (!(c)) { printf("FAIL slab line %d\n", __LINE__); ++failures; } } while (0)
static void Test(FLOAT* points, BYTE* flags, UINT n, FLOAT area, FLOAT pixelX, FLOAT pixelY, FLOAT coverage)
{
    PSPathPipeLine pipe = PathPipeLineCreate();
    PSPathStageEntry stage;
    PSRasterSlabItem item;
    UINT i;
    double sum = 0, pixels = 0;
    int x, y, len, count;
    CHECK(PathPipeLineAddLogicalLine(pipe, points, flags, n, FALSE, FALSE));
    CHECK(PathPipeLineRasterizeSlabs(pipe));
    stage = PathPipeLineGetLastStage(pipe);
    if (stage->StageType != PathStageTypeRasterSlab) { PathPipeLineDestroy(pipe); return; }
    item = (PSRasterSlabItem)stage->StageData;
    for (i = 0; i < item->Count; ++i)
    {
        SRasterSlab* s = item->Slabs + i;
        CHECK(s->Height > 0 && s->Height <= 1);
        CHECK(floorf(s->Y) == floorf(s->Y + s->Height * 0.999f));
        CHECK(s->XRightTop + 1e-6f >= s->XLeftTop && s->XRightBottom + 1e-6f >= s->XLeftBottom);
        sum += s->Height * ((double)s->XRightTop - s->XLeftTop + s->XRightBottom - s->XLeftBottom) / 2;
    }
    CHECK(fabs(sum - area) < 1e-5);
    for (y = -2; y < 6; ++y) for (x = -2; x < 6; ++x) pixels += RasterSlabGetCoverage(stage, (FLOAT)x, (FLOAT)y);
    CHECK(fabs(pixels - area) < 1e-5);
    CHECK(fabsf(RasterSlabGetCoverage(stage, pixelX, pixelY) - coverage) < 1e-5f);
    len = 0;
    CHECK(GetObjectProperty(stage, ESP_SLAB_DATA, NULL, &len) && len == (INT)(item->Count * sizeof(SRasterSlab)));
    CHECK(GetObjectProperty(stage, ESP_SLAB_COUNT, &count, &len) && count == (INT)item->Count);
    CHECK(!PathPipeLineRasterizeSlabs(pipe) && pipe->m_sPathStages.Count == 2);
    PathPipeLineDestroy(pipe);
}
int main(void)
{
    FLOAT thin[] = { 0, .1f, 2, .1f, 2, .3f, 0, .3f };
    FLOAT triangle[] = { 0, 0, 2, 0, 0, 2 };
    FLOAT bow[] = { 0, 0, 2, 2, 0, 2, 2, 0 };
    FLOAT negative[] = { -.5f, -.5f, .5f, -.5f, .5f, .5f, -.5f, .5f };
    FLOAT hole[] = { 0, 0, 4, 0, 4, 4, 0, 4, 1, 1, 1, 3, 3, 3, 3, 1 };
    FLOAT overlap[] = { 0, 0, 2, 0, 2, 2, 0, 2, 1, 0, 3, 0, 3, 2, 1, 2 };
    BYTE four[] = { 0, 1, 1, 0x81 }, three[] = { 0, 1, 0x81 };
    BYTE eight[] = { 0, 1, 1, 0x81, 0, 1, 1, 0x81 };
    Test(thin, four, 4, .4f, 0, 0, .2f);
    Test(triangle, three, 3, 2, 1, 0, .5f);
    Test(bow, four, 4, 2, 0, 0, .5f);
    Test(negative, four, 4, 1, -1, -1, .25f);
    Test(hole, eight, 8, 12, 1, 1, 0);
    Test(overlap, eight, 8, 6, 1, 0, 1);
    {
        FLOAT line[] = { 0, 0, 10, 0 };
        BYTE flags[] = { 0, 1 };
        PSPathPipeLine pipe = PathPipeLineCreate();
        CHECK(PathPipeLineAddLogicalLine(pipe, line, flags, 2, FALSE, FALSE));
        CHECK(PathPipeLineApplyThickLine(pipe, 2, ThickLineJoin_Miter, ThickLineCap_Flat, 4));
        CHECK(PathPipeLineRasterizeSlabs(pipe));
        CHECK(RasterSlabGetCoverage(PathPipeLineGetLastStage(pipe), 4, 0) == 1);
        CHECK(RasterSlabGetCoverage(PathPipeLineGetLastStage(pipe), 4, 1) == 0);
        PathPipeLineDestroy(pipe);
    }
    {
        FLOAT points[] = { NAN, 0, 1, 0, 1, 1 };
        PSPathPipeLine pipe = PathPipeLineCreate();
        CHECK(PathPipeLineAddLogicalLine(pipe, points, three, 3, FALSE, FALSE));
        CHECK(!PathPipeLineRasterizeSlabs(pipe) && pipe->m_sPathStages.Count == 1);
        PathPipeLineDestroy(pipe);
    }
    printf("Raster slab tests: %s (%d failures)\n", failures ? "FAIL" : "PASS", failures);
    return failures ? 1 : 0;
}
