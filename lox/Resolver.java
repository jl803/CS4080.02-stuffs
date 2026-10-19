import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Stack;

class Resolver implements Expr.Visitor<Void>, Stmt.Visitor<Void> {
  private enum FunctionType {
    NONE,
    FUNCTION,
    INITIALIZER,
    METHOD
  }

  private enum ClassType {
    NONE,
    CLASS
  }

  private enum VariableState {
    DECLARED,
    DEFINED,
    READ
  }

  private static class Variable {
    final Token name;
    final int index;
    VariableState state;

    Variable(Token name, int index, VariableState state) {
      this.name = name;
      this.index = index;
      this.state = state;
    }
  }

  private final Interpreter interpreter;
  private final Stack<Map<String, Variable>> scopes = new Stack<>();
  private FunctionType currentFunction = FunctionType.NONE;
  private ClassType currentClass = ClassType.NONE;

  Resolver(Interpreter interpreter) {
    this.interpreter = interpreter;
  }

  void resolve(List<Stmt> statements) {
    for (Stmt statement : statements) {
      resolve(statement);
    }
  }

  private void resolve(Stmt statement) {
    statement.accept(this);
  }

  private void resolve(Expr expression) {
    expression.accept(this);
  }

  private void beginScope() {
    scopes.push(new HashMap<>());
  }

  private void endScope() {
    Map<String, Variable> scope = scopes.pop();
    for (Variable variable : scope.values()) {
      if (variable.state == VariableState.DEFINED) {
        Lox.error(variable.name, "Local variable is never used.");
      }
    }
  }

  private void declare(Token name) {
    if (scopes.isEmpty()) {
      return;
    }

    Map<String, Variable> scope = scopes.peek();
    if (scope.containsKey(name.lexeme)) {
      Lox.error(name, "Already a variable with this name in this scope.");
    }
    int index = scope.size();
    scope.put(
        name.lexeme,
        new Variable(name, index, VariableState.DECLARED));
    interpreter.resolveDeclaration(name, index);
  }

  private void define(Token name) {
    if (scopes.isEmpty()) {
      return;
    }
    scopes.peek().get(name.lexeme).state = VariableState.DEFINED;
  }

  private void resolveLocal(Expr expression, Token name, boolean isRead) {
    for (int i = scopes.size() - 1; i >= 0; i--) {
      Variable variable = scopes.get(i).get(name.lexeme);
      if (variable != null) {
        interpreter.resolve(
            expression, scopes.size() - 1 - i, variable.index);
        if (isRead) {
          variable.state = VariableState.READ;
        }
        return;
      }
    }
  }

  private void resolveFunction(
      List<Token> parameters, List<Stmt> body, FunctionType type) {
    FunctionType enclosingFunction = currentFunction;
    currentFunction = type;
    beginScope();
    try {
      if (parameters != null) {
        for (Token parameter : parameters) {
          declare(parameter);
          define(parameter);
        }
      }
      resolve(body);
    } finally {
      endScope();
      currentFunction = enclosingFunction;
    }
  }

  @Override
  public Void visitArrayExpr(Expr.Array expr) {
    for (Expr element : expr.elements) {
      resolve(element);
    }
    return null;
  }

  @Override
  public Void visitAssignExpr(Expr.Assign expr) {
    resolve(expr.value);
    resolveLocal(expr, expr.name, false);
    return null;
  }

  @Override
  public Void visitBinaryExpr(Expr.Binary expr) {
    resolve(expr.left);
    resolve(expr.right);
    return null;
  }

  @Override
  public Void visitCallExpr(Expr.Call expr) {
    resolve(expr.callee);
    for (Expr argument : expr.arguments) {
      resolve(argument);
    }
    return null;
  }

  @Override
  public Void visitConditionalExpr(Expr.Conditional expr) {
    resolve(expr.condition);
    resolve(expr.thenBranch);
    resolve(expr.elseBranch);
    return null;
  }

  @Override
  public Void visitFunctionExpr(Expr.Function expr) {
    resolveFunction(expr.params, expr.body, FunctionType.FUNCTION);
    return null;
  }

  @Override
  public Void visitGetExpr(Expr.Get expr) {
    resolve(expr.object);
    return null;
  }

  @Override
  public Void visitGroupingExpr(Expr.Grouping expr) {
    resolve(expr.expression);
    return null;
  }

  @Override
  public Void visitIndexExpr(Expr.Index expr) {
    resolve(expr.object);
    resolve(expr.index);
    return null;
  }

