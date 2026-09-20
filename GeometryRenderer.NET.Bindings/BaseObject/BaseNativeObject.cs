using GeometryRenderer.NET.Bindings.Types;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;

namespace GeometryRenderer.NET.Bindings
{
    public class BaseNativeObject
    {
        private const string GeometryRendererDll = "GeometryRenderer.dll";

        /// <summary>
        /// Нативная функция для получения свойства объекта
        /// </summary>
        /// <param name="pObject">Указатель на объект</param>
        /// <param name="eProp">Идентификатор свойства</param>
        /// <param name="pOutData">Указатель на буфер для выходных данных (может быть NULL)</param>
        /// <param name="pOutLen">Указатель на размер выходных данных (вход/выход)</param>
        /// <returns>0 при успехе, иначе код ошибки</returns>
        [DllImport(
            GeometryRendererDll,
            CallingConvention = CallingConvention.Cdecl)]
        internal static extern int GetObjectProperty(
            IntPtr pObject,
            int eProp,
            IntPtr pOutData,
            ref int pOutLen
        );

        public static byte[] GetObjectPropertyBytes(IntPtr pObject, int propertyId)
        {
            // Шаг 1: Получаем необходимый размер буфера
            int requiredSize = 0;
            int result = GetObjectProperty(pObject, propertyId, IntPtr.Zero, ref requiredSize);

            if (result == 0)
                throw new InvalidOperationException($"GetObjectProperty failed with code: {result}");

            if (requiredSize <= 0)
                return new byte[0];

            // Шаг 2: Выделяем буфер и получаем данные
            byte[] buffer = new byte[requiredSize];
            IntPtr ptr = Marshal.AllocHGlobal(requiredSize);

            try
            {
                int bufferSize = requiredSize;
                result = GetObjectProperty(pObject, propertyId, ptr, ref bufferSize);

                if (result == 0)
                    throw new InvalidOperationException($"GetObjectProperty failed with code: {result}");

                if (bufferSize > requiredSize)
                    throw new InvalidOperationException("Buffer size mismatch");

                // Копируем данные из неуправляемой памяти в управляемый массив
                Marshal.Copy(ptr, buffer, 0, bufferSize);

                // Если данные меньше буфера, обрезаем массив
                if (bufferSize < requiredSize)
                {
                    Array.Resize(ref buffer, bufferSize);
                }

                return buffer;
            }
            finally
            {
                Marshal.FreeHGlobal(ptr);
            }
        }

        public static GRNativePoint[] GetObjectPropertyPath(IntPtr pObject, int propertyId)
        {
            // Шаг 1: Получаем необходимый размер буфера
            int requiredSize = 0;
            int result = GetObjectProperty(pObject, propertyId, IntPtr.Zero, ref requiredSize);

            if (result != 1)
                throw new InvalidOperationException($"GetObjectProperty failed with code: {result}");

            if (requiredSize <= 0)
                return new GRNativePoint[0];

            requiredSize = requiredSize / 2 / sizeof(float);

            // Шаг 2: Выделяем буфер и получаем данные
            GRNativePoint[] buffer = new GRNativePoint[requiredSize];
            
            var handle = GCHandle.Alloc(buffer, GCHandleType.Pinned);
            IntPtr ptr = handle.AddrOfPinnedObject(); //Marshal.AllocHGlobal(requiredSize);

            try
            {
                int bufferSize = requiredSize;
                result = GetObjectProperty(pObject, propertyId, ptr, ref bufferSize);

                if (result == 0)
                    throw new InvalidOperationException($"GetObjectProperty failed with code: {result}");

                return buffer;
            }
            finally
            {
                handle.Free();
            }
        }

        /// <summary>
        /// Получить свойство как структуру (для блобных типов)
        /// </summary>
        public static T GetObjectPropertyStruct<T>(IntPtr pObject, int propertyId) where T : struct
        {
            int size = Marshal.SizeOf(typeof(T));
            IntPtr ptr = Marshal.AllocHGlobal(size);

            try
            {
                int bufferSize = size;
                int result = GetObjectProperty(pObject, propertyId, ptr, ref bufferSize);

                if (result != 0)
                    throw new InvalidOperationException($"GetObjectProperty failed with code: {result}");

                if (bufferSize != size)
                    throw new InvalidOperationException($"Expected size {size}, but got {bufferSize}");

                // Преобразуем указатель в структуру
                return (T)Marshal.PtrToStructure(ptr, typeof(T));
            }
            finally
            {
                Marshal.FreeHGlobal(ptr);
            }
        }

        /// <summary>
        /// Получить свойство как целое число
        /// </summary>
        public static int GetObjectPropertyInt(IntPtr pObject, int propertyId)
        {
            IntPtr ptr = Marshal.AllocHGlobal(sizeof(int));

            try
            {
                int bufferSize = sizeof(int);
                int result = GetObjectProperty(pObject, propertyId, ptr, ref bufferSize);

                if (result != 1)
                    throw new InvalidOperationException($"GetObjectProperty failed");

                if (bufferSize != sizeof(int))
                    throw new InvalidOperationException($"Expected {sizeof(int)} bytes, but got {bufferSize}");

                return Marshal.ReadInt32(ptr);
            }
            finally
            {
                Marshal.FreeHGlobal(ptr);
            }
        }

        /// <summary>
        /// Получить свойство как число с плавающей точкой
        /// </summary>
        public static float GetObjectPropertyFloat(IntPtr pObject, int propertyId)
        {
            IntPtr ptr = Marshal.AllocHGlobal(sizeof(float));

            try
            {
                int bufferSize = sizeof(float);
                int result = GetObjectProperty(pObject, propertyId, ptr, ref bufferSize);

                if (result != 0)
                    throw new InvalidOperationException($"GetObjectProperty failed with code: {result}");

                if (bufferSize != sizeof(float))
                    throw new InvalidOperationException($"Expected {sizeof(float)} bytes, but got {bufferSize}");

                return (float)Marshal.PtrToStructure(ptr, typeof(float));
            }
            finally
            {
                Marshal.FreeHGlobal(ptr);
            }
        }

        /// <summary>
        /// Получить свойство как строку (предполагается Unicode, либо измените Encoding)
        /// </summary>
        public static string GetObjectPropertyString(IntPtr pObject, int propertyId)
        {
            // Сначала получаем размер
            int requiredSize = 0;
            int result = GetObjectProperty(pObject, propertyId, IntPtr.Zero, ref requiredSize);

            if (result != 0)
                throw new InvalidOperationException($"GetObjectProperty failed with code: {result}");

            if (requiredSize <= 0)
                return string.Empty;

            // Выделяем буфер для строки (с учетом того, что строка может быть Unicode)
            // Обычно строки передаются как UTF-8 или Unicode (2 байта на символ)
            // В этом примере предполагаем Unicode, но проверьте документацию
            IntPtr ptr = Marshal.AllocHGlobal(requiredSize);

            try
            {
                int bufferSize = requiredSize;
                result = GetObjectProperty(pObject, propertyId, ptr, ref bufferSize);

                if (result != 0)
                    throw new InvalidOperationException($"GetObjectProperty failed with code: {result}");

                // Предполагаем Unicode строку (2 байта на символ)
                // Если строка UTF-8, используйте другие методы декодирования
                return Marshal.PtrToStringUni(ptr, bufferSize / 2);
            }
            finally
            {
                Marshal.FreeHGlobal(ptr);
            }
        }
    }
}
