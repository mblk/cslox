using System.Text.RegularExpressions;

namespace Lox.TestRunner;

public record ExpectedOutput(string Output);

public partial class TestFileParser
{
    // expect: false
    [GeneratedRegex("// expect: (.*)", RegexOptions.IgnoreCase, "en-US")]
    private static partial Regex ExpectOutputRegex();



    // Error at '=': Invalid assignment target.
    [GeneratedRegex("// (Error.*)", RegexOptions.IgnoreCase, "en-US")]
    private static partial Regex ExpectErrorRegex();



    // [line 2] Error at ';': Expect property name after '.'.
    [GeneratedRegex(@"// \[line (\d+)\] (Error.*)", RegexOptions.IgnoreCase, "en-US")]
    private static partial Regex ExpectErrorWithLineRegex();



    // expect runtime error: Operands must be two numbers or two strings.
    [GeneratedRegex("// expect runtime error: (.+)", RegexOptions.IgnoreCase, "en-US")]
    private static partial Regex ExpectRuntimeErrorRegex();



    // TODO syntax error?
    //[GeneratedRegex(@"\[.*line (\d+)\] (Error.+)", RegexOptions.IgnoreCase, "en-US")]
    //private static partial Regex ExpectSyntaxErrorRegex();

    // TODO stacktrace?
    //[GeneratedRegex(@"\[line (\d+)\]", RegexOptions.IgnoreCase, "en-US")]
    //private static partial Regex ExpectStackTraceRegex();

    // TODO nontest?


    private readonly string _fileName;

    public TestFileParser(string fileName)
    {
        _fileName = fileName;
    }

    public IEnumerable<ExpectedOutput> Parse()
    {
        var lines = File.ReadAllLines(_fileName);
        var currentLineNum = 0;

        foreach (var line in lines)
        {
            currentLineNum++;

            Match match;

            match = ExpectOutputRegex().Match(line);
            if (match.Success)
            {
                var value = match.Groups[1].Value;
                yield return new ExpectedOutput(value);
                continue;
            }

            match = ExpectErrorRegex().Match(line);
            if (match.Success)
            {
                var value = match.Groups[1].Value;
                yield return new ExpectedOutput($"[{currentLineNum}] {value}"); // Automatically set the line number
                continue;
            }

            match = ExpectErrorWithLineRegex().Match(line);
            if (match.Success)
            {
                var lineNum = match.Groups[1].Value;
                var errMsg = match.Groups[2].Value;

                yield return new ExpectedOutput($"[{lineNum}] {errMsg}");
                continue;
            }

            match = ExpectRuntimeErrorRegex().Match(line);
            if (match.Success)
            {
                var errMsg = match.Groups[1].Value;

                yield return new ExpectedOutput($"RuntimeError: {errMsg}");
                continue;
            }


            if (line.Contains("//") && (line.Contains("error") || line.Contains("Error")))
            {
                Console.WriteLine();
                Console.WriteLine($"!! Unhandled comment: {line}");
                Console.WriteLine();
            }

            //match = ExpectSyntaxErrorRegex().Match(line);
            //if (match.Success)
            //{
            //    Console.WriteLine("...");
            //    yield return new ExpectedOutput($"...");
            //}

            //match = ExpectStackTraceRegex().Match(line);
            //if (match.Success)
            //{
            //    Console.WriteLine("...");
            //    yield return new ExpectedOutput($"...");
            //}
        }
    }
}