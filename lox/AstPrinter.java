class AstPrinter implements Expr.Visitor<String> {
  String print(Expr expr) {
    return expr.accept(this);
  }

  @Override
  public String visitArrayExpr(Expr.Array expr) {
    return parenthesize("array", expr.elements.toArray(new Expr[0]));
  }

  @Override
  public String visitAssignExpr(Expr.Assign expr) {
    return parenthesize("assign " + expr.name.lexeme, expr.value);
  }

  @Override
  public String visitCallExpr(Expr.Call expr) {
    Expr[] parts = new Expr[expr.arguments.size() + 1];
    parts[0] = expr.callee;
    for (int i = 0; i < expr.arguments.size(); i++) {
      parts[i + 1] = expr.arguments.get(i);
    }
    return parenthesize("call", parts);
  }

  @Override
  public String visitBinaryExpr(Expr.Binary expr) {
    return parenthesize(expr.operator.lexeme, expr.left, expr.right);
  }

  @Override
  public String visitConditionalExpr(Expr.Conditional expr) {
    return parenthesize("?:", expr.condition, expr.thenBranch, expr.elseBranch);
  }

  @Override
  public String visitFunctionExpr(Expr.Function expr) {
    return "<fn>";
  }

  @Override
  public String visitGetExpr(Expr.Get expr) {
    return parenthesize("get " + expr.name.lexeme, expr.object);
  }

  @Override
  public String visitGroupingExpr(Expr.Grouping expr) {
    return parenthesize("group", expr.expression);
  }

  @Override
  public String visitIndexExpr(Expr.Index expr) {
    return parenthesize("index", expr.object, expr.index);
  }

  @Override
  public String visitIndexSetExpr(Expr.IndexSet expr) {
    return parenthesize("index-set", expr.object, expr.index, expr.value);
  }

  @Override
  public String visitLiteralExpr(Expr.Literal expr) {
    if (expr.value == null) {
      return "nil";
    }
    return expr.value.toString();
  }

  @Override
  public String visitLogicalExpr(Expr.Logical expr) {
    return parenthesize(expr.operator.lexeme, expr.left, expr.right);
  }

  @Override
  public String visitSetExpr(Expr.Set expr) {
    return parenthesize("set " + expr.name.lexeme, expr.object, expr.value);
  }

  @Override
  public String visitThisExpr(Expr.This expr) {
    return "this";
  }

  @Override
  public String visitUnaryExpr(Expr.Unary expr) {
    return parenthesize(expr.operator.lexeme, expr.right);
  }

  @Override
  public String visitVariableExpr(Expr.Variable expr) {
    return expr.name.lexeme;
  }

  private String parenthesize(String name, Expr... expressions) {
    StringBuilder builder = new StringBuilder();
    builder.append('(').append(name);

    for (Expr expression : expressions) {
      builder.append(' ');
      builder.append(expression.accept(this));
    }

    builder.append(')');
    return builder.toString();
  }
}
