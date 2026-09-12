using GeometryRenderer.NET.Bindings.Types;

namespace GeometryRenderer.NET.TestApp
{
    internal static class Program
    {
        [STAThread]
        static void Main()
        {
            PathPipeLineCreateTests.RunAllTests();
            ApplicationConfiguration.Initialize();
            Application.Run(new Form1());
        }
    }
}