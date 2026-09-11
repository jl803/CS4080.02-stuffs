import java.io.IOException;
import java.io.PrintWriter;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.Arrays;
import java.util.List;

public class GenerateAst {
  public static void main(String[] args) throws IOException {
    if (args.length != 1) {
      System.err.println("Usage: java GenerateAst <output directory>");
      System.exit(64);
    }

    Path outputDir = Path.of(args[0]);
    if (!Files.isDirectory(outputDir)) {
      System.err.println("Output directory does not exist: " + outputDir);
      System.exit(64);
    }

    defineAst(outputDir, "Expr", Arrays.asList(
        "Binary   : Expr left, Token operator, Expr right",
        "Grouping : Expr expression",
        "Literal  : Object value",
        "Unary    : Token operator, Expr right"));

    System.out.println("Generated " + outputDir.resolve("Expr.java"));
  }

  private static void defineAst(
      Path outputDir, String baseName, List<String> types) throws IOException {
    Path path = outputDir.resolve(baseName + ".java");

    try (PrintWriter writer = new PrintWriter(
        Files.newBufferedWriter(path, StandardCharsets.UTF_8))) {
      writer.println("abstract class " + baseName + " {");

      defineVisitor(writer, baseName, types);

      for (String type : types) {
        String[] parts = type.split(":", 2);
        String className = parts[0].trim();
        String fields = parts[1].trim();
        defineType(writer, baseName, className, fields);
      }

      writer.println();
      writer.println("  abstract <R> R accept(Visitor<R> visitor);");
      writer.println("}");
    }
  }

  private static void defineVisitor(
      PrintWriter writer, String baseName, List<String> types) {
    writer.println("  interface Visitor<R> {");

    for (String type : types) {
      String typeName = type.split(":", 2)[0].trim();
      writer.println("    R visit" + typeName + baseName + "(" +
          typeName + " " + baseName.toLowerCase() + ");");
    }

    writer.println("  }");
  }

  private static void defineType(
      PrintWriter writer, String baseName, String className, String fieldList) {
    writer.println();
    writer.println("  static class " + className + " extends " + baseName + " {");

    writer.println("    " + className + "(" + fieldList + ") {");
    String[] fields = fieldList.split(", ");
    for (String field : fields) {
      String name = field.substring(field.lastIndexOf(' ') + 1);
      writer.println("      this." + name + " = " + name + ";");
    }
    writer.println("    }");

    writer.println();
    writer.println("    @Override");
    writer.println("    <R> R accept(Visitor<R> visitor) {");
    writer.println("      return visitor.visit" + className + baseName + "(this);");
    writer.println("    }");

    writer.println();
    for (String field : fields) {
      writer.println("    final " + field + ";");
    }

    writer.println("  }");
  }
}
