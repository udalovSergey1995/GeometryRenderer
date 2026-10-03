using GeometryRenderer.NET.Bindings;
using GeometryRenderer.NET.Bindings.Types;
using System.Drawing.Drawing2D;

namespace GeometryRenderer.NET.TestApp
{
    public partial class Form1 : Form
    {
        public Form1()
        {
            InitializeComponent();

            this.Paint += Form1_Paint;
        }

        private static PointF[] GetPts(GRNativePoint[] nPts)
        {
            var pts = new PointF[nPts.Length];

            for (int i = 0; i < nPts.Length; i++) 
            {
                pts[i].X = nPts[i].X;
                pts[i].Y = nPts[i].Y;
            }

            return pts;
        }

        private void Form1_Paint(object? sender, PaintEventArgs e)
        {
            using (var pl = new PipeLineObject())
            {
                float mult = 3f;

                float[] points = new float[]
                {
                    5.0f * mult,   5.0f * mult,   // P0  (Move)
                    50.0f * mult,  100.0f * mult, // C1  (BezierControl)
                    150.0f * mult, 100.0f * mult, // C2  (BezierControl)
                    200.0f * mult, 5.0f * mult,    // P3  (Line)

                    5.0f * mult,   95.0f * mult,   // P0  (Move)
                    50.0f * mult,  190.0f * mult, // C1  (BezierControl)
                    150.0f * mult, 190.0f * mult, // C2  (BezierControl)
                    200.0f * mult, 95.0f * mult,    // P3  (Line)
                };

                byte[] types = new[]
                {
                    (byte)LinePointType.Move,
                    (byte)LinePointType.BezierControl,
                    (byte)LinePointType.BezierControl,
                    (byte)LinePointType.Line.MakeClose(),

                    (byte)LinePointType.Move,
                    (byte)LinePointType.BezierControl,
                    (byte)LinePointType.BezierControl,
                    (byte)LinePointType.Line.MakeClose(),
                };

                pl.AddPath(points, types, false);

                var plType = pl.CurrentStage.StageType;

                var pts = (pl.CurrentStage as PipeLogicalLineStage)?.Path;

                var flags = (pl.CurrentStage as PipeLogicalLineStage)?.PathFlags;

                pl.FlettenizePath();

                plType = pl.CurrentStage.StageType;

                pts = (pl.CurrentStage as PipeApproximatedLineStage)?.Path;

                flags = (pl.CurrentStage as PipeApproximatedLineStage)?.PathFlags;

                pl.SetDashPatten(new[] { 10f * mult, 10f * mult, 5f * mult, 30f * mult });

                plType = pl.CurrentStage.StageType;

                pts = (pl.CurrentStage as PipeDashedLineStage)?.Path;

                flags = (pl.CurrentStage as PipeDashedLineStage)?.PathFlags;

                var wPts = GetPts(pts);

                var gp = new GraphicsPath(wPts, flags);

                e.Graphics.DrawPath(Pens.Red, gp);

            }
        }
    }
}

