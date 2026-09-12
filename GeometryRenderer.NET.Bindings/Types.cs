using System;
using System.Collections;
using System.Collections.Generic;
using System.Drawing;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;

namespace GeometryRenderer.NET.Bindings.Types
{
    public enum _EObjectType : int
    {
        EOT_BadObject = 0,

        EOT_PipeLine,

        EOT_PathStages,

        EOT_PipeStageEntry,

        EOT_TypesCount,
    }

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

    public class PipeLineObject : IDisposable
    {
        private IntPtr _nativeObject = IntPtr.Zero;

        public int EntriesCount => Native.PathPipeEnumPathstages(_nativeObject).Count();

        public IEnumerable<IntPtr> Entries => Native.PathPipeEnumPathstages(_nativeObject);

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
            if (points.Length/2 != ptTypes.Length)
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
                        points.Length,
                        isBeziere ? 1 : 0,
                        isClosed ? 1 : 0);
                }
            }

            if (result == 0)
            {
                throw new Exception("Init path error");
            }
        }

        public void Dispose()
        {
            if (_nativeObject == IntPtr.Zero)
                throw new Exception("Null native object");

            Native.PathPipeLineDestroy(_nativeObject);
        }
    }

    public class PathStageEntry
    {
        public int Type { get; set; }
    }
}
