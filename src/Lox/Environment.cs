using Lox.Model;

namespace Lox;

public class Environment
{
    private readonly Environment? _enclosing = null;
    private readonly Dictionary<string, object?> _values = [];

    public Environment()
    {
    }

    public Environment(Environment enclosing)
    {
        _enclosing = enclosing;
    }

    public void Define(string name, object? value)
    {
        _values[name] = value;
    }

    public void Assign(string name, object? value, Token token)
    {
        if (_values.ContainsKey(name))
        {
            _values[name] = value;
            return;
        }

        if (_enclosing is not null)
        {
            _enclosing.Assign(name, value, token);
            return;
        }

        throw new RuntimeError(token, $"Undefined variable '{name}'");
    }

    public object? Get(string name, Token token)
    {
        if (_values.TryGetValue(name, out var value))
            return value;

        if (_enclosing is not null)
        {
            return _enclosing.Get(name, token);
        }

        throw new RuntimeError(token, $"Undefined variable '{name}'");
    }
}
