using System;
using System.Linq;
using GeometryRenderer.NET.Bindings;
using GeometryRenderer.NET.Bindings.Types;

internal static class StagePropertyBindingsTests
{
    private static PipeLogicalLineStage DetachedStage(out WeakReference owner)
    {
        var pipeline = new PipeLineObject();
        pipeline.AddLinePath(new float[] { 0, 0, 1, 1 }, new byte[] { 0, 1 });
        owner = new WeakReference(pipeline);
        return (PipeLogicalLineStage)pipeline.CurrentStage;
    }
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
            {
                var pipeline = new PipeLineObject();
                pipeline.AddLinePath(new float[] { 0, 0, 1, 1 }, new byte[] { 0, 1 });
                var stage = (PipeLogicalLineStage)pipeline.CurrentStage;
                bool failed = false;
                try { BaseNativeObject.GetObjectPropertyStruct<int>(stage.GetNativeObject(), 6); }
                catch (InvalidOperationException) { failed = true; }
                Check(failed, "Wrong property structure size rejected before copy");
                pipeline.Dispose();
                pipeline.Dispose();
                failed = false;
                try { var ignored = stage.Path; }
                catch (ObjectDisposedException) { failed = true; }
                Check(failed, "Stage access after owner Dispose is safe");
                failed = false;
                try { pipeline.FlettenizePath(); }
                catch (ObjectDisposedException) { failed = true; }
                Check(failed, "Pipeline use after Dispose is safe");
            }
            {
                WeakReference owner;
                var stage = DetachedStage(out owner);
                GC.Collect(); GC.WaitForPendingFinalizers(); GC.Collect();
                Check(owner.IsAlive && stage.PathLen == 2, "Stage keeps owner alive through GC");
                ((PipeLineObject)owner.Target).Dispose();
            }
            {
                var pipeline = new PipeLineObject();
                pipeline.AddLinePath(new float[] { 0, 0, 1, 1 }, new byte[] { 0, 1 });
                var stage = (PipeLogicalLineStage)pipeline.CurrentStage;
                var reader = System.Threading.Tasks.Task.Factory.StartNew(() =>
                {
                    for (int i = 0; i < 1000; i++)
                    {
                        try { Check(stage.PathLen == 2, "Concurrent property read"); }
                        catch (ObjectDisposedException) { break; }
                    }
                });
                pipeline.Dispose();
                reader.Wait();
            }
            using (var pipeline = new PipeLineObject())
            {
                pipeline.AddLinePath(new float[] { 0, .1f, 2, .1f, 2, .3f, 0, .3f }, new byte[] { 0, 1, 1, 0x81 });
                var pixels = new byte[8];
                pipeline.DrawSlabs(pixels, 2, 1, 8, System.Drawing.Color.Red);
                Check(pixels[2] == 51 && pixels[3] == 51, "Native slab render buffer");
                int count = pipeline.EntriesCount;
                pipeline.DrawSlabs(pixels, 2, 1, 8, System.Drawing.Color.Red);
                Check(pipeline.EntriesCount == count, "Raster stage reused");
            }
            return 0;
        }
        catch (Exception error)
        {
            Console.WriteLine("Stage property C# tests: FAIL: " + error);
            return 1;
        }
    }
}
