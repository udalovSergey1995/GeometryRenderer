# Raster slab stage

`PathPipeLineRasterizeSlabs` / C# `PipeLineObject.RasterizeSlabs()` append a
`PathStageTypeRasterSlab` item. Input can be a linear logical path, approximated
path, dashed path or thick geometry. Bezier input must be flattened first.
All subpaths participate in one nonzero Winding fill; open figures close implicitly.
An empty/degenerate region succeeds with an empty array. Failure leaves the pipeline intact.

Each slab occupies 24 bytes: `Y`, `Height`, `XLeftTop`, `XLeftBottom`,
`XRightTop`, `XRightBottom`, all FLOAT. Slabs split at integer rows, vertices
and edge intersections. Height is positive and at most one. Coordinates should
already be transformed to the intended pixel space. Intermediate calculations
use double; output precision remains limited by FLOAT.

The item owns its data. `RasterSlabItemGetProperty` handles `ESP_TYPE`,
`ESP_SLAB_COUNT`, `ESP_SLAB_DATA` using the existing size-query/copy protocol.
C# `PipeRasterSlabStage.Slabs` returns a copied array.

`RasterSlabGetCoverage(stage, x, y)` / `stage.GetCoverage(x, y)` returns the
summed intersection area with `[x,x+1] x [y,y+1]`, clamped to [0,1]. Multiply
this coverage by paint opacity and blend once per pixel. This is analytic area
coverage, with no heuristic AA edge widths. Colors and bitmap output are not
part of this stage.

Initial implementation favors correctness: intersection discovery is O(E²),
each band scans edges, and each coverage query scans slabs. A row index / active
edge optimization is needed before using point queries for large full images.
Limits: 4,096 nonhorizontal edges, 1,048,576 events/slabs, finite coordinates
within ±1,000,000. Exceeding a limit fails rather than truncating output.

Build Debug|x64 and run `tests\run_property_tests.cmd` from an x64 Visual Studio
Developer Command Prompt. Tests cover fractional height, analytic pixel areas,
negative coordinates, holes, overlap, self intersections, thick-stage input,
failure atomicity and C# interop.
