import java.util.List;

class LoxFunction implements LoxCallable {
  private final String name;
  private final List<Token> parameters;
  private final List<Stmt> body;
  private final Environment closure;

  LoxFunction(Stmt.Function declaration, Environment closure) {
    this.name = declaration.name.lexeme;
    this.parameters = declaration.params;
    this.body = declaration.body;
    this.closure = closure;
  }

  LoxFunction(Expr.Function expression, Environment closure) {
    this.name = null;
    this.parameters = expression.params;
    this.body = expression.body;
    this.closure = closure;
  }

  @Override
  public int arity() {
    return parameters.size();
  }

  @Override
  public Object call(Interpreter interpreter, List<Object> arguments) {
    Environment environment = new Environment(closure);
    for (int i = 0; i < parameters.size(); i++) {
      environment.define(parameters.get(i).lexeme, arguments.get(i));
    }

    try {
      interpreter.executeBlock(body, environment);
    } catch (Return returnValue) {
      return returnValue.value;
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