  @Override
  public Void visitIndexSetExpr(Expr.IndexSet expr) {
    resolve(expr.object);
    resolve(expr.index);
    resolve(expr.value);
    return null;
  }

  @Override
  public Void visitLiteralExpr(Expr.Literal expr) {
    return null;
  }

  @Override
  public Void visitLogicalExpr(Expr.Logical expr) {
    resolve(expr.left);
    resolve(expr.right);
    return null;
  }

  @Override
  public Void visitSetExpr(Expr.Set expr) {
    resolve(expr.value);
    resolve(expr.object);
    return null;
  }

  @Override
  public Void visitThisExpr(Expr.This expr) {
    if (currentClass == ClassType.NONE) {
      Lox.error(expr.keyword, "Can't use 'this' outside of a class.");
      return null;
    }
    resolveLocal(expr, expr.keyword, true);
    return null;
  }

  @Override
  public Void visitUnaryExpr(Expr.Unary expr) {
    resolve(expr.right);
    return null;
  }

  @Override
  public Void visitVariableExpr(Expr.Variable expr) {
    if (!scopes.isEmpty()) {
      Variable variable = scopes.peek().get(expr.name.lexeme);
      if (variable != null && variable.state == VariableState.DECLARED) {
        Lox.error(expr.name, "Can't read local variable in its own initializer.");
      }
    }

    resolveLocal(expr, expr.name, true);
    return null;
  }

  @Override
  public Void visitBlockStmt(Stmt.Block stmt) {
    beginScope();
    try {
      resolve(stmt.statements);
    } finally {
      endScope();
    }
    return null;
  }

  @Override
  public Void visitBreakStmt(Stmt.Break stmt) {
    return null;
  }

  @Override
  public Void visitClassStmt(Stmt.Class stmt) {
    ClassType enclosingClass = currentClass;
    currentClass = ClassType.CLASS;

    declare(stmt.name);
    define(stmt.name);

    if (stmt.superclass != null) {
      if (stmt.name.lexeme.equals(stmt.superclass.name.lexeme)) {
        Lox.error(stmt.superclass.name, "A class can't inherit from itself.");
      }
      resolve(stmt.superclass);
    }

    beginScope();
    scopes.peek().put(
        "this", new Variable(stmt.name, 0, VariableState.READ));
    Token inner = new Token(
        TokenType.IDENTIFIER, "inner", null, stmt.name.line);
    scopes.peek().put(
        "inner", new Variable(inner, 1, VariableState.READ));
    try {
      for (Stmt.Function method : stmt.methods) {
        FunctionType type = FunctionType.METHOD;
        if (method.name.lexeme.equals("init")) {
          type = FunctionType.INITIALIZER;
        }
        resolveFunction(method.params, method.body, type);
      }

      for (Stmt.Function method : stmt.classMethods) {
        resolveFunction(method.params, method.body, FunctionType.METHOD);
      }
    } finally {
      endScope();
      currentClass = enclosingClass;
    }
    return null;
  }

  @Override
  public Void visitExpressionStmt(Stmt.Expression stmt) {
    resolve(stmt.expression);
    return null;
  }

  @Override
  public Void visitFunctionStmt(Stmt.Function stmt) {
    declare(stmt.name);
    define(stmt.name);
    resolveFunction(stmt.params, stmt.body, FunctionType.FUNCTION);
    return null;
  }

  @Override
  public Void visitIfStmt(Stmt.If stmt) {
    resolve(stmt.condition);
    resolve(stmt.thenBranch);
    if (stmt.elseBranch != null) {
      resolve(stmt.elseBranch);
    }
    return null;
  }

  @Override
  public Void visitPrintStmt(Stmt.Print stmt) {
    resolve(stmt.expression);
    return null;
  }

  @Override
  public Void visitReturnStmt(Stmt.Return stmt) {
    if (currentFunction == FunctionType.NONE) {
      Lox.error(stmt.keyword, "Can't return from top-level code.");
    }
    if (stmt.value != null) {
      if (currentFunction == FunctionType.INITIALIZER) {
        Lox.error(stmt.keyword, "Can't return a value from an initializer.");
      }
      resolve(stmt.value);
    }
    return null;
  }

  @Override
  public Void visitVarStmt(Stmt.Var stmt) {
    declare(stmt.name);
    if (stmt.initializer != null) {
      resolve(stmt.initializer);
    }
    define(stmt.name);
    return null;
  }

  @Override
  public Void visitWhileStmt(Stmt.While stmt) {
    resolve(stmt.condition);
    resolve(stmt.body);
    return null;
  }
}
