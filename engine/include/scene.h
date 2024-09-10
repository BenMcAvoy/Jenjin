#ifndef JENJIN_SCENE_H
#define JENJIN_SCENE_H

#include "gameobject.h"
#include "camera.h"
#include "shader.h"
#include "mesh.h"

#include <functional>
#include <vector>
#include <memory>

namespace Jenjin {
// A scene is a collection of game objects
// that are updated and rendered together
// (e.g. scripting, rendering).
//
// It also manages loading and unloading of
// game objects and their resources.
class Scene {
public:
	// RAII pattern
	Scene();
	Scene(std::vector<std::shared_ptr<GameObject>> game_objects);
	~Scene();

	// Used to rebuild the scene buffers
	void build();

	// Updaters
	void update();
	void render();

	// Adders
	void add_gameobject(std::shared_ptr<GameObject> game_object);
	void add_gameobjects(std::vector<std::shared_ptr<GameObject>> game_objects);

	// Callback management
	void set_update_callback(std::function<void(Scene&)> callback);
	void set_render_callback(std::function<void(Scene&)> callback);
	void set_resize_callback(std::function<void(Scene&, GLFWwindow*, int, int)> callback);

private:
	// Game object pointers
	std::vector<std::shared_ptr<GameObject>> m_game_objects;
	std::vector<MeshReference> m_mesh_references;

	// OpenGL buffers
	unsigned int m_vao, m_vbo, m_ebo = 0;

	// Callbacks
	std::function<void(Scene*)> m_update_callback;
	std::function<void(Scene*)> m_render_callback;
	std::function<void(Scene*, GLFWwindow*, int, int)> m_resize_callback;

	// Default resources
	Shader m_default_shader = Shader("engine/shaders/vshader.glsl", "engine/shaders/fshader.glsl");
	Camera m_default_camera;
};
}

#endif
