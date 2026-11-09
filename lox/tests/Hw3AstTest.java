import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.List;

public class Hw3AstTest {
  public static void main(String[] args) throws Exception {
    if (args.length != 1) {
      System.out.println("Usage: java Hw3AstTest <script>");
      System.exit(64);
    }

    String source = Files.readString(Path.of(args[0]), StandardCharsets.UTF_8);
    Scanner scanner = new Scanner(source);
    List<Token> tokens = scanner.scanTokens();
    Expr expression = new Parser(tokens).parse();

    if (expression != null) {
      System.out.println(new AstPrinter().print(expression));
    }
  }
}
