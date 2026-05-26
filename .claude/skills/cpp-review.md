# Module hygiene
Check that a C++ module respects the following constraints: interface units (`.ixx`) should only export what downstream consumers need and implementation units (`.cpp`) should import their own module first. Verify there are no circular imports. Module names should match file names.

# C++ 20 Features
Check that Concepts do real constraining work, not decorating a template for appearances. Ensure Coroutine keywords (`co_await`, `co_return`) appear only in functions with a coroutine return type. Verify there is no usage of raw pointers where smart pointers or value type would do. Prioritize the use of references and value type over smart pointers. No `using namespace std` in interface units.

# ECS pattern compliance
Ensure Components are pure data, no logic, no virtual functions. Check Systems operate on components through the registry and must not hold component state themselves. Entity IDs don't leak outside ECS-aware code as raw integers.

# Naming and style
Member variables: trailing_underscore_. Types: PascalCase. Free functions and methods: snake_case. No abbreviations in public API names.