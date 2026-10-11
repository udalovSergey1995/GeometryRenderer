using GeometryRenderer.NET.Bindings.BaseObject;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Drawing;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;

namespace GeometryRenderer.NET.Bindings.Types
{
/// <summary>
/// Типы точек пути. Соответствуют LINE_POINT_TYPE в нативном коде.
/// </summary>
public enum LinePointType : int
{
    Move = 0x0,
    Line = 0x1,
    BezierControl = 0x3
}

public static class LinePointTypeEx
{
    public static LinePointType
    MakeClose(this LinePointType type)
    {
        byte closeFlag = 0x80;
        return (LinePointType)((byte)type | closeFlag);
    }
}

public enum EPathStageType : int
{
    PathStageTypeInvalid = 0,
    PathStageTypeLogicalCurve,
    PathStageTypeApproximated,
    PathStageTypeDashPattern,
    PathStageTypeThickLine,
    PathStageTypeRasterSlab
}

public enum ThickLineJoin : int
{
    Miter = 0,
    Round = 1,
    Bevel = 2
}

public enum ThickLineCap
{
    Flat = 0, // Butt
    Round = 1,
    Square = 2
}

public class PipeLineStage
{
    protected IntPtr _nativeObject = IntPtr.Zero;
    private readonly PipeLineObject _owner;
    // Raw-pointer constructors require external ownership. Pipeline stages retain their owner.
    public PipeLineStage(IntPtr nativeObject)
    {
        _nativeObject = nativeObject;
    }
    internal
    PipeLineStage(IntPtr nativeObject, PipeLineObject owner)
    {
        _nativeObject = nativeObject;
        _owner = owner;
    }
    protected T
    ReadNative<T>(Func<IntPtr, T> read)
    {
        if (_owner != null)
            return _owner.WithNative(pointer => read(_nativeObject));
        if (_nativeObject == IntPtr.Zero)
            throw new ObjectDisposedException(GetType().Name);
        return read(_nativeObject);
    }
    public EPathStageType StageType => (EPathStageType)ReadNative(
        pointer => BaseNativeObject.GetObjectPropertyInt(pointer, (int)PathStagesEntryProps.Type));
    public IntPtr
    GetNativeObject()
    {
        return ReadNative(pointer => pointer);
    }
}
public class PipeLogicalLineStage : PipeLineStage
{
    internal
    PipeLogicalLineStage(IntPtr pointer, PipeLineObject owner)
        : base(pointer, owner)
    {
    }
    public bool IsBeziere => ReadNative(pointer => BaseNativeObject.GetObjectPropertyInt(
                                            pointer, (int)PathStagesEntryProps.IsBeziere)) == 1;

    public bool IsClosed => ReadNative(pointer => BaseNativeObject.GetObjectPropertyInt(
                                           pointer, (int)PathStagesEntryProps.IsClosed)) == 1;

    public int PathLen => ReadNative(pointer => BaseNativeObject.GetObjectPropertyInt(
                                         pointer, (int)PathStagesEntryProps.PathLen));

    public GRNativePoint[] Path => ReadNative(pointer => BaseNativeObject.GetObjectPropertyPath(
                                                  pointer, (int)PathStagesEntryProps.PathData));

    public byte[] PathFlags => ReadNative(pointer => BaseNativeObject.GetObjectPropertyBytes(
                                              pointer, (int)PathStagesEntryProps.PathFlags));

    public PipeLogicalLineStage(IntPtr nativeOblect) : base(nativeOblect)
    {
    }
}

public class PipeApproximatedLineStage : PipeLogicalLineStage
{
    internal
    PipeApproximatedLineStage(IntPtr pointer, PipeLineObject owner)
        : base(pointer, owner)
    {
    }
    public PipeApproximatedLineStage(IntPtr nativeOblect) : base(nativeOblect)
    {
    }
}

public class PipeDashedLineStage : PipeLogicalLineStage
{
    internal
    PipeDashedLineStage(IntPtr pointer, PipeLineObject owner)
        : base(pointer, owner)
    {
    }
    public int DashCount => ReadNative(pointer => BaseNativeObject.GetObjectPropertyInt(
                                           pointer, (int)PathStagesEntryProps.DashCount));

    public float DashOffset => ReadNative(pointer => BaseNativeObject.GetObjectPropertyFloat(
                                              pointer, (int)PathStagesEntryProps.DashOffset));

