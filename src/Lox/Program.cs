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
        var lox = new Lox();

        lox.Run(source);
    }

    private static void RunRepl()
    {
        var lox = new Lox();

        while (true)
        {
            Console.Write("> ");
            var input = Console.ReadLine();
            if (String.IsNullOrWhiteSpace(input)) continue;

            lox.Run(input);
        }
    }
}

public class Lox
{
    private readonly Interpreter _interpreter = new();

    public Lox()
    {
    }

    public void Run(string source)
    {
        var scanner = new Scanner(source);
        var tokens = scanner.ScanTokens().ToArray();

        var parser = new Parser(tokens);
        var statements = parser.Parse();

        foreach (var statement in statements)
        {
            var s = statement.Accept(new PrintVisitor());
            Console.WriteLine($"Statement AST: {s}");
        }

        // TODO abort if error occurred

        _interpreter.Interpret(statements);

        //if (expr is not null)
        //{
        //    var s = expr.Accept(new PrintVisitor());
        //    Console.WriteLine($"AST: {s}");

        //    var value = _interpreter.Interpret(expr);
        //    Console.WriteLine($"Result: {value} ({value?.GetType().Name})");
        //}
    }
}