import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

class Environment {
  final Environment enclosing;
  private final Map<String, Object> values = new HashMap<>();
  private final List<Object> localValues = new ArrayList<>();
  private static final Object UNINITIALIZED = new Object();

  Environment() {
    enclosing = null;
  }

  Environment(Environment enclosing) {
    this.enclosing = enclosing;
  }

  void define(String name, Object value) {
    values.put(name, value);
  }

  void define(String name) {
    values.put(name, UNINITIALIZED);
  }

  Object get(Token name) {
    if (values.containsKey(name.lexeme)) {
      Object value = values.get(name.lexeme);

      if (value == UNINITIALIZED) {
        throw new RuntimeError(
          name, "Variable '" + name.lexeme + "' has not been initialized.");
      }

      return value;
    }

    if (enclosing != null) {
      return enclosing.get(name);
    }

    throw new RuntimeError(name, "Undefined variable '" + name.lexeme + "'.");
  }

  void defineAt(int index, Object value) {
    ensureLocalCapacity(index);
    localValues.set(index, value);
  }

  void defineAt(int index) {
    defineAt(index, UNINITIALIZED);
  }

  Object getAt(int distance, int index, Token name) {
    Object value = ancestor(distance).localValues.get(index);
    if (value == UNINITIALIZED) {
      throw new RuntimeError(
          name, "Variable '" + name.lexeme + "' has not been initialized.");
    }
    return value;
  }

  Object getAt(int distance, int index) {
    return ancestor(distance).localValues.get(index);
  }

  void assign(Token name, Object value) {
    if (values.containsKey(name.lexeme)) {
      values.put(name.lexeme, value);
      return;
    }

    if (enclosing != null) {
      enclosing.assign(name, value);
      return;
    }

    throw new RuntimeError(name, "Undefined variable '" + name.lexeme + "'.");
  }

  void assignAt(int distance, int index, Object value) {
    ancestor(distance).localValues.set(index, value);
  }

  private Environment ancestor(int distance) {
    Environment environment = this;
    for (int i = 0; i < distance; i++) {
      environment = environment.enclosing;
    }
    return environment;
  }

  private void ensureLocalCapacity(int index) {
    while (localValues.size() <= index) {
      localValues.add(null);
    }
  }
}
