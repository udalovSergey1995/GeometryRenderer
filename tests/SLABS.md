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

## Rendering

`PathPipeLineDrawSlabs` / `PipeLineObject.DrawSlabs(pixels, width, height, stride, color)`
blend a solid color into a caller-owned BGRA32 buffer with premultiplied alpha
(compatible with `Format32bppPArgb`). Stride must be positive and at least width*4;
buffer length must cover every row. Buffer validation precedes pipeline changes.
The buffer is not cleared; transparent black is the initial value for an empty image.

If the latest item is not RasterSlab, rendering generates slabs automatically.
Logical Bezier paths are flattened first. Existing RasterSlab stages are reused.
Coverage is accumulated for a row, clipped to the image, then blended source-over
once per pixel, including both paint opacity and geometric coverage. Row padding
is untouched. This function produces pixels; displaying the image is the caller's job.

On a returned failure, DrawSlabs restores the stage list to its original tail.
Any newly appended approximated/raster stages are removed in reverse order through
their FreeCallback. Previously existing stages and their addresses are preserved.
The destination buffer is unchanged on failure: validation and the coverage-buffer
allocation finish before any pixel writes; the drawing loop has no fallible allocations.
A retry after failure can reuse the same pipeline.

The native container helper `PathStagesRollbackTo(stages, checkpoint)` supports
append-only transactions. NULL means an originally empty list. A foreign checkpoint
is rejected before deletion. The caller must retain the checkpoint and prevent
concurrent mutations; this helper does not restore modifications to existing items.
