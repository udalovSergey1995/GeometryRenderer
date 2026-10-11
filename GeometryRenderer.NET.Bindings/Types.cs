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
        public static LinePointType MakeClose(this LinePointType type)
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
        Flat = 0,    // Butt
        Round = 1,
        Square = 2
    }

    public class PipeLineStage
    {
        protected IntPtr _nativeObject = IntPtr.Zero;

        public EPathStageType StageType
            => (EPathStageType)BaseNativeObject.GetObjectPropertyInt(
                    _nativeObject,
                    (int)PathStagesEntryProps.Type);

        public PipeLineStage(IntPtr nativeOblect)
        {
            _nativeObject = nativeOblect;
        }

        public IntPtr GetNativeObject()
        {
            return _nativeObject;
        }
    }

    public class PipeLogicalLineStage : PipeLineStage
    {
        public bool IsBeziere
            => BaseNativeObject.GetObjectPropertyInt(
                    base._nativeObject,
                    (int)PathStagesEntryProps.IsBeziere) == 1;

        public bool IsClosed
            => BaseNativeObject.GetObjectPropertyInt(
                    base._nativeObject,
                    (int)PathStagesEntryProps.IsClosed) == 1;

        public int PathLen
            => BaseNativeObject.GetObjectPropertyInt(
                    base._nativeObject,
                    (int)PathStagesEntryProps.PathLen);

        public GRNativePoint[] Path 
            => BaseNativeObject.GetObjectPropertyPath(
                    base._nativeObject,
                    (int)PathStagesEntryProps.PathData);

        public byte[] PathFlags 
            => BaseNativeObject.GetObjectPropertyBytes(
                    base._nativeObject,
                    (int)PathStagesEntryProps.PathFlags);

        public PipeLogicalLineStage(IntPtr nativeOblect) : base(nativeOblect)
        {}
    }

    public class PipeApproximatedLineStage : PipeLogicalLineStage
    {
        public PipeApproximatedLineStage(IntPtr nativeOblect) : base(nativeOblect)
        {}
    }

    public class PipeDashedLineStage : PipeLogicalLineStage
    {
        public int DashCount
            => BaseNativeObject.GetObjectPropertyInt(
                    base._nativeObject,
                    (int)PathStagesEntryProps.DashCount);

        public float DashOffset
            => BaseNativeObject.GetObjectPropertyFloat(
                    base._nativeObject,
                    (int)PathStagesEntryProps.DashOffset);

        public PipeDashedLineStage(IntPtr nativeOblect) : base(nativeOblect)
        { }
    }

    public class PipeThickLineStage : PipeLogicalLineStage
    {
        /// <summary>Допуск дуг в единицах координат пути.</summary>
        public float ArcTolerance
            => BaseNativeObject.GetObjectPropertyFloat(
                    base._nativeObject,
                    (int)PathStagesEntryProps.ArcTolerance);

        public float Thickness
            => BaseNativeObject.GetObjectPropertyFloat(
                    base._nativeObject,
                    (int)PathStagesEntryProps.Thicknes);

        public int JoinStyle
            => BaseNativeObject.GetObjectPropertyInt(
                    base._nativeObject,
                    (int)PathStagesEntryProps.JounStyle);

        public int CapStyle
            => BaseNativeObject.GetObjectPropertyInt(
                    base._nativeObject,
                    (int)PathStagesEntryProps.CapStyle);

        public float MiterLimit
            => BaseNativeObject.GetObjectPropertyFloat(
                    base._nativeObject,
                    (int)PathStagesEntryProps.MiterLimit);

        public PipeThickLineStage(IntPtr nativeOblect) : base(nativeOblect)
        { }
    }

    [System.Runtime.InteropServices.StructLayout(System.Runtime.InteropServices.LayoutKind.Sequential)]
    public struct RasterSlab
    {
        public float Y, Height;
        public float XLeftTop, XLeftBottom, XRightTop, XRightBottom;
    }

    public class PipeRasterSlabStage : PipeLineStage
    {
        public PipeRasterSlabStage(IntPtr nativeObject) : base(nativeObject) { }
        public int SlabCount => BaseNativeObject.GetObjectPropertyInt(_nativeObject, (int)PathStagesEntryProps.SlabCount);
        public RasterSlab[] Slabs
        {
            get
            {
                byte[] bytes = BaseNativeObject.GetObjectPropertyBytes(_nativeObject, (int)PathStagesEntryProps.SlabData);
                int size = System.Runtime.InteropServices.Marshal.SizeOf(typeof(RasterSlab));
                if (bytes.Length % size != 0) throw new InvalidOperationException("Invalid slab data size");
                var slabs = new RasterSlab[bytes.Length / size];
                if (bytes.Length == 0) return slabs;
                var handle = System.Runtime.InteropServices.GCHandle.Alloc(slabs, System.Runtime.InteropServices.GCHandleType.Pinned);
                try { System.Runtime.InteropServices.Marshal.Copy(bytes, 0, handle.AddrOfPinnedObject(), bytes.Length); }
                finally { handle.Free(); }
                return slabs;
            }
        }
        public float GetCoverage(float x, float y) => Native.RasterSlabGetCoverage(_nativeObject, x, y);
    }

    public class PipeLineObject : IDisposable
    {
        private IntPtr _nativeObject = IntPtr.Zero;

        public int EntriesCount => NativeEnumerators.PathPipeEnumPathstages(_nativeObject).Count();

        private IEnumerable<IntPtr> nativeEntries => NativeEnumerators.PathPipeEnumPathstages(_nativeObject);

        public IEnumerable<PipeLineStage> StageList
        {
            get
            {
                foreach (var item in NativeEnumerators.PathPipeEnumPathstages(_nativeObject))
                {
                    EPathStageType type = (EPathStageType)BaseNativeObject.GetObjectPropertyInt(
                        item, 
                        (int)PathStagesEntryProps.Type);

                    switch (type)
                    {
                        case EPathStageType.PathStageTypeInvalid: 
                            break;

                        case EPathStageType.PathStageTypeApproximated:
                            yield return new PipeApproximatedLineStage(item);
                            break;

                        case EPathStageType.PathStageTypeLogicalCurve:
                            yield return new PipeLogicalLineStage(item);
                            break;

                        case EPathStageType.PathStageTypeDashPattern:
                            yield return new PipeDashedLineStage(item);
                            break;

                        case EPathStageType.PathStageTypeThickLine:
                            yield return new PipeThickLineStage(item);
                            break;

                        case EPathStageType.PathStageTypeRasterSlab:
                            yield return new PipeRasterSlabStage(item);
                            break;

                        default:
                            yield return new PipeLineStage(item);
                            break;
                    }
                }

                yield break;
            }
        }

        public PipeLineStage CurrentStage => StageList.Last();

        /// <summary>
        /// Создать объект графического пайплайна
        /// </summary>
        public PipeLineObject()
        {
            _nativeObject = Native.PathPipeLineCreate();

            if (_nativeObject == IntPtr.Zero)
                throw new Exception("Native error");
        }

        ~PipeLineObject()
        {
            this.Dispose();
        }

        public void AddPathData(PointF[] points, byte[] ptTypes, bool isBeziere = false, bool isClosed = false)
        {
            if (points.Length != ptTypes.Length)
                throw new Exception("Not equals array len");

            unsafe
            {
                fixed (PointF* pPoints = points)
                {
                    Native.PathPipeLineAddLogicalLine(
                        _nativeObject, 
                        new IntPtr((float*)pPoints), 
                        ptTypes, 
                        points.Length, 
                        isBeziere ? 1 : 0,
                        isClosed ? 1 : 0);
                }
            }
        }

        public void AddPathData(float[] points, byte[] ptTypes, bool isBeziere = false, bool isClosed = false)
        {
            if (points.Length / 2 != ptTypes.Length)
                throw new Exception("Not equals array len");

            int result = 0;

            unsafe
            {
                fixed (float* pPoints = points)
                {
                    result = Native.PathPipeLineAddLogicalLine(
                        _nativeObject,
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

        public void AddBezierePath(float[] points, byte[] ptTypes, bool isClosed = false)
        {
            AddPathData(points, ptTypes, true, isClosed);
        }

        public void AddLinePath(float[] points, byte[] ptTypes, bool isClosed = false)
        {
            AddPathData(points, ptTypes, false, isClosed);
        }

        /// <summary>
        /// Автоматически решит какую линию добавлять на основе типов точек
        /// </summary>
        /// <param name="points"></param>
        /// <param name="ptTypes"></param>
        /// <param name="isClosed"></param>
        public void AddPath(float[] points, byte[] ptTypes, bool isClosed = false)
        {
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

        public void FlettenizePath()
        {
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

        public void SetDashPatten(float[] pattern, float offset = 0.0f)
        {
            if (CurrentStage.StageType != EPathStageType.PathStageTypeApproximated)
            {
                throw new Exception("Текущее состояние пайплайна недопускает данную операцию");
            }

            int result = Native.PathPipeLineApplyDashPattern(
                _nativeObject,
                pattern,
                pattern.Length,
                offset);

            if (result != 1)
            {
                throw new Exception("Ошибка апроксимации");
            }
        }

        public void SetThickLine(float thickness,
                         ThickLineJoin join = ThickLineJoin.Miter,
                         ThickLineCap cap = ThickLineCap.Flat,
                         float miterLimit = 10.0f)
        {
            SetThickLine(thickness, join, cap, miterLimit, 0.1f);
        }

        public void SetThickLine(float thickness, ThickLineJoin join,
                         ThickLineCap cap, float miterLimit, float arcTolerance)
        {
            if (CurrentStage.StageType != EPathStageType.PathStageTypeDashPattern
                && CurrentStage.StageType != EPathStageType.PathStageTypeApproximated)
            {
                throw new Exception("Текущее состояние пайплайна недопускает данную операцию");
            }

            int result = Native.PathPipeLineApplyThickLineEx(
                _nativeObject,
                thickness,
                (int)join,
                (int)cap,
                miterLimit,
                arcTolerance);

            if (result != 1)
            {
                throw new Exception("Ошибка построения обводки: некорректные параметры, нехватка памяти или превышение лимитов геометрии");
            }
        }

        public void ClosePath()
        {

        }

        public void RasterizeSlabs()
        {
            if (Native.PathPipeLineRasterizeSlabs(_nativeObject) != 1)
                throw new InvalidOperationException("Slab rasterization failed: unsupported geometry, invalid coordinates or resource limit");
        }

        /// <summary>Source-over в BGRA32 с premultiplied alpha. Слэбы создаются автоматически.</summary>
        public void DrawSlabs(byte[] pixels, int width, int height, int stride, Color color)
        {
            if (pixels == null) throw new ArgumentNullException("pixels");
            if (Native.PathPipeLineDrawSlabs(_nativeObject, pixels, pixels.Length,
                width, height, stride, unchecked((uint)color.ToArgb())) != 1)
                throw new InvalidOperationException("Slab rendering failed: invalid buffer or geometry");
        }

        public void Dispose()
        {
            if (_nativeObject != IntPtr.Zero)
            {
                Native.PathPipeLineDestroy(_nativeObject);
                _nativeObject = IntPtr.Zero;
            }

            GC.SuppressFinalize(this);
        }
    }
}
