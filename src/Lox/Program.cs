namespace Lox;

public static class Program
{
    public static int Main(string[] args)
    {
        // TODO make/use generic arg-parser or keep it like this?

        Console.OutputEncoding = System.Text.Encoding.UTF8;

        if (args.Length == 2 && args[0] == "-scan")
        {
            return ScanFile(args[1]);
        }
        else if (args.Length == 2 && args[0] == "-parse")
        {
            return ParseFile(args[1]);
        }
        else if (args.Length == 1)
        {
            return RunFile(args[0]);
        }
        else if (args.Length == 0)
        {
            return RunRepl();
        }
        else
        {
            return PrintUsage();
        }
    }

    private static int PrintUsage()
    {
        Console.WriteLine($"Hello LOX!");
        Console.WriteLine($"");
        Console.WriteLine($"Usage: cslox [args] [filename]");
        Console.WriteLine($"");
        Console.WriteLine($"Optional args:");
        Console.WriteLine($"  -scan     Scan file and print list of tokens.");
        Console.WriteLine($"  -parse    Scan and parse file and print AST of statements.");
        return -1;
    }

    private static int ScanFile(string fileName)
    {
        var source = File.ReadAllText(fileName);
        var scanner = new Scanner(source);
        var tokens = scanner.ScanTokens().ToArray();

        foreach (var token in tokens)
        {
            Console.WriteLine(token);
        }

        return 0;
    }

    private static int ParseFile(string fileName)
    {
        var source = File.ReadAllText(fileName);
        var scanner = new Scanner(source);
        var tokens = scanner.ScanTokens().ToArray();
        var parser = new Parser(tokens);
        var statements = parser.Parse();

        foreach (var statement in statements)
        {
            var s = statement.Accept(new PrintVisitor());
            Console.WriteLine($"Statement AST: {s}");
        }

        return 0;
    }

    private static int RunFile(string fileName)
    {
        var source = File.ReadAllText(fileName);
        var lox = new Lox();

        lox.Run(source);

        return 0;
    }

    private static int RunRepl()
    {
        var lox = new Lox();

        while (true)
        {
            Console.Write("> ");
            var input = Console.ReadLine();
            if (String.IsNullOrWhiteSpace(input)) continue;

            lox.Run(input);
        }

        return 0;
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

        _interpreter.Interpret(statements);
    }
}
