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
public class AstGenerator : IIncrementalGenerator
{
    private record AstData(ClassDeclarationSyntax ClassDeclaration, string ParentClassName, IReadOnlyList<AstClassData> Classses);
    private record AstClassData(string Name, IReadOnlyList<AstPropertyData> Properties, string VisitMethodName);
    private record AstPropertyData(string Type, string LowerName, string UpperName);

    public void Initialize(IncrementalGeneratorInitializationContext context)
    {
        Log($"AstGenerator Initialize");

        context.RegisterPostInitializationOutput(GenerateAttribute);

        IncrementalValuesProvider<AstData> astData = context
            .SyntaxProvider
            .ForAttributeWithMetadataName("Lox.AstGenAttribute",
                predicate: (node, ct) => node is ClassDeclarationSyntax classDeclaration,
                transform: (ctx, ct) => TransformAstData(ctx)
                );

        context.RegisterSourceOutput(astData, GenerateAst);
    }

    private void GenerateAttribute(IncrementalGeneratorPostInitializationContext context)
    {
        context.AddSource("AstGenAttribute.g.cs", """
            // AstGenAttribute.g.cs

            using System;
            
            namespace Lox;

            [AttributeUsage(AttributeTargets.Class, AllowMultiple = true)]
            public class AstGenAttribute : Attribute
            {
                public string Data { get; }

                public AstGenAttribute(string data)
                {
                    Data = data;
                }
            }
            """);
    }

    private static AstData TransformAstData(GeneratorAttributeSyntaxContext ctx)
    {
        if (ctx.TargetNode is not ClassDeclarationSyntax)
            throw new InvalidOperationException("TargetNode must be ClassDeclarationSyntax");

        var classDeclaration = (ClassDeclarationSyntax)ctx.TargetNode;
        var className = classDeclaration.Identifier.ToString();

        var dataStrings = new List<string>();

        foreach (var attribute in ctx.Attributes)
        {
            if (attribute.ConstructorArguments.Length != 1)
                throw new InvalidOperationException("Attrubet must get exactly 1 ctor-argument");

            var arg = attribute.ConstructorArguments[0];

            if (arg.Kind == TypedConstantKind.Primitive && arg.Value is string stringValue)
            {
                dataStrings.Add(stringValue);
            }
            else
            {
                throw new InvalidOperationException("ctor-argument must be a string");
            }
        }

        var classes = dataStrings.Select(s => ParseDataString(s, className)).ToArray();

        return new AstData(classDeclaration, className, classes);
    }

    private static AstClassData ParseDataString(string data, string parentClassName)
    {
        // example:
        // Binary   : Expr left, Token op, Expr right

        var x = data.Split(':');
        if (x.Length != 2) throw new InvalidOperationException("Invalid data string");

        var className = x[0].Trim();
        var propertiesString = x[1].Trim();


        var properties = new List<AstPropertyData>();

        foreach (var propertyString in propertiesString.Split(','))
        {
            var p = propertyString.Trim().Split(' ');
            if (p.Length != 2) throw new InvalidOperationException("Invalid data string");

            var type = p[0];
            var name = p[1];
            var lowerName = EscapeKeywords(FirstCharacterToLower(name));
            var upperName = EscapeKeywords(FirstCharacterToUpper(name));

            properties.Add(new AstPropertyData(Type: type, LowerName: lowerName, UpperName: upperName));
        }

        var visitMethodName = $"Visit{className}{parentClassName}";

        return new AstClassData(className, properties, visitMethodName);
    }

    private void GenerateAst(SourceProductionContext context, AstData astData)
    {
        Log($"GenerateAst {astData}");

        try
        {
            var classDeclaration = astData.ClassDeclaration;
            var className = classDeclaration.Identifier.ToString();
            var fileName = $"AstGen_{className}.g.cs";

            var sb = new StringBuilder();

            // header
            sb.AppendLine($"// {fileName}");
            sb.AppendLine($"// Generated at {DateTime.Now.ToLongTimeString()}");
            sb.AppendLine();
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
            sb.AppendLine($"{classDeclaration.Modifiers} class {className}");
            sb.AppendLine("{");
            sb.AppendLine("    public static int SomeFunc() => 42;");
            sb.AppendLine();
            sb.AppendLine("    public abstract T Accept<T>(IVisitor<T> visitor);");
            sb.AppendLine();

            // visitor interface
            GenerateVisitorInterface(sb, astData);

            // ast classes
            GenerateAstClasses(sb, astData);

            // end of class
            sb.AppendLine("}");

            // xxx remove
            Log($"AddSource {fileName}: {sb}");
            // xxx remove

            context.AddSource(fileName, sb.ToString());
        }
        catch (Exception e)
        {
            Log($"Error: {e}");
            throw;
        }
    }

    private static void GenerateVisitorInterface(StringBuilder sb, AstData astData)
    {
        sb.AppendLine("    public interface IVisitor<T>");
        sb.AppendLine("    {");

        foreach (var astClass in astData.Classses)
        {
            var className = astClass.Name;
            var methodName = astClass.VisitMethodName;

            sb.AppendLine($"        T {methodName}({className} {EscapeKeywords(FirstCharacterToLower(className))});");
        }

        sb.AppendLine("    }");
    }

    private static void GenerateAstClasses(StringBuilder sb, AstData astData)
    {
        foreach (var astClass in astData.Classses)
        {
            // start
            sb.AppendLine();
            sb.AppendLine($"    public class {astClass.Name} : {astData.ParentClassName}");
            sb.AppendLine("    {");

            // props
            foreach (var p in astClass.Properties)
            {
                sb.AppendLine($"        public {p.Type} {p.UpperName} {{ get; }}");
            }

            // ctor
            var ctorArgs = astClass.Properties.Select(p => $"{p.Type} {p.LowerName}");
            var ctorArg = String.Join(", ", ctorArgs);

            sb.AppendLine($"        public {astClass.Name}({ctorArg})");
            sb.AppendLine("        {");

            foreach (var p in astClass.Properties)
            {
                sb.AppendLine($"            {p.UpperName} = {p.LowerName};");
            }

            sb.AppendLine("        }");

            // accept-method
            sb.AppendLine($"        public override T Accept<T>(IVisitor<T> visitor)");
            sb.AppendLine("        {");
            sb.AppendLine($"            return visitor.{astClass.VisitMethodName}(this);");
            sb.AppendLine("        }");

            // end
            sb.AppendLine("    }");
        }
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

    //private static string GetFullClassName(ClassDeclarationSyntax classDeclaration)
    //{
    //    var ns = GetNamespace(classDeclaration);

    //    return $"{ns}.{classDeclaration.Identifier}";
    //}

    private static string EscapeKeywords(string s)
    {
        if (SyntaxFacts.GetKeywordKind(s) == SyntaxKind.None)
            return s;

        return $"@{s}";
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
#if false
        var s = $"{DateTime.Now.ToLongTimeString()}: {message}\n";

#pragma warning disable RS1035 // Do not use APIs banned for analyzers
        File.AppendAllText(@"c:\temp\gen_log.txt", s);
#pragma warning restore RS1035 // Do not use APIs banned for analyzers
#endif
    }
}
