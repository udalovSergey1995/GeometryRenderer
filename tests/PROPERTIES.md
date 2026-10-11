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
`pOutLen` remains an output parameter; it does not enforce buffer capacity.
Unknown properties return failure and a zero length. Empty arrays return success
with a zero length. Property copies do not allocate temporary native memory.

The tests exercise all four stages, scalar and array properties, size queries,
Bezier metadata, empty geometry, errors, and a custom stage callback without
adding a central type switch. C# tests exercise the rebuilt DLL. `DashOffset`
is now a `float`, matching the native property (callers expecting `int` must adapt).