    public PipeDashedLineStage(IntPtr nativeOblect) : base(nativeOblect)
    {
    }
}

public class PipeThickLineStage : PipeLogicalLineStage
{
    internal
    PipeThickLineStage(IntPtr pointer, PipeLineObject owner)
        : base(pointer, owner)
    {
    }
    /// <summary>Допуск дуг в единицах координат пути.</summary>
    public float ArcTolerance => ReadNative(pointer => BaseNativeObject.GetObjectPropertyFloat(
                                                pointer, (int)PathStagesEntryProps.ArcTolerance));

    public float Thickness => ReadNative(pointer => BaseNativeObject.GetObjectPropertyFloat(
                                             pointer, (int)PathStagesEntryProps.Thicknes));

    public int JoinStyle => ReadNative(pointer => BaseNativeObject.GetObjectPropertyInt(
                                           pointer, (int)PathStagesEntryProps.JounStyle));

    public int CapStyle => ReadNative(pointer => BaseNativeObject.GetObjectPropertyInt(
                                          pointer, (int)PathStagesEntryProps.CapStyle));

    public float MiterLimit => ReadNative(pointer => BaseNativeObject.GetObjectPropertyFloat(
                                              pointer, (int)PathStagesEntryProps.MiterLimit));

    public PipeThickLineStage(IntPtr nativeOblect) : base(nativeOblect)
    {
    }
}

[System.Runtime.InteropServices.StructLayout(System.Runtime.InteropServices.LayoutKind.Sequential)]
public struct RasterSlab
{
    public float Y, Height;
    public float XLeftTop, XLeftBottom, XRightTop, XRightBottom;
}

public class PipeRasterSlabStage : PipeLineStage
{
    internal
    PipeRasterSlabStage(IntPtr pointer, PipeLineObject owner)
        : base(pointer, owner)
    {
    }
    public PipeRasterSlabStage(IntPtr nativeObject) : base(nativeObject)
    {
    }
    public int SlabCount => ReadNative(pointer => BaseNativeObject.GetObjectPropertyInt(
                                           pointer, (int)PathStagesEntryProps.SlabCount));
    public RasterSlab[] Slabs
    {
        get {
            byte[] bytes = ReadNative(pointer => BaseNativeObject.GetObjectPropertyBytes(
                                          pointer, (int)PathStagesEntryProps.SlabData));
            int size = System.Runtime.InteropServices.Marshal.SizeOf(typeof(RasterSlab));
            if (bytes.Length % size != 0)
                throw new InvalidOperationException("Invalid slab data size");
            var slabs = new RasterSlab[bytes.Length / size];
            if (bytes.Length == 0)
                return slabs;
            var handle = System.Runtime.InteropServices.GCHandle.Alloc(
                slabs, System.Runtime.InteropServices.GCHandleType.Pinned);
            try
            {
                System.Runtime.InteropServices.Marshal.Copy(
                    bytes, 0, handle.AddrOfPinnedObject(), bytes.Length);
            }
            finally
            {
                handle.Free();
            }
            return slabs;
        }
    }
    public float
    GetCoverage(float x,
                float y) => ReadNative(pointer => Native.RasterSlabGetCoverage(pointer, x, y));
}

public class PipeLineObject : IDisposable
{
    private sealed class PipelineHandle
        : Microsoft.Win32.SafeHandles.SafeHandleZeroOrMinusOneIsInvalid
    {
        internal
        PipelineHandle()
            : base(true)
        {
            SetHandle(Native.PathPipeLineCreate());
        }
        protected override bool
        ReleaseHandle()
        {
            Native.PathPipeLineDestroy(handle);
            return true;
        }
    }
    private readonly PipelineHandle _handle;
    private readonly object _syncRoot = new object();
    private IntPtr _nativeObject
    {
        get {
            EnsureAlive();
            return _handle.DangerousGetHandle();
        }
    }
    private void
    EnsureAlive()
    {
        if (_handle.IsClosed || _handle.IsInvalid)
            throw new ObjectDisposedException("PipeLineObject");
    }
    internal T
    WithNative<T>(Func<IntPtr, T> operation)
    {
        lock (_syncRoot)
        {
            EnsureAlive();
            bool retained = false;
            try
            {
                _handle.DangerousAddRef(ref retained);
                return operation(_handle.DangerousGetHandle());
            }
            finally
            {
                if (retained)
                    _handle.DangerousRelease();
            }
        }
    }
    public int EntriesCount =>
        WithNative(pointer => NativeEnumerators.PathPipeEnumPathstages(pointer).Count());
    public IEnumerable<PipeLineStage> StageList
    {
        get {
            return WithNative(
                pointer =>
                {
                    var stages = new List<PipeLineStage>();
                    foreach (var item in NativeEnumerators.PathPipeEnumPathstages(pointer))
                    {
                        var type = (EPathStageType)BaseNativeObject.GetObjectPropertyInt(
                            item, (int)PathStagesEntryProps.Type);
                        switch (type)
                        {
                        case EPathStageType.PathStageTypeLogicalCurve:
                            stages.Add(new PipeLogicalLineStage(item, this));
                            break;
                        case EPathStageType.PathStageTypeApproximated:
                            stages.Add(new PipeApproximatedLineStage(item, this));
                            break;
                        case EPathStageType.PathStageTypeDashPattern:
                            stages.Add(new PipeDashedLineStage(item, this));
                            break;
                        case EPathStageType.PathStageTypeThickLine:
                            stages.Add(new PipeThickLineStage(item, this));
                            break;
                        case EPathStageType.PathStageTypeRasterSlab:
                            stages.Add(new PipeRasterSlabStage(item, this));
                            break;
                        default:
                            stages.Add(new PipeLineStage(item, this));
                            break;
                        }
                    }
                    return stages;
                });
        }
    }
    public PipeLineStage CurrentStage => StageList.Last();

