#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <string>

namespace Jenjin {
namespace Editor {
void ensure_dir(std::string path);
std::string get_jendir();
}
}
