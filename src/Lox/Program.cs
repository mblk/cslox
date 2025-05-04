namespace Lox;

public static class Program
{
    public static int Main(string[] args)
    {
        Console.WriteLine($"Hello LOX!");

        if (args.Length == 1)
        {
            RunFile(args[0]);
            return 0;
        }
        else if (args.Length == 0)
        {
            RunRepl();
            return 0;
        }
        else
        {
            PrintUsage();
            return -1;
        }
    }

    private static void PrintUsage()
    {
        Console.WriteLine($"Usage: cslox [script]");
    }

    private static void RunFile(string fileName)
    {
        var source = File.ReadAllText(fileName);

        Run(source);
    }

    private static void RunRepl()
    {
        while (true)
        {
            Console.Write("> ");
            var input = Console.ReadLine();
            if (String.IsNullOrWhiteSpace(input)) continue;

            Run(input);
        }
    }

    private static void Run(string source)
    {
        var scanner = new Scanner(source);

        var tokens = scanner.ScanTokens().ToArray();

        //foreach (var token in tokens)
        //{
        //    Console.WriteLine($"{token}");
        //}

        var parser = new Parser(tokens);

        var expr = parser.Parse();

        if (expr is not null)
        {
            var s = expr.Accept(new PrintVisitor());
            Console.WriteLine($"AST: {s}");

            var value = new InterpreterVisitor().Interpret(expr);
            Console.WriteLine($"Result: {value} ({value?.GetType().Name})");
        }
    }
}
