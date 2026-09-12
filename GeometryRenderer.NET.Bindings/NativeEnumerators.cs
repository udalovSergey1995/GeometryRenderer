using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;

namespace GeometryRenderer.NET.Bindings
{
    internal static class NativeEnumerators
    {
        [DllImport(
            NtivePinvokeDefs.GeometryRendererDll,
            CallingConvention = CallingConvention.StdCall,
            EntryPoint = "PathPipeEnumPathstages")]
        private static extern IntPtr PathPipeEnumPathstages(IntPtr pPipeline, IntPtr pCurrentEntry);

        internal static IEnumerable<IntPtr> PathPipeEnumPathstages(IntPtr pPipeline)
        {
            IntPtr pCurrentEntry = IntPtr.Zero;

            do
            {
                pCurrentEntry = PathPipeEnumPathstages(pPipeline, pCurrentEntry);

                if (pCurrentEntry == IntPtr.Zero)
                    yield break;

                yield return pCurrentEntry;

            } while (pCurrentEntry != IntPtr.Zero);
        }

        [DllImport(
            NtivePinvokeDefs.GeometryRendererDll,
            CallingConvention = CallingConvention.StdCall,
            EntryPoint = "PathPipeLineGetLastStage")]
        internal static extern IntPtr PathPipeLineGetLastStage(IntPtr pPipeline);
    }
}
