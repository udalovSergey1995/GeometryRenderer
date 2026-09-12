using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Security.Permissions;
using System.Text;

namespace GeometryRenderer.NET.Bindings
{
    public static class Native
    {
        /// <summary>
        /// Нативная функция для получения свойства объекта
        /// </summary>
        /// <param name="pObject">Указатель на объект</param>
        /// <param name="eProp">Идентификатор свойства</param>
        /// <param name="pOutData">Указатель на буфер для выходных данных (может быть NULL)</param>
        /// <param name="pOutLen">Указатель на размер выходных данных (вход/выход)</param>
        /// <returns>0 при успехе, иначе код ошибки</returns>
        [DllImport(
            NtivePinvokeDefs.GeometryRendererDll, 
            CallingConvention = CallingConvention.Cdecl)]
        private static extern int GetObjectProperty(
            IntPtr pObject,
            int eProp,
            IntPtr pOutData,
            ref int pOutLen
        );

        [DllImport(
            NtivePinvokeDefs.GeometryRendererDll,
            CallingConvention = CallingConvention.StdCall,
            EntryPoint = "PathPipeLineCreate")]
        public static extern IntPtr PathPipeLineCreate();


        [DllImport(
            NtivePinvokeDefs.GeometryRendererDll,
            CallingConvention = CallingConvention.StdCall,
            EntryPoint = "PathPipeLineDestroy")]
        public static extern void PathPipeLineDestroy(IntPtr pPipeline);

        /// <summary>
        /// Создает объект пайплайна рендеринга пути.
        /// </summary>
        /// <param name="points">
        /// Массив координат {x,y,x,y,...}. Длина = <paramref name="count"/> * 2.
        /// </param>
        /// <param name="types">
        /// Массив типов точек. Длина = <paramref name="count"/>.
        /// ВАЖНО: передается как int[] (4 байта на элемент) из-за бага в нативном коде,
        /// где PBYTE приводится к PLINE_POINT_TYPE (int*).
        /// </param>
        /// <param name="count">Количество точек (не пар координат!).</param>
        /// <param name="isBezier">TRUE (1) если путь содержит кривые Безье.</param>
        /// <param name="isClose">TRUE (1) если путь замкнутый.</param>
        /// <returns>Необработанный указатель на SPathPipeLine или IntPtr.Zero при ошибке.</returns>
        [DllImport(
            NtivePinvokeDefs.GeometryRendererDll,
            CallingConvention = CallingConvention.StdCall,
            EntryPoint = "PathPipeLineAddLogicalLine")]
        public static extern int PathPipeLineAddLogicalLine(
            IntPtr pPipeLine,
            [In] IntPtr points,
            [In, MarshalAs(UnmanagedType.LPArray)] byte[] types,
            int count,
            int isBezier,
            int isClose);

        [DllImport(
            NtivePinvokeDefs.GeometryRendererDll,
            CallingConvention = CallingConvention.StdCall,
            EntryPoint = "PathPipeLineAddLogicalLine")]
        public static extern int PathPipeLineAddLogicalLine(
            IntPtr pPipeLine,
            [In, MarshalAs(UnmanagedType.LPArray)] float[] points,
            [In, MarshalAs(UnmanagedType.LPArray)] byte[] types,
            int count,
            int isBezier,
            int isClose);
    }
}