    /// <summary>
    /// Создать объект графического пайплайна
    /// </summary>
    public PipeLineObject()
    {
        _handle = new PipelineHandle();
        if (_handle.IsInvalid)
        {
            _handle.Dispose();
            throw new InvalidOperationException("Native allocation failed");
        }
    }

    public void
    AddPathData(PointF[] points, byte[] ptTypes, bool isBeziere = false, bool isClosed = false)
    {
        try
        {
            lock (_syncRoot)
            {
                EnsureAlive();
                if (points.Length != ptTypes.Length)
                    throw new Exception("Not equals array len");

                unsafe
                {
                    fixed(PointF *pPoints = points)
                    {
                        Native.PathPipeLineAddLogicalLine(_nativeObject,
                                                          new IntPtr((float *)pPoints),
                                                          ptTypes,
                                                          points.Length,
                                                          isBeziere ? 1 : 0,
                                                          isClosed ? 1 : 0);
                    }
                }
            }
        }
        finally
        {
            GC.KeepAlive(this);
        }
    }

    public void
    AddPathData(float[] points, byte[] ptTypes, bool isBeziere = false, bool isClosed = false)
    {
        try
        {
            lock (_syncRoot)
            {
                EnsureAlive();
                if (points.Length / 2 != ptTypes.Length)
                    throw new Exception("Not equals array len");

                int result = 0;

                unsafe
                {
                    fixed(float *pPoints = points)
                    {
                        result = Native.PathPipeLineAddLogicalLine(_nativeObject,
                                                                   new IntPtr(pPoints),
                                                                   ptTypes,
                                                                   points.Length / 2,
                                                                   isBeziere ? 1 : 0,
                                                                   isClosed ? 1 : 0);
                    }
                }

                if (result == 0)
                {
                    throw new Exception("Init path error");
                }
            }
        }
        finally
        {
            GC.KeepAlive(this);
        }
    }

    public void
    AddBezierePath(float[] points, byte[] ptTypes, bool isClosed = false)
    {
        try
        {
            lock (_syncRoot)
            {
                EnsureAlive();
                AddPathData(points, ptTypes, true, isClosed);
            }
        }
        finally
        {
            GC.KeepAlive(this);
        }
    }

    public void
    AddLinePath(float[] points, byte[] ptTypes, bool isClosed = false)
    {
        try
        {
            lock (_syncRoot)
            {
                EnsureAlive();
                AddPathData(points, ptTypes, false, isClosed);
            }
        }
        finally
        {
            GC.KeepAlive(this);
        }
    }

    /// <summary>
    /// Автоматически решит какую линию добавлять на основе типов точек
    /// </summary>
    /// <param name="points"></param>
    /// <param name="ptTypes"></param>
    /// <param name="isClosed"></param>
    public void
    AddPath(float[] points, byte[] ptTypes, bool isClosed = false)
    {
        try
        {
            lock (_syncRoot)
            {
                EnsureAlive();
                foreach (var flag in ptTypes)
                {
                    if (flag == (byte)LinePointType.BezierControl)
                    {
                        AddBezierePath(points, ptTypes, isClosed);
                        return;
                    }
                }

                AddLinePath(points, ptTypes, isClosed);
            }
        }
        finally
        {
            GC.KeepAlive(this);
        }
    }

