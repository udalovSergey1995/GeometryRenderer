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
        Move = 0,
        Line = 1,
        BezierControl = 2,
        Close = 3
    }

    public enum EPathStageType
    {
        PathStageTypeInvalid = 0,
        PathStageTypeLogicalCurve,
        PathStageTypeApproximated,
        PathStageTypeDashPattern,
        PathStageTypeThickLine
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

        public PipeLogicalLineStage(IntPtr nativeOblect) : base(nativeOblect)
        {}
    }

    public class PipeLineObject : IDisposable
    {
        private IntPtr _nativeObject = IntPtr.Zero;

        public int EntriesCount => NativeEnumerators.PathPipeEnumPathstages(_nativeObject).Count();

        private IEnumerable<IntPtr> nativeEntries => NativeEnumerators.PathPipeEnumPathstages(_nativeObject);

        public IEnumerable<PipeLineStage> Entries
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
                        case EPathStageType.PathStageTypeInvalid: break;

                        case EPathStageType.PathStageTypeLogicalCurve:
                            yield return new PipeLogicalLineStage(item);
                            break;
                        case EPathStageType.PathStageTypeApproximated:
                            break;
                        case EPathStageType.PathStageTypeDashPattern:
                            break;
                        case EPathStageType.PathStageTypeThickLine:
                            break;
                        default:
                            yield return new PipeLineStage(item);
                            break;
                    }
                }

                yield break;
            }
        }

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

        public void AddPath(float[] points, byte[] ptTypes, bool isClosed = false)
        {
            AddPathData(points, ptTypes, false, isClosed);
        }

        public void ClosePath()
        {

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
