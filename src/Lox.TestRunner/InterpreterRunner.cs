﻿using System.Diagnostics;

namespace Lox.TestRunner;

public class InterpreterRunner
{
    public InterpreterRunner()
    {
    }

    public (IReadOnlyList<string>, int) Run(string fileName, string args)
    {
        var pathToLox = @"C:\workspace\repos\cslox\src\Lox\bin\Debug\net9.0\Lox.exe";

        var startInfo = new ProcessStartInfo()
        {
            FileName = pathToLox,
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
            if (String.IsNullOrEmpty(line)) continue;

            outputLines.Add(line);
        }

        while (!process.StandardError.EndOfStream)
        {
            var line = process.StandardError.ReadLine();
            if (String.IsNullOrEmpty(line)) continue;

            outputLines.Add($"Err: {line}");
        }

        process.WaitForExit();

        //Console.WriteLine($"Exit code: {process.ExitCode}");

        return (outputLines, process.ExitCode);
    }
}
