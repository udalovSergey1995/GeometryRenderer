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

            DoubleBuffered = true;

            this.Paint += Form1_Paint;
            this.SizeChanged += Form1_SizeChanged;
        }

        private void Form1_SizeChanged(object? sender, EventArgs e)
        {
            Invalidate();
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
            DrawIosDesktop(e.Graphics);
            DrawGeometryDemo(e);
        }

        private void DrawIosDesktop(Graphics graphics)
        {
            float w = ClientSize.Width, h = ClientSize.Height;
            if (w <= 0 || h <= 0) return;
            float unit = Math.Min(w, h);
            PointF P(float x, float y) => new PointF(w * x, h * y);
            var saved = graphics.Save();
            try
            {
                graphics.SmoothingMode = SmoothingMode.AntiAlias;
                using var white = new SolidBrush(Color.White);
                using var textFont = new Font("Segoe UI", unit * 0.025f, FontStyle.Regular, GraphicsUnit.Pixel);
                using var textFormat = new StringFormat { Alignment = StringAlignment.Center };

                // GraphicsPath только принимает готовые точки генератора для заливки.
                // Все обводки строятся SetThickLine; GDI+ Pen/DrawPath не используются.
                void Draw(PointF[] points, Brush brush, bool closed = false, float thickness = 0,
                    ThickLineJoin join = ThickLineJoin.Round, ThickLineCap cap = ThickLineCap.Round,
                    bool bezier = false, float[]? dash = null)
                {
                    using var pipeline = new PipeLineObject();
                    var coords = new float[points.Length * 2];
                    var types = new byte[points.Length];
                    for (int i = 0; i < points.Length; i++)
                    {
                        coords[i * 2] = points[i].X;
                        coords[i * 2 + 1] = points[i].Y;
                        types[i] = i == 0 ? (byte)0 : bezier && i % 3 != 0 ? (byte)3 : (byte)1;
                    }
                    if (closed && thickness == 0) types[types.Length - 1] |= 0x80;
                    pipeline.AddPath(coords, types, closed);
                    if (bezier || thickness > 0) pipeline.FlettenizePath();
                    if (dash != null) pipeline.SetDashPatten(dash);
                    if (thickness > 0) pipeline.SetThickLine(thickness, join, cap, 4, unit * 0.0002f);
                    var stage = (PipeLogicalLineStage)pipeline.CurrentStage;
                    var result = stage.Path;
                    if (result.Length == 0) return;
                    using var path = new GraphicsPath(GetPts(result), stage.PathFlags, FillMode.Winding);
                    graphics.FillPath(brush, path);
                }

                void Box(float x, float y, float width, float height, Brush brush)
                    => Draw(new[] { P(x, y), P(x + width, y), P(x + width, y + height), P(x, y + height) }, brush, true);
                void Label(string text, float x, float y)
                    => graphics.DrawString(text, textFont, white, P(x, y), textFormat);

                using var wallpaper = new SolidBrush(Color.FromArgb(40, 41, 91));
                Box(0, 0, 1, 1, wallpaper);
                using var lavender = new SolidBrush(Color.FromArgb(112, 139, 202));
                using var peach = new SolidBrush(Color.FromArgb(224, 147, 166));
                Draw(new[] { P(-0.2f, 0.25f), P(0.2f, -0.05f), P(0.6f, 0.7f), P(1.2f, 0.2f) },
                    lavender, thickness: unit * 0.55f, bezier: true);
                Draw(new[] { P(-0.2f, 0.85f), P(0.3f, 0.2f), P(0.7f, 1.3f), P(1.2f, 0.65f) },
                    peach, thickness: unit * 0.45f, bezier: true);

                Label("9:41", 0.09f, 0.035f);
                using var black = new SolidBrush(Color.FromArgb(20, 20, 28));
                Draw(new[] { P(0.43f, 0.055f), P(0.57f, 0.055f) }, black, thickness: h * 0.035f);
                for (int i = 0; i < 4; i++)
                    Draw(new[] { P(0.81f + i * 0.015f, 0.065f), P(0.81f + i * 0.015f, 0.055f - i * 0.006f) },
                        white, thickness: w * 0.006f, cap: ThickLineCap.Flat);
                Draw(new[] { P(0.90f, 0.045f), P(0.95f, 0.045f), P(0.95f, 0.07f), P(0.90f, 0.07f) },
                    white, true, unit * 0.003f, ThickLineJoin.Miter, ThickLineCap.Flat);
                Box(0.905f, 0.050f, 0.038f, 0.015f, white);

                float icon = Math.Min(w * 0.16f, h * 0.16f);
                void Tile(float cx, float cy, Brush brush)
                {
                    // Замкнутая центральная линия + Round join создают скруглённую рамку.
                    float a = icon * 0.31f;
                    var center = P(cx, cy);
                    Draw(new[] { new PointF(center.X - a, center.Y - a), new PointF(center.X + a, center.Y - a),
                        new PointF(center.X + a, center.Y + a), new PointF(center.X - a, center.Y + a) },
                        brush, true, icon * 0.38f);
                    Draw(new[] { new PointF(center.X - a, center.Y - a), new PointF(center.X + a, center.Y - a),
                        new PointF(center.X + a, center.Y + a), new PointF(center.X - a, center.Y + a) }, brush, true);
                }
                using var green = new SolidBrush(Color.FromArgb(55, 205, 100));
                using var blue = new SolidBrush(Color.FromArgb(35, 145, 235));
                Tile(0.25f, 0.24f, green);
                Tile(0.50f, 0.24f, blue);
                PointF I(float cx, float cy, float x, float y)
                {
                    var center = P(cx, cy);
                    return new PointF(center.X + icon * x, center.Y + icon * y);
                }
                Draw(new[] { I(0.25f, 0.24f, -0.20f, -0.08f), I(0.25f, 0.24f, 0.20f, -0.08f) },
                    white, thickness: icon * 0.40f);
                Draw(new[] { I(0.25f, 0.24f, -0.22f, 0.20f), I(0.25f, 0.24f, -0.20f, 0.03f),
                    I(0.25f, 0.24f, -0.04f, 0.09f) }, white, true);
                // Стрелка: Bevel join + Square cap.
                Draw(new[] { I(0.50f, 0.24f, -0.24f, 0.20f), I(0.50f, 0.24f, 0, -0.24f),
                    I(0.50f, 0.24f, 0.24f, 0.20f) }, white, thickness: icon * 0.09f,
                    join: ThickLineJoin.Bevel, cap: ThickLineCap.Square);
                Label("Сообщения", 0.25f, 0.24f + icon / h * 0.62f);
                Label("Навигация", 0.50f, 0.24f + icon / h * 0.62f);

                // Матрица 3x3: все комбинации join/cap на одинаковой ломаной.
                var joins = new[] { ThickLineJoin.Miter, ThickLineJoin.Round, ThickLineJoin.Bevel };
                var caps = new[] { ThickLineCap.Flat, ThickLineCap.Round, ThickLineCap.Square };
                for (int row = 0; row < 3; row++)
                    for (int col = 0; col < 3; col++)
                    {
                        float x = 0.18f + col * 0.32f, y = 0.46f + row * 0.10f;
                        Draw(new[] { P(x - 0.06f, y + 0.02f), P(x, y - 0.02f), P(x + 0.06f, y + 0.02f) },
                            white, thickness: unit * 0.018f, join: joins[row], cap: caps[col]);
                        Label($"{joins[row]} / {caps[col]}", x, y + 0.038f);
                    }
                using var dock = new SolidBrush(Color.FromArgb(75, 255, 255, 255));
                Draw(new[] { P(0.16f, 0.88f), P(0.84f, 0.88f) }, dock, thickness: h * 0.12f);
                Draw(new[] { P(0.25f, 0.88f), P(0.32f, 0.91f), P(0.39f, 0.86f) },
                    white, thickness: unit * 0.035f, join: ThickLineJoin.Round, cap: ThickLineCap.Round);
                Draw(new[] { P(0.56f, 0.88f), P(0.75f, 0.88f) }, white, thickness: unit * 0.025f,
                    cap: ThickLineCap.Flat, dash: new[] { unit * 0.035f, unit * 0.018f });
                Draw(new[] { P(0.39f, 0.979f), P(0.61f, 0.979f) }, white, thickness: h * 0.007f);
            }
            finally { graphics.Restore(saved); }
        }

        private void DrawGeometryDemo(PaintEventArgs e)
        {
            using (var pl = new PipeLineObject())
            {
                float mult = 3f;
                float thick = 90f;

                float[] points = new float[]
                {
                    thick,   thick,   // P0  (Move)
                    50f * mult ,  100.0f , // C1  (BezierControl)
                    this.Width - 150.0f , this.Height - 100.0f * mult, // C2  (BezierControl)
                    200.0f * mult, 5.0f * mult,    // P3  (Line)

                    thick,   95.0f * mult,   // P0  (Move)
                    50.0f * mult,  190.0f * mult, // C1  (BezierControl)
                    this.Width - 150.0f , this.Height - 190.0f * mult, // C2  (BezierControl)
                    200.0f * mult, 95.0f * mult,    // P3  (Line)
                };

                byte[] types = new[]
                {
                    (byte)LinePointType.Move,
                    (byte)LinePointType.BezierControl,
                    (byte)LinePointType.BezierControl,
                    (byte)LinePointType.Line,

                    (byte)LinePointType.Move,
                    (byte)LinePointType.BezierControl,
                    (byte)LinePointType.BezierControl,
                    (byte)LinePointType.Line,
                };

                pl.AddPath(points, types, false);

                var plType = pl.CurrentStage.StageType;

                var pts = (pl.CurrentStage as PipeLogicalLineStage)?.Path;

                var flags = (pl.CurrentStage as PipeLogicalLineStage)?.PathFlags;



                pl.FlettenizePath();

                plType = pl.CurrentStage.StageType;

                pts = (pl.CurrentStage as PipeApproximatedLineStage)?.Path;

                flags = (pl.CurrentStage as PipeApproximatedLineStage)?.PathFlags;



                if (1 == 1)
                {
                    pl.SetDashPatten(new[] { thick * mult, thick * mult });

                    plType = pl.CurrentStage.StageType;

                    pts = (pl.CurrentStage as PipeDashedLineStage)?.Path;

                    flags = (pl.CurrentStage as PipeDashedLineStage)?.PathFlags;
                }


                if (1 == 1)
                {
                    pl.SetThickLine(thick, ThickLineJoin.Miter, ThickLineCap.Round);

                    plType = pl.CurrentStage.StageType;

                    pts = (pl.CurrentStage as PipeThickLineStage)?.Path;

                    flags = (pl.CurrentStage as PipeThickLineStage)?.PathFlags;

                    var thck = (pl.CurrentStage as PipeThickLineStage)?.Thickness;
                }


                var wPts = GetPts(pts);

                var gp = new GraphicsPath(wPts, flags);

                if (plType == EPathStageType.PathStageTypeThickLine)
                {
                    gp.FillMode = FillMode.Winding;
                    var br = new SolidBrush(Color.FromArgb(150, Color.Red));
                    e.Graphics.FillPath(br, gp);
                }
                else
                {
                    e.Graphics.DrawPath(Pens.Red, gp);
                }
            }
        }
    }
}

