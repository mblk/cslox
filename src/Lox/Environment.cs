﻿namespace Lox;

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

    public void AssignAtHop(string name, object? value, int hops)
    {
        var env = Ancestor(hops);

        if (!env._values.ContainsKey(name))
            throw new InvalidOperationException("Internal error. Item not found in environment after hopping.");

        env._values[name] = value;
    }

    public object? GetAtHop(string name, int hops)
    {
        var env = Ancestor(hops);
        if (env._values.TryGetValue(name, out var value))
        {
            return value;
        }

        throw new InvalidOperationException("Internal error. Item not found in environment after hopping.");
    }

    private Environment Ancestor(int hops)
    {
        Environment current = this;

        while (hops > 0)
        {
            if (current._enclosing is null)
                throw new InvalidOperationException("Internal error. Invalid hop count.");

            current = current._enclosing!;
            hops--;
        }

        return current;
    }
}
