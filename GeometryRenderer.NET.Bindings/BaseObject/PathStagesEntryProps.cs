namespace GeometryRenderer.NET.Bindings.BaseObject
{
    internal enum PathStagesEntryProps
    {
        Bad,
        Type,

        LogicalItemPropsStart,

        IsClosed = LogicalItemPropsStart + 1,

        IsBeziere,

        PathLen,

        PathData,

        PathFlags,

        DashItemPropsStart = 9,

        DashCount,

        DashOffset,
    }
}
