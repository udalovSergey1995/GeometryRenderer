# Stage properties

Build `GeometryRenderer.slnx` in `Debug|x64`, then run
`tests\run_property_tests.cmd` from an **x64 Visual Studio Developer Command Prompt**.
The runner writes binaries to `%TEMP%\GeometryRendererPropertyTests`.

`SPathStageEntry.GetPropertyCallback` has type `GET_PATH_STAGE_PROPERTY`.
`PathStagesAddStage` receives that callback as its last argument. Every item
module declares its handler with the same signature as `PathStagesEntryGetProperty`.
The container dispatches without knowing item layouts or property namespaces.

Current handlers:

- `LineItemGetProperty`: logical path properties.
- `FlattenItemGetProperty`: delegates to the logical path reader.
- `DashPatternItemGetProperty`: dash settings and the nested `DashedLine` properties.
- `ThickLineItemGetProperty`: stroke settings and the nested `ThickGeometry` properties.

The exported `GetObjectProperty` API and property IDs are unchanged. Returns
`TRUE` (1) on success and `FALSE` (0) on failure. First query with `pOutData == NULL`
to obtain the size in bytes, then allocate a sufficient buffer and query again.
For exported `GetObjectProperty`, `pOutLen` is now input/output: set it to the
destination capacity before a copy. An undersized buffer fails without writing
and returns the required byte count. A NULL destination remains a size query.
Item callbacks and the internal `CopyObjectProperty` helper retain their
output-size contract and are invoked behind this capacity check.
Unknown properties return failure and a zero length. Empty arrays return success
with a zero length. Property copies do not allocate temporary native memory.

The tests exercise all four stages, scalar and array properties, size queries,
Bezier metadata, empty geometry, errors, and a custom stage callback without
adding a central type switch. C# tests exercise the rebuilt DLL. `DashOffset`
is now a `float`, matching the native property (callers expecting `int` must adapt).

`ArcTolerance` is owned by the thick item and read through its property callback.
It controls chord deviation for round joins/caps in path coordinate units;
it does not change Bezier flattening tolerance. Legacy creation uses `0.1`.
The `Ex` native calls and the five-argument C# `SetThickLine` accept an explicit
tolerance. Output uses `float`, so coordinate rounding limits precision.
Creation fails instead of relaxing the requested tolerance above 65,536 segments
per arc or 1,048,576 points per result. Geometry tests check Winding coverage,
joins, caps, reversals, short segments, intersections and actual chord deviation.

Managed stages returned by `PipeLineObject` retain their owner. The owner uses
SafeHandle and serializes reads, mutations and Dispose. Reading a stage after
explicit owner disposal throws ObjectDisposedException. Raw IntPtr constructors
and pointers obtained through GetNativeObject require external lifetime control;
they cannot make an arbitrary native pointer safe.

Reliability tests additionally exercise Bezier loops, Close flags, exact dash
boundaries, large offsets, guarded property buffers and injected realloc failures.
Zero, negative, nonfinite and <=0.0001 dash lengths are rejected; offset must be
finite. Bezier flattening propagates allocation/depth-limit failure atomically.
