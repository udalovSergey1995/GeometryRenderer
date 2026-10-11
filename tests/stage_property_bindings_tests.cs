using System;
using System.Linq;
using GeometryRenderer.NET.Bindings;
using GeometryRenderer.NET.Bindings.Types;

internal static class StagePropertyBindingsTests
{
    private static void Check(bool condition, string name)
    {
        if (!condition) throw new Exception(name);
    }

    public static int Main()
    {
        try
        {
            using (var pipeline = new PipeLineObject())
            {
                pipeline.AddLinePath(new float[] { 0, 0, 10, 0 }, new byte[] { 0, 1 });
                pipeline.FlettenizePath();
                pipeline.SetDashPatten(new float[] { 3, 2 }, 1.25f);
                var dashed = pipeline.CurrentStage as PipeDashedLineStage;
                Check(dashed != null, "Dash stage type");
                Check(dashed.DashCount == 2, "DashCount");
                Check(dashed.DashOffset == 1.25f, "Fractional DashOffset");
                pipeline.SetThickLine(4, ThickLineJoin.Bevel, ThickLineCap.Flat, 3.5f);
                var thick = pipeline.CurrentStage as PipeThickLineStage;
                Check(thick != null, "Thick stage type");
                Check(thick.Thickness == 4, "Thickness");
                Check(thick.MiterLimit == 3.5f, "MiterLimit");
                Check(thick.ArcTolerance == 0.1f, "Legacy ArcTolerance");
                Check(thick.JoinStyle == (int)ThickLineJoin.Bevel, "JoinStyle");
                Check(thick.CapStyle == (int)ThickLineCap.Flat, "CapStyle");
                Check(BaseNativeObject.GetObjectPropertyStruct<float>(thick.GetNativeObject(), 14) == 4,
                    "Generic property reader follows BOOL result convention");

                var stages = pipeline.StageList.Cast<PipeLogicalLineStage>().ToArray();
                Check(stages.Length == 4, "Stage enumeration");
                foreach (var stage in stages)
                {
                    Check(stage.PathLen == stage.Path.Length, "PathLen");
                    Check(stage.PathFlags.Length == stage.PathLen, "PathFlags");
                    Check(!stage.IsBeziere, "IsBeziere");
                    Check(!stage.IsClosed, "IsClosed metadata");
                }
                Check(stages[0].PathLen == 2 && stages[1].PathLen == 2, "Source path length");
            }
            using (var pipeline = new PipeLineObject())
            {
                pipeline.AddLinePath(new float[] { 0, 0, 10, 0 }, new byte[] { 0, 1 });
                pipeline.FlettenizePath();
                int count = pipeline.EntriesCount;
                bool failed = false;
                try { pipeline.SetThickLine(4, ThickLineJoin.Round, ThickLineCap.Round, 10, 0); }
                catch (Exception) { failed = true; }
                Check(failed && pipeline.EntriesCount == count, "Invalid tolerance leaves pipeline intact");
                pipeline.SetThickLine(4, ThickLineJoin.Round, ThickLineCap.Round, 10, 0.025f);
                var thick = (PipeThickLineStage)pipeline.CurrentStage;
                Check(thick.ArcTolerance == 0.025f, "Explicit tolerance via item callback");
                Check(thick.Path.Length == thick.PathLen, "Explicit tolerance geometry");
            }
            Console.WriteLine("Stage property C# tests: PASS");
            using (var pipeline = new PipeLineObject())
            {
                pipeline.AddLinePath(new float[] { 0, 0.1f, 2, 0.1f, 2, 0.3f, 0, 0.3f },
                    new byte[] { 0, 1, 1, 0x81 });
                pipeline.RasterizeSlabs();
                var stage = (PipeRasterSlabStage)pipeline.CurrentStage;
                Check(stage.SlabCount == stage.Slabs.Length && stage.SlabCount == 1, "Slab property layout");
                Check(Math.Abs(stage.GetCoverage(0, 0) - 0.2f) < 0.00001f, "Analytic slab coverage");
            }
            Console.WriteLine("Raster slab C# tests: PASS");
            return 0;
        }
        catch (Exception error)
        {
            Console.WriteLine("Stage property C# tests: FAIL: " + error);
            return 1;
        }
    }
}
