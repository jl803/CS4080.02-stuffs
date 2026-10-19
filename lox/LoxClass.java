import java.util.List;
import java.util.Map;

class LoxClass extends LoxInstance implements LoxCallable {
  final String name;
  private final LoxClass superclass;
  private final Map<String, LoxFunction> methods;

  LoxClass(
      LoxClass metaclass,
      String name,
      LoxClass superclass,
      Map<String, LoxFunction> methods) {
    super(metaclass);
    this.name = name;
    this.superclass = superclass;
    this.methods = methods;
  }

  LoxFunction findMethod(LoxInstance instance, String methodName) {
    List<LoxFunction> matches = new java.util.ArrayList<>();

    for (LoxClass current = this;
        current != null;
        current = current.superclass) {
      LoxFunction method = current.methods.get(methodName);
      if (method != null) {
        matches.add(method);
      }
    }

    LoxFunction result = null;
    for (LoxFunction method : matches) {
      result = method.bind(instance, result);
    }
    return result;
  }

  private LoxFunction findHighestMethod(String methodName) {
    LoxFunction result = null;
    for (LoxClass current = this;
        current != null;
        current = current.superclass) {
      LoxFunction method = current.methods.get(methodName);
      if (method != null) {
        result = method;
      }
    }
    return result;
  }

  @Override
  public int arity() {
    LoxFunction initializer = findHighestMethod("init");
    if (initializer == null) {
      return 0;
    }
    return initializer.arity();
  }

  @Override
  public Object call(Interpreter interpreter, List<Object> arguments) {
    LoxInstance instance = new LoxInstance(this);
    LoxFunction initializer = findMethod(instance, "init");
    if (initializer != null) {
      initializer.call(interpreter, arguments);
    }
    return instance;
  }

  @Override
  public String toString() {
    return name;
  }
}