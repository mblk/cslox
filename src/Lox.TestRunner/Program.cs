using System.Diagnostics;
using System.Runtime.InteropServices;

namespace Lox.TestRunner;

// dotnet run --property:Configuration=Release -- cslox
// dotnet run --configuration Release -- cslox

public static class Program
{
    private record Settings(DirectoryInfo TestsDir, FileInfo TesteeFile, TestDefinition TestDefinition, string SkipLang);

    public static int Main(string[] args)
    {
        try
        {
            var settings = ParseArgs(args);

            var success = RunTests(settings);

            if (Debugger.IsAttached)
            {
                Console.WriteLine("Press any key to quit ...");
                Console.ReadKey();
            }

            return success ? 0 : -1;
        }
        catch (Exception e)
        {
            Console.Error.WriteLine($"Unhandled error: {e}");
            return -1;
        }
    }

    private static Settings ParseArgs(IReadOnlyList<string> args)
    {
        var repoRoot = FindRepoRoot();

        DirectoryInfo testsDir;
        FileInfo testeeFile;
        TestDefinition testDefinition;
        string skipLang;

        if (args.Count != 1) throw new ArgumentException($"Invalid argument count");

        switch (args[0])
        {
            case "clox":
                testsDir = new DirectoryInfo(Path.Combine(repoRoot.FullName, "scripts", "test_clox"));
                testeeFile = new FileInfo(Path.Combine(repoRoot.FullName, "clox", "clox"));
                testDefinition = TestDefinitionProvider.GetCloxTestDefinition();
                skipLang = "java";
                break;

            case "cslox":
                {
                    testsDir = new DirectoryInfo(Path.Combine(repoRoot.FullName, "scripts", "test_cslox"));
                    testeeFile = GetCsLoxTesteeFile(repoRoot);
                    testDefinition = TestDefinitionProvider.GetCsloxTestDefinition();
                    skipLang = "c";
                    break;
                }

            default:
                throw new ArgumentException($"Invalid argument");
        }

        if (!testsDir.Exists) throw new ArgumentException($"Tests dir does not exist: {testsDir.FullName}");
        if (!testeeFile.Exists) throw new ArgumentException($"Testee file does not exist: {testeeFile.FullName}");

        return new Settings(testsDir, testeeFile, testDefinition, skipLang);
    }

    private static DirectoryInfo FindRepoRoot()
    {
        DirectoryInfo? current = new(Environment.CurrentDirectory);
        while (current != null)
        {
            if (current.GetDirectories(".git").Length != 0)
            {
                return current;
            }
            current = current.Parent;
        }
        throw new InvalidOperationException($"Can't find repo root directory (started search at {Environment.CurrentDirectory})");
    }

    private static FileInfo GetCsLoxTesteeFile(DirectoryInfo repoRoot)
    {
        string buildConfig =
#if DEBUG
        "Debug";
#else
        "Release";
#endif

        string fileName;
        if (RuntimeInformation.IsOSPlatform(OSPlatform.Windows))
        {
            fileName = "Lox.exe";
        }
        else
        {
            fileName = "Lox";
        }

        return new FileInfo(Path.Combine(repoRoot.FullName, "src", "Lox", "bin", buildConfig, "net9.0", fileName));
    }

    private static bool RunTests(Settings settings)
    {
        var numTotal = 0;
        var numSuccess = 0;
        var numFailed = 0;

        foreach (var testCase in settings.TestDefinition.TestCases)
        {
            if (RunTestCase(settings, testCase))
            {
                numSuccess++;
            }
            else
            {
                numFailed++;
            }
            numTotal++;
        }

        Console.WriteLine();
        Console.WriteLine($"Total:  {numTotal}");
        Console.WriteLine($"OK:     {numSuccess}");
        Console.WriteLine($"Failed: {numFailed}");

        return numSuccess == numTotal;
    }

    private static bool RunTestCase(Settings settings, TestCase testCase)
    {
        Console.Write($"Running [{testCase.Group}/{testCase.Name}] -> ");

        var testFile = new FileInfo(Path.Combine(settings.TestsDir.FullName, testCase.Group, $"{testCase.Name}.lox"));
        if (!testFile.Exists)
        {
            Console.WriteLine($"Test file not found: {testFile.FullName}");
            return false;
        }

        var args = testCase.Type switch
        {
            TestCaseType.Scanning => "-scan",
            TestCaseType.Parsing => "-parse",
            TestCaseType.Running => "",
            _ => "",
        };

        var expectedOutputs = new TestFileParser(testFile.FullName, settings.SkipLang)
            .Parse()
            .ToArray();

        var (outputLines, exitCode) = new InterpreterRunner(settings.TesteeFile)
            .Run(testFile.FullName, args);

        var validator = new TestValidator(expectedOutputs, outputLines);

        // TODO check exit code: should indicate failure if the compiler/interpreter reported an error.
        _ = exitCode;

        if (validator.Validate())
        {
            PrintWithColor(ConsoleColor.Green, "Success");
            return true;
        }
        else
        {
            PrintWithColor(ConsoleColor.Red, "Failed");
            return false;
        }
    }

    private static void PrintWithColor(ConsoleColor color, string text)
    {
        var oldColor = Console.ForegroundColor;
        try
        {
            Console.ForegroundColor = color;
            Console.WriteLine(text);
        }
        finally
        {
            Console.ForegroundColor = oldColor;
        }
    }
}
