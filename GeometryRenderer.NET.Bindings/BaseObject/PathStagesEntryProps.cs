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

        DashItemPropsStart,

        DashCount = DashItemPropsStart + 1,

        DashOffset,
    }
}
