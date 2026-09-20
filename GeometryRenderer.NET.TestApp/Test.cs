using GeometryRenderer.NET.Bindings;
using GeometryRenderer.NET.Bindings.Types;
using System;
using System.Collections.Generic;
using System.Reflection;
using System.Security.AccessControl;
using System.Text;

namespace GeometryRenderer.NET.TestApp
{
    /// <summary>
    /// Тестовый набор для проверки нативной функции PathPipeLineCreate.
    /// </summary>
    public static class PathPipeLineCreateTests
    {
        /// <summary>
        /// Запускает все тесты и выводит результаты в консоль.
        /// </summary>
        public static void RunAllTests()
        {
            Console.WriteLine("========================================");
            Console.WriteLine("  PathPipeLineCreate Native API Tests");
            Console.WriteLine("========================================");
            Console.WriteLine();

            TestLongPipeline();
            //TestSimpleLine();
            //TestBezierCurve();
            //TestClosedPath();
            //TestNullPoints();
            //TestEmptyCount();
            //TestLargePolyline();

            Console.WriteLine();
            Console.WriteLine("========================================");
            Console.WriteLine("  All tests completed.");
            Console.WriteLine("========================================");
        }

        private static void LogPass(string message)
        {
            Console.ForegroundColor = ConsoleColor.Green;
            Console.Write("  [PASS] ");
            Console.ResetColor();
            Console.WriteLine(message);
        }

        private static void LogFail(string message)
        {
            Console.ForegroundColor = ConsoleColor.Red;
            Console.Write("  [FAIL] ");
            Console.ResetColor();
            Console.WriteLine(message);
        }

        private static void LogInfo(string message)
        {
            Console.ForegroundColor = ConsoleColor.Cyan;
            Console.Write("  [INFO] ");
            Console.ResetColor();
            Console.WriteLine(message);
        }

        /// <summary>
        /// Тест 1: Простая линия из 2 точек.
        /// </summary>
        private static void TestSimpleLine()
        {
            Console.WriteLine("[Test] Simple line (Move + Line)");

            float[] points = new float[]
            {
                10.0f, 10.0f,
                100.0f, 100.0f
            };

            byte[] types = new byte[]
            {
                (byte)LinePointType.Move,
                (byte)LinePointType.Line
            };

            using (var pipeLine = new PipeLineObject())
            {
                pipeLine.AddPathData(points, types);
            }
        }

        /// <summary>
        /// Тест 2: Кубическая кривая Безье (1 сегмент: Move + 2 control + end).
        /// </summary>
        private static void TestBezierCurve()
        {
            Console.WriteLine("[Test] Cubic Bezier curve");

            float[] points = new float[]
            {
                0.0f,   0.0f,   // P0  (Move)
                50.0f,  100.0f, // C1  (BezierControl)
                150.0f, 100.0f, // C2  (BezierControl)
                200.0f, 0.0f    // P3  (Line)
            };

            byte[] types = new []
            {
                (byte)LinePointType.Move,
                (byte)LinePointType.BezierControl,
                (byte)LinePointType.BezierControl,
                (byte)LinePointType.Line
            };

            IntPtr pipeline = Native.PathPipeLineCreate();

            if (pipeline == IntPtr.Zero)
            {
                LogFail("Returned NULL for valid simple line");
                return;
            }

            int res = Native.PathPipeLineAddLogicalLine(pipeline, points, types, 4, 1, 0);

            if (res == 0)
            {
                LogFail("add error");
                return;
            }

            LogPass($"Bezier pipeline at 0x{pipeline.ToInt64():X16}");
        }

        /// <summary>
        /// Тест 3: Замкнутый треугольник.
        /// </summary>
        private static void TestClosedPath()
        {
            Console.WriteLine("[Test] Closed triangle");

            float[] points = new float[]
            {
                0.0f,   0.0f,
                100.0f, 0.0f,
                50.0f,  100.0f
            };

            byte[] types = new []
            {
                (byte)LinePointType.Move,
                (byte)LinePointType.Line,
                (byte)LinePointType.Line
            };

            IntPtr pipeline = Native.PathPipeLineCreate();

            if (pipeline == IntPtr.Zero)
            { 
                LogFail("Returned NULL for closed path");

                return;
            }

            int res = Native.PathPipeLineAddLogicalLine(pipeline, points, types, 3, 0, 1);

            if (res == 0)
            {
                LogFail("add error");
                return;
            }

            LogPass($"Closed path pipeline at 0x{pipeline.ToInt64():X16}");
        }

