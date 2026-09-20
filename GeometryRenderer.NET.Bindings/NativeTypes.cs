using System.Runtime.InteropServices;

namespace GeometryRenderer.NET.Bindings
{
    [StructLayout(LayoutKind.Sequential)]
    public struct GRNativePoint
    {
        public float X { get; set; }

        public float Y { get; set; }

        public override string ToString()
        {
            return $"x{X}:{Y}";
        }
    }
}
