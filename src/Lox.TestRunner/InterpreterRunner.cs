using System.Diagnostics;

namespace Lox.TestRunner;

public class InterpreterRunner
{
    private readonly FileInfo _testeeFile;

    public InterpreterRunner(FileInfo testeeFile)
    {
        if (!testeeFile.Exists) throw new ArgumentException($"Testee file does not exist: {testeeFile.FullName}");

        _testeeFile = testeeFile;
    }

    public (IReadOnlyList<string>, int) Run(string fileName, string args)
    {
        var startInfo = new ProcessStartInfo()
        {
            FileName = _testeeFile.FullName,
            Arguments = $"{args} \"{fileName}\"",
            UseShellExecute = false,
            RedirectStandardOutput = true,
            RedirectStandardError = true,
            CreateNoWindow = true,
        };

        var process = new Process()
        {
            StartInfo = startInfo,
        };

        _ = process.Start();

        var outputLines = new List<string>();

        while (!process.StandardOutput.EndOfStream)
        {
            var line = process.StandardOutput.ReadLine();
            if (ShouldIgnore(line)) continue;
            if (String.IsNullOrEmpty(line)) continue;

            outputLines.Add(line);
        }

        while (!process.StandardError.EndOfStream)
        {
            var line = process.StandardError.ReadLine();
            if (ShouldIgnore(line)) continue;
            if (String.IsNullOrEmpty(line)) continue;

            outputLines.Add(line);
        }

        process.WaitForExit();

        //Console.WriteLine($"Exit code: {process.ExitCode}");

        return (outputLines, process.ExitCode);
    }

    private static bool ShouldIgnore(string? line)
    {
        if (String.IsNullOrWhiteSpace(line)) return true;

        // part of stack trace:
        // RuntimeError: ...
        // [line 123] in ...
        // [line 123] in ...
        // [line 123] in ...

        var i1 = line.IndexOf('[');
        var i2 = line.IndexOf("] in ");

        if (i1 != -1 && i2 != -2 && i1 < i2)
        {
            //Console.WriteLine($"Ignoring line: {line}");
            return true;
        }

        return false;
    }
}
