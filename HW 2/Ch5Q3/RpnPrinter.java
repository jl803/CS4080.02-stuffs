class RpnPrinter implements Expr.Visitor<String> {
  
  String print(Expr expression) {
    return expression.accept(this);
  }

  @Override
  public String visitBinaryExpr(Expr.Binary expr) {
    String left = expr.left.accept(this);
    String right = expr.right.accept(this);

    return left + " " + right + " " + expr.operator.lexeme;
  }

  @Override
  public String visitGroupingExpr(Expr.Grouping expr) {
    return expr.expression.accept(this);
  }

  @Override
  public String visitLiteralExpr(Expr.Literal expr) {
    if (expr.value == null) {
      return "nil";
    }

    return expr.value.toString();
  }

  @Override
  public String visitUnaryExpr(Expr.Unary expr) {
    String operand = expr.right.accept(this);

    return operand + " " + expr.operator.lexeme;
  }

  public static void main(String[] args) {
    Expr expression = new Expr.Binary(
      new Expr.Grouping(
        new Expr.Binary(
          new Expr.Literal(1),
          new Token(TokenType.PLUS, "+", null, 1),
          new Expr.Literal(2)
        )
      ),
      new Token(TokenType.STAR, "*", null, 1),
      new Expr.Grouping(
        new Expr.Binary(
          new Expr.Literal(4),
          new Token(TokenType.MINUS, "-", null, 1),
          new Expr.Literal(3)
        )
      )
    );

    System.out.println(new RpnPrinter().print(expression));
  }
}