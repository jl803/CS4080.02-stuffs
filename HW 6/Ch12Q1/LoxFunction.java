import java.util.List;

class LoxFunction implements LoxCallable {
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
    Environment environment = new Environment(closure);
    environment.defineAt(0, instance);
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
    return parameters.size();
  }

  @Override
  public Object call(Interpreter interpreter, List<Object> arguments) {
    Environment environment = new Environment(closure);
    for (int i = 0; i < parameters.size(); i++) {
      int index = interpreter.getDeclarationIndex(parameters.get(i));
      environment.defineAt(index, arguments.get(i));
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
