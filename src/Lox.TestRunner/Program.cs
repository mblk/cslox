using System.Diagnostics;

namespace Lox.TestRunner;

public static class Program
{
#if false
    public static void Main(string[] args)
    {
        var pathPrefix = @"C:\workspace\repos\cslox\scripts\test\";

        var di = new DirectoryInfo(pathPrefix);

        foreach (var fi in di.GetFiles("*.lox", SearchOption.AllDirectories))
        {
            var s = fi.FullName.Substring(pathPrefix.Length);

            var pathElements = s.Split(new char[] { '/', '\\' }, StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries);
            if (pathElements.Length == 2)
            {
                var group = pathElements[0];
                var name = pathElements[1][..^4];

                // ("scanning", "identifiers", TestCaseType.Scanning),

                Console.WriteLine($"(\"{group}\", \"{name}\", TestCaseType.Running),");
            }
            else
            {
                Console.WriteLine($"Unknown: {s}");
            }
        }
    }
#else
    public static int Main(string[] args)
    {
        var testDefinition = TestDefinitionProvider.GetTestDefinition();

        var numTotal = 0;
        var numSuccess = 0;
        var numFailed = 0;

        foreach (var testCase in testDefinition.TestCases)
        {
            if (RunTestCase(testCase))
                numSuccess++;
            else
                numFailed++;
            numTotal++;
        }

        Console.WriteLine();
        Console.WriteLine($"Total:  {numTotal}");
        Console.WriteLine($"OK:     {numSuccess}");
        Console.WriteLine($"Failed: {numFailed}");

        if (Debugger.IsAttached)
        {
            Console.WriteLine("Press any key to quit ...");
            Console.ReadKey();
        }

        return 0;
    }

    public static bool RunTestCase(TestCase testCase)
    {
        Console.WriteLine($"== {testCase.Group}: {testCase.Name} ==");

        var pathPrefix = @"C:\workspace\repos\cslox\scripts\test\";

        var fileInfo = new FileInfo(Path.Combine(pathPrefix, testCase.Group, $"{testCase.Name}.lox"));

        var args = testCase.Type switch
        {
            TestCaseType.Scanning => "-scan",
            TestCaseType.Parsing => "-parse",
            TestCaseType.Running => "",
            _ => "",
        };


        var testParser = new TestFileParser(fileInfo.FullName);
        var expectedOutputs = testParser.Parse().ToArray();

        var interpreterRunner = new InterpreterRunner();
        var (outputLines, exitCode) = interpreterRunner.Run(fileInfo.FullName, args); //.ToArray();

        var validator = new TestValidator(expectedOutputs, outputLines);
        if (exitCode != 0)
        {
            Console.WriteLine($"Exit code != 0, output:");
            foreach (var line in outputLines)
                Console.WriteLine(line);
            return false;
        }
        else if (validator.Validate())
        {
            //Console.WriteLine($"Success");
            return true;
        }
        else
        {
            Console.WriteLine($"Failed");
            return false;
        }
    }
#endif
}
