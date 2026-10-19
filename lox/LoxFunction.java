import java.util.List;

class LoxFunction implements LoxCallable {
  private static final LoxCallable NO_INNER = new LoxCallable() {
    @Override
    public int arity() {
      return 0;
    }

    @Override
    public Object call(Interpreter interpreter, List<Object> arguments) {
      return null;
    }

    @Override
    public String toString() {
      return "<no inner method>";
    }
  };

  private final String name;
  private final List<Token> parameters;
  private final List<Stmt> body;
  private final Environment closure;
  private final boolean isInitializer;

  LoxFunction(Stmt.Function declaration, Environment closure) {
    this(declaration, closure, false);
  }

  LoxFunction(
      Stmt.Function declaration, Environment closure, boolean isInitializer) {
    this.name = declaration.name.lexeme;
    this.parameters = declaration.params;
    this.body = declaration.body;
    this.closure = closure;
    this.isInitializer = isInitializer;
  }

  LoxFunction(Expr.Function expression, Environment closure) {
    this.name = null;
    this.parameters = expression.params;
    this.body = expression.body;
    this.closure = closure;
    this.isInitializer = false;
  }

  LoxFunction bind(LoxInstance instance) {
    return bind(instance, null);
  }

  LoxFunction bind(LoxInstance instance, LoxFunction inner) {
    Environment environment = new Environment(closure);
    environment.defineAt(0, instance);
    environment.defineAt(1, inner == null ? NO_INNER : inner);
    return new LoxFunction(name, parameters, body, environment, isInitializer);
  }

  private LoxFunction(
      String name,
      List<Token> parameters,
      List<Stmt> body,
      Environment closure,
      boolean isInitializer) {
    this.name = name;
    this.parameters = parameters;
    this.body = body;
    this.closure = closure;
    this.isInitializer = isInitializer;
  }

  @Override
  public int arity() {
    if (parameters == null) {
      return 0;
    }
    return parameters.size();
  }

  boolean isGetter() {
    return parameters == null;
  }

  @Override
  public Object call(Interpreter interpreter, List<Object> arguments) {
    Environment environment = new Environment(closure);
    if (parameters != null) {
      for (int i = 0; i < parameters.size(); i++) {
        int index = interpreter.getDeclarationIndex(parameters.get(i));
        environment.defineAt(index, arguments.get(i));
      }
    }

    try {
      interpreter.executeBlock(body, environment);
    } catch (Return returnValue) {
      if (isInitializer) {
        return closure.getAt(0, 0);
      }
      return returnValue.value;
    }
    if (isInitializer) {
      return closure.getAt(0, 0);
    }
    return null;
  }

  @Override
  public String toString() {
    if (name == null) {
      return "<fn>";
    }
    return "<fn " + name + ">";
  }
}