    public void
    FlettenizePath()
    {
        try
        {
            lock (_syncRoot)
            {
                EnsureAlive();
                if (CurrentStage.StageType != EPathStageType.PathStageTypeLogicalCurve)
                {
                    throw new Exception("Текущее состояние пайплайна недопускает данную операцию");
                }

                int result = Native.PathPipeLineFlattenizeLogicalLine(_nativeObject);

                if (result != 1)
                {
                    throw new Exception("Ошибка апроксимации");
                }
            }
        }
        finally
        {
            GC.KeepAlive(this);
        }
    }

    public void
    SetDashPatten(float[] pattern, float offset = 0.0f)
    {
        try
        {
            lock (_syncRoot)
            {
                EnsureAlive();
                if (CurrentStage.StageType != EPathStageType.PathStageTypeApproximated)
                {
                    throw new Exception("Текущее состояние пайплайна недопускает данную операцию");
                }

                int result = Native.PathPipeLineApplyDashPattern(
                    _nativeObject, pattern, pattern.Length, offset);

                if (result != 1)
                {
                    throw new Exception("Ошибка апроксимации");
                }
            }
        }
        finally
        {
            GC.KeepAlive(this);
        }
    }

    public void
    SetThickLine(float thickness,
                 ThickLineJoin join = ThickLineJoin.Miter,
                 ThickLineCap cap = ThickLineCap.Flat,
                 float miterLimit = 10.0f)
    {
        try
        {
            lock (_syncRoot)
            {
                EnsureAlive();
                SetThickLine(thickness, join, cap, miterLimit, 0.1f);
            }
        }
        finally
        {
            GC.KeepAlive(this);
        }
    }

    public void
    SetThickLine(
        float thickness, ThickLineJoin join, ThickLineCap cap, float miterLimit, float arcTolerance)
    {
        try
        {
            lock (_syncRoot)
            {
                EnsureAlive();
                if (CurrentStage.StageType != EPathStageType.PathStageTypeDashPattern &&
                    CurrentStage.StageType != EPathStageType.PathStageTypeApproximated)
                {
                    throw new Exception("Текущее состояние пайплайна недопускает данную операцию");
                }

                int result = Native.PathPipeLineApplyThickLineEx(
                    _nativeObject, thickness, (int)join, (int)cap, miterLimit, arcTolerance);

                if (result != 1)
                {
                    throw new Exception("Ошибка построения обводки: некорректные параметры, " +
                                        "нехватка памяти или превышение лимитов геометрии");
                }
            }
        }
        finally
        {
            GC.KeepAlive(this);
        }
    }

    public void
    ClosePath()
    {
        try
        {
            lock (_syncRoot)
            {
                EnsureAlive();
            }
        }
        finally
        {
            GC.KeepAlive(this);
        }
    }

    public void
    RasterizeSlabs()
    {
        try
        {
            lock (_syncRoot)
            {
                EnsureAlive();
                if (Native.PathPipeLineRasterizeSlabs(_nativeObject) != 1)
                    throw new InvalidOperationException(
                        "Slab rasterization failed: unsupported geometry, invalid coordinates or " +
                        "resource limit");
            }
        }
        finally
        {
            GC.KeepAlive(this);
        }
    }

    /// <summary>Source-over в BGRA32 с premultiplied alpha. Слэбы создаются
    /// автоматически.</summary>
    public void
    DrawSlabs(byte[] pixels, int width, int height, int stride, Color color)
    {
        try
        {
            lock (_syncRoot)
            {
                EnsureAlive();
                if (pixels == null)
                    throw new ArgumentNullException("pixels");
                if (Native.PathPipeLineDrawSlabs(_nativeObject,
                                                 pixels,
                                                 pixels.Length,
                                                 width,
                                                 height,
                                                 stride,
                                                 unchecked((uint)color.ToArgb())) != 1)
                    throw new InvalidOperationException(
                        "Slab rendering failed: invalid buffer or geometry");
            }
        }
        finally
        {
            GC.KeepAlive(this);
        }
    }

    public void
    Dispose()
    {
        lock (_syncRoot)
        {
            _handle.Dispose();
        }
    }
}
}