        /// <summary>
        /// Тест 4: NULL вместо точек — ожидаем NULL в ответ.
        /// </summary>
        private static void TestNullPoints()
        {
            Console.WriteLine("[Test] NULL points (negative)");

            IntPtr pipeline = Native.PathPipeLineCreate();

            if (pipeline == IntPtr.Zero)
            {
                LogFail("Returned NULL");
                return;
            }

            int res = Native.PathPipeLineAddLogicalLine(pipeline, null, null, 10, 0, 0);

            if (res == 0)
            {
                LogPass("Correctly returned NULL for NULL input");
            }
            else
            {
                LogFail($"Expected NULL, got 0x{pipeline.ToInt64():X16}");
            }
        }

        /// <summary>
        /// Тест 5: Нулевое количество точек — ожидаем NULL.
        /// </summary>
        private static void TestEmptyCount()
        {
            Console.WriteLine("[Test] Zero point count (negative)");

            float[] points = new float[] { 0.0f, 0.0f };
            byte[] types = new [] { (byte)LinePointType.Move };

            IntPtr pipeline = Native.PathPipeLineCreate();

            if (pipeline == IntPtr.Zero)
            {
                LogFail("Returned NULL");
                return;
            }

            int res = Native.PathPipeLineAddLogicalLine(pipeline, points, types, 0, 0, 0);

            if (res == 0)
            {
                LogPass("Correctly returned NULL for zero count");
            }
            else
            {
                LogFail($"Expected NULL, got 0x{pipeline.ToInt64():X16}");
            }
        }

        /// <summary>
        /// Тест 6: Большая полилиния (стресс-тест аллокации).
        /// </summary>
        private static void TestLargePolyline()
        {
            Console.WriteLine("[Test] Large polyline (1000 points)");

            const int count = 1000;
            float[] points = new float[count * 2];
            byte[] types = new byte[count];

            for (int i = 0; i < count; i++)
            {
                points[i * 2] = i * 10.0f;
                points[i * 2 + 1] = (float)(Math.Sin(i * 0.1) * 100.0);
                types[i] = (i == 0)
                    ? (byte)LinePointType.Move
                    : (byte)LinePointType.Line;
            }

            IntPtr pipeline = Native.PathPipeLineCreate();

            if (pipeline == IntPtr.Zero)
            {
                LogFail("Returned NULL");
                return;
            }

            int res = Native.PathPipeLineAddLogicalLine(pipeline, points, types, count, 0, 0);

            if (res == 0)
            {
                LogFail("add error");
                return;
            }

            LogPass($"Large pipeline at 0x{pipeline.ToInt64():X16}");
        }

        private static void TestLongPipeline()
        {
            using (var pl = new PipeLineObject())
            {
                Console.WriteLine("[Test] Cubic Bezier curve");

                float[] points = new float[]
                {
                    0.0f,   0.0f,   // P0  (Move)
                    50.0f,  100.0f, // C1  (BezierControl)
                    150.0f, 100.0f, // C2  (BezierControl)
                    200.0f, 0.0f    // P3  (Line)
                };

                byte[] types = new[]
                {
                    (byte)LinePointType.Move,
                    (byte)LinePointType.BezierControl,
                    (byte)LinePointType.BezierControl,
                    (byte)LinePointType.Line
                };

                pl.AddPath(points, types, false);

                var plType = pl.CurrentStage.StageType;

                var pts = (pl.CurrentStage as PipeLogicalLineStage)?.Path;

                var flags = (pl.CurrentStage as PipeLogicalLineStage)?.PathFlags;

                pl.FlettenizePath();

                plType = pl.CurrentStage.StageType;

                pts = (pl.CurrentStage as PipeApproximatedLineStage)?.Path;

                flags = (pl.CurrentStage as PipeApproximatedLineStage)?.PathFlags;

                pl.SetDashPatten(new[] { 1f, 1f });

                plType = pl.CurrentStage.StageType;

                pts = (pl.CurrentStage as PipeDashedLineStage)?.Path;

                flags = (pl.CurrentStage as PipeDashedLineStage)?.PathFlags;
            }
        }
    }
}
