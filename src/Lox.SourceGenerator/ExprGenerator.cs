using Microsoft.CodeAnalysis;
using Microsoft.CodeAnalysis.CSharp;
using Microsoft.CodeAnalysis.CSharp.Syntax;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;

namespace Lox.SourceGenerator;

[Generator]
public class ExprGenerator : IIncrementalGenerator
{
    public void Initialize(IncrementalGeneratorInitializationContext context)
    {
        Log($"ExprGenerator Initialize");

        context.RegisterPostInitializationOutput(GenerateAttribute);

        IncrementalValuesProvider<ClassDeclarationSyntax> exprClassesProvider = context.SyntaxProvider
            .CreateSyntaxProvider(
            predicate: (node, ct) =>
            {
                return node is ClassDeclarationSyntax classDeclaration &&
                       GetFullClassName(classDeclaration) == "Lox.Expr";
            },
            transform: (ctx, ct) =>
            {
                return (ClassDeclarationSyntax)ctx.Node;
            });

        context.RegisterSourceOutput(exprClassesProvider, GenerateExpressionClasses);
    }

    private void GenerateAttribute(IncrementalGeneratorPostInitializationContext context)
    {
        context.AddSource("ExprAttribute.g.cs", """
            // ExprAttribute.g.cs
            
            namespace Lox;

            [System.AttributeUsage(System.AttributeTargets.Class, AllowMultiple = true)]
            public class ExprAttribute : System.Attribute
            {
                public string Data { get; }

                public ExprAttribute(string data)
                {
                    Data = data;
                }
            }
            """);
    }

    private void GenerateExpressionClasses(SourceProductionContext context, ClassDeclarationSyntax classDeclaration)
    {
        Log($"GenerateExpressionClasses {classDeclaration.Identifier}");

        try
        {
            var exprDataStrings = GetExprDataStrings(classDeclaration).ToArray();

            var sb = new StringBuilder();

            // header
            sb.AppendLine("// Hello World!");
            sb.AppendLine($"// Generated at {DateTime.Now.ToLongTimeString()}");
            sb.AppendLine("#nullable enable");

            // usings
            sb.AppendLine();
            sb.AppendLine("// Usings:");
            foreach (var usingStatement in classDeclaration.SyntaxTree.GetCompilationUnitRoot().Usings)
            {
                sb.AppendLine(usingStatement.ToString());
            }

            // namespace
            sb.AppendLine();
            sb.AppendLine("// Namespace:");
            sb.AppendLine($"namespace {GetNamespace(classDeclaration)};");

            // start of class
            sb.AppendLine();
            sb.AppendLine($"{classDeclaration.Modifiers} class {classDeclaration.Identifier}");
            sb.AppendLine("{");
            sb.AppendLine("    public static int SomeFunc() => 42;");
            sb.AppendLine("    public abstract T Accept<T>(IVisitor<T> visitor);");

            // visitor
            GenerateVisitorInterface(sb, exprDataStrings);

            // expr classes
            foreach (var data in exprDataStrings)
            {
                GenerateExprClass(sb, data);
            }

            // end of class
            sb.AppendLine("}");

            // xxx remove
            Log($"AddSource: {sb}");
            // xxx remove

            context.AddSource("Expr.g.cs", sb.ToString());
        }
        catch (Exception e)
        {
            Log($"Error: {e}");
        }
    }

    private static IEnumerable<string> GetExprDataStrings(ClassDeclarationSyntax classDeclaration)
    {
        foreach (var attrList in classDeclaration.AttributeLists)
        {
            foreach (var attr in attrList.Attributes)
            {
                if (attr.ArgumentList is null) continue;
                foreach (var argument in attr.ArgumentList.Arguments)
                {
                    var data = argument.ToString().Trim('"', ' ');
                    //GenerateExprClass(sb, data);

                    yield return data;
                }
            }
        }
    }

    private static void GenerateVisitorInterface(StringBuilder sb, IEnumerable<string> datas)
    {
        sb.AppendLine("    public interface IVisitor<T>");
        sb.AppendLine("    {");

        foreach (var data in datas)
        {
            var x = data.Split(':');
            var className = x[0].Trim(); // TODO make a nice record for the expr-datas and parse only once.

            // T VisitBinary(Binary binary);
            sb.AppendLine($"        T Visit{className}({className} {FirstCharacterToLower(className)});");
        }

        sb.AppendLine("    }");
    }

    private static void GenerateExprClass(StringBuilder sb, string data)
    {
        var x = data.Split(':');
        var className = x[0].Trim();
        var properties = x[1].Trim();

        // start
        sb.AppendLine();
        sb.AppendLine($"    public class {className} : Expr");
        sb.AppendLine("    {");

        // props
        foreach (var property in properties.Split(','))
        {
            var p = property.Trim().Split(' ');
            var type = p[0];
            var name = FirstCharacterToUpper(p[1]);

            sb.AppendLine($"        public {type} {name} {{ get; }}");
        }

        // ctor
        sb.AppendLine($"        public {className}({properties})");
        sb.AppendLine("        {");

        foreach (var property in properties.Split(','))
        {
            var p = property.Trim().Split(' ');
            var argName = p[1];
            var propName = FirstCharacterToUpper(p[1]);

            sb.AppendLine($"            {propName} = {argName};");
        }

        sb.AppendLine("        }");

        // accept-method
        sb.AppendLine("        public override T Accept<T>(IVisitor<T> visitor)");
        sb.AppendLine("        {");
        sb.AppendLine($"            return visitor.Visit{className}(this);");
        sb.AppendLine("        }");

        // end
        sb.AppendLine("    }");
    }









    //
    // TODO make extensions / move
    //

    private static string FirstCharacterToLower(string s)
    {
        return $"{Char.ToLower(s[0])}{s.Substring(1)}";
    }

    private static string FirstCharacterToUpper(string s)
    {
        return $"{Char.ToUpper(s[0])}{s.Substring(1)}";
    }

    private static string GetFullClassName(ClassDeclarationSyntax classDeclaration)
    {
        var ns = GetNamespace(classDeclaration);

        return $"{ns}.{classDeclaration.Identifier}";
    }

    private static string GetNamespace(SyntaxNode node)
    {
        var namespaces = node
            .Ancestors()
            .OfType<BaseNamespaceDeclarationSyntax>()
            .Select(x => x.Name);

        return String.Join(".", namespaces);
    }

    private static void Log(string message)
    {
        var s = $"{DateTime.Now.ToLongTimeString()}: {message}\n";

#pragma warning disable RS1035 // Do not use APIs banned for analyzers
        File.AppendAllText(@"c:\temp\gen_log.txt", s);
#pragma warning restore RS1035 // Do not use APIs banned for analyzers
    }
}
