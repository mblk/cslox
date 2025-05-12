using System.Diagnostics;

namespace Lox;

public static class Program
{
    public static int Main(string[] args)
    {
        // TODO make/use generic arg-parser or keep it like this?

        try
        {
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
        finally
        {
            if (Debugger.IsAttached)
            {
                Console.WriteLine("Press any key to quit ...");
                Console.ReadKey();
            }
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
        var scanResult = scanner.ScanTokens();

        foreach (var scanError in scanResult.Errors)
        {
            Console.WriteLine(scanError);
        }

        foreach (var token in scanResult.Tokens)
        {
            Console.WriteLine(token);
        }

        return 0;
    }

    private static int ParseFile(string fileName)
    {
        var source = File.ReadAllText(fileName);
        var scanner = new Scanner(source);
        var scanResult = scanner.ScanTokens();

        foreach (var scanError in scanResult.Errors)
        {
            Console.WriteLine(scanError);
        }

        var parser = new Parser(scanResult.Tokens);
        var parseResult = parser.ParseStatements();

        foreach (var parseError in parseResult.Errors)
        {
            Console.WriteLine(parseError);
        }

        foreach (var statement in parseResult.Statements)
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
        var lox = new Lox(replMode: true);

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
    private readonly bool _replMode;

    public Lox(bool replMode = false)
    {
        _replMode = replMode;
    }

    public void Run(string source)
    {
        var error = false;

        var scanner = new Scanner(source);
        var scanResult = scanner.ScanTokens();

        foreach (var scanError in scanResult.Errors)
        {
            Console.WriteLine(scanError);
        }

        error |= !scanResult.Success;

        // Try expressions first.
        if (_replMode)
        {
            var parser = new Parser(scanResult.Tokens);
            var parseExprResult = parser.ParseExpression();
            if (parseExprResult.Success)
            {
                var value = _interpreter.Interpret(parseExprResult.Expression!);

                Console.WriteLine($">>> {value}");
                return;
            }
        }

        // Statements then.
        {
            var parser = new Parser(scanResult.Tokens);
            var parseResult = parser.ParseStatements();

            foreach (var parseError in parseResult.Errors)
            {
                Console.WriteLine(parseError);
            }

            error |= !parseResult.Success;

            if (scanResult.Success && parseResult.Success)
            {
                var resolver = new Resolver();
                var resolverResult = resolver.DoWork(parseResult.Statements);

                foreach (var err in resolverResult.Errors)
                {
                    Console.WriteLine(err);
                }

                error |= !resolverResult.Success;
            }
            
            if (error)
            {
                return;
            }

            _interpreter.Interpret(parseResult.Statements);
        }
    }
}
