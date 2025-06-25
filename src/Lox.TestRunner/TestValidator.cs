namespace Lox.TestRunner;

public class TestValidator
{
    private readonly IReadOnlyList<ExpectedOutput> _expectedOutputs;
    private readonly IReadOnlyList<string> _actualOutput;

    public TestValidator(IReadOnlyList<ExpectedOutput> expectedOutputs, IReadOnlyList<string> actualOutput)
    {
        _expectedOutputs = expectedOutputs;
        _actualOutput = actualOutput;
    }

    public bool Validate()
    {
        var expectedIndex = 0;
        var actualIndex = 0;

        while (true)
        {
            if (expectedIndex >= _expectedOutputs.Count) break;
            if (actualIndex >= _actualOutput.Count) break;

            var expected = _expectedOutputs[expectedIndex++];
            var actual = _actualOutput[actualIndex++];

            if (expected.Output == actual)
            {
                //Console.WriteLine($"Match {actual}");
            }
            else
            {
                Console.WriteLine($"Error:");
                Console.WriteLine($"  Expected: '{expected.Output}'");
                Console.WriteLine($"  Actual:   '{actual}'");

                Console.WriteLine($"Full output:");
                Console.WriteLine($"[start]");
                foreach (var s in _actualOutput) Console.WriteLine(s);
                Console.WriteLine($"[end]");

                return false;
            }
        }

        var isSuccessful = true;

        if (expectedIndex < _expectedOutputs.Count)
        {
            while (expectedIndex < _expectedOutputs.Count)
            {
                Console.WriteLine($"Missing expected output: '{_expectedOutputs[expectedIndex].Output}'");
                expectedIndex++;
            }

            isSuccessful = false;
        }

        if (actualIndex < _actualOutput.Count)
        {
            while (actualIndex < _actualOutput.Count)
            {
                Console.WriteLine($"Extra output which was not expected: '{_actualOutput[actualIndex]}'");
                actualIndex++;
            }

            isSuccessful = false;
        }

        return isSuccessful;
    }
}
