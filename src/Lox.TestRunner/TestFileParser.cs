using System.Diagnostics;
using System.Text.RegularExpressions;

namespace Lox.TestRunner;

public record ExpectedOutput(string Output);

public partial class TestFileParser
{
    //
    // Expected output by print-statements:
    //
    // expect: false

    [GeneratedRegex("// expect: (.*)", RegexOptions.IgnoreCase, "en-US")]
    private static partial Regex ExpectOutputRegex();

    //
    // Errors raised by the Scanner or Parser or Resolver:
    //
    // Error: Invalid assignment target.
    // Error at '=': Invalid assignment target.
    // [1] Error at '=': Invalid assignment target.
    // [line 1] Error at '=': Invalid assignment target.
    // [c 1] Error at '=': Invalid assignment target.
    // [c line 1] Error at '=': Invalid assignment target.

    [GeneratedRegex(@"//\s*(\[(java|c)?\s*(line )?(\d+)\])?\s*((Error|ResolveError).*)", RegexOptions.IgnoreCase, "en-US")]
    private static partial Regex ExpectErrorRegex();

    //
    // Runtime errors:
    //
    // expect runtime error: Operands must be two numbers or two strings.

    [GeneratedRegex("// expect runtime error: (.+)", RegexOptions.IgnoreCase, "en-US")]
    private static partial Regex ExpectRuntimeErrorRegex();



    private readonly string _fileName;
    private readonly string _skipLang;

    public TestFileParser(string fileName, string skipLang)
    {
        _fileName = fileName;
        _skipLang = skipLang;
    }

    public IEnumerable<ExpectedOutput> Parse()
    {
        var lines = File.ReadAllLines(_fileName);
        var currentLineNum = 0;

        foreach (var line in lines)
        {
            currentLineNum++;

            Match match;

            // Expect output by print statement?
            match = ExpectOutputRegex().Match(line);
            if (match.Success)
            {
                var value = match.Groups[1].Value;
                yield return new ExpectedOutput(value);
                continue;
            }

            // Expect error by scanner/parser/resolver?
            match = ExpectErrorRegex().Match(line);
            if (match.Success)
            {
                var langGroup = match.Groups[2];
                var lineGroup = match.Groups[4];
                var errorGroup = match.Groups[5];

                if (langGroup.Success && langGroup.Value == _skipLang)
                {
                    //Console.WriteLine($"Skipping: {line}");
                    continue;
                }

                var lineNum = lineGroup.Success ? lineGroup.Value : currentLineNum.ToString(); // auto set line if not specified
                Debug.Assert(errorGroup.Success);

                var expectedOutput = $"[{lineNum}] {errorGroup.Value}";

                yield return new ExpectedOutput(expectedOutput);
                continue;
            }

            // Expect runtime error?
            match = ExpectRuntimeErrorRegex().Match(line);
            if (match.Success)
            {
                var errMsg = match.Groups[1].Value;

                yield return new ExpectedOutput($"RuntimeError: {errMsg}");
                continue;
            }

            // Unmatched comments?
            if (line.Contains("//"))
            {
                string[] words = [ "error", "expect", "runtime" ];

                if (words.Any(w => line.Contains(w, StringComparison.InvariantCultureIgnoreCase)))
                {
                    Console.WriteLine("!!");
                    Console.WriteLine($"!! Unhandled comment: {line}");
                    Console.WriteLine("!!");
                }
            }
        }
    }
}