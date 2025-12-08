#ifndef LUABINDINGS_HPP
#define LUABINDINGS_HPP

#include <sol/sol.hpp>

// Forward declaration
class Application;

class LuaBindings {
public:
    // Set up all Lua API bindings for the application
    static void setupBindings(Application* app, sol::state& lua);
};

#endif // LUABINDINGS_HPP
