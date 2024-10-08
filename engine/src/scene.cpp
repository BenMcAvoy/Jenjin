#define GLFW_INCLUDE_NONE

#include "jenjin/scene.h"
#include "jenjin/gameobject.h"
#include "jenjin/helpers.h"
#include "jenjin/mesh.h"

#include <glm/ext/matrix_transform.hpp>
#include <glm/fwd.hpp>

#include <spdlog/spdlog.h>

#include <algorithm>
#include <fstream>
#include <iostream>

using namespace Jenjin;

Scene::Scene() { spdlog::trace("Scene::Scene()"); }

void Scene::SetTarget(Target *target) {
  spdlog::trace("Scene::SetTarget({})", (void *)target);

  this->target = target;
  this->camera.Resize(target->GetSize());
}

void Scene::AddGameObject(std::shared_ptr<GameObject> gameObject) {
  gameObjects.emplace_back(gameObject);
}

void Scene::RemoveGameObject(std::shared_ptr<GameObject> gameObject) {
  this->RemoveGameObject(gameObject.get());
}
void Scene::RemoveGameObject(GameObject *gameObject) {
  gameObjects.erase(
      std::remove_if(gameObjects.begin(), gameObjects.end(),
                     [gameObject](std::shared_ptr<GameObject> go) {
                       return go.get() == gameObject;
                     }),
      gameObjects.end());
}

void Scene::SetGameObjectTexture(GameObject *gameObject,
                                 const std::string &texturePath) {
  if (this->textures.find(texturePath) == this->textures.end()) {
    auto alpha = texturePath.find(".png") != std::string::npos;
    this->textures[texturePath] =
        std::make_shared<Texture>(texturePath.c_str(), alpha);
  }

  gameObject->texturePath = texturePath;
}

void Scene::SetGameObjectTexture(std::shared_ptr<GameObject> gameObject,
                                 const std::string &texturePath) {
  SetGameObjectTexture(gameObject.get(), texturePath);
}

void Scene::Build() {
  spdlog::trace("Scene::Build()");

  // Clear out any old data
  this->meshReferences.clear();
  this->meshReferences.reserve(gameObjects.size());

  std::vector<Vertex> vertices;
  std::vector<unsigned int> indices;

  for (auto &gameObject : gameObjects) {
    int baseVertex = vertices.size();
    int baseIndex = indices.size();

    std::vector<Vertex> &meshVertices = gameObject->mesh.vertices;
    std::vector<GLuint> &meshIndices = gameObject->mesh.indices;

    MeshReference meshReference = {baseVertex, baseIndex,
                                   (int)meshIndices.size(),
                                   (int)meshVertices.size()};
    this->meshReferences.emplace_back(meshReference);

    vertices.insert(vertices.end(), meshVertices.begin(), meshVertices.end());
    indices.insert(indices.end(), meshIndices.begin(), meshIndices.end());

    gameObject->meshReferenceID = this->meshReferences.size() - 1;
  }

  spdlog::debug("Built {} mesh references... now generating buffers",
                this->meshReferences.size());

  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);

  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex),
               vertices.data(), GL_STATIC_DRAW);

  glGenBuffers(1, &ebo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int),
               indices.data(), GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (void *)offsetof(Vertex, position));
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (void *)offsetof(Vertex, texCoord));

  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);

  spdlog::debug("Generated buffers (vao: {}, vbo: {}, ebo: {})", vao, vbo, ebo);
}

void Scene::Update() { spdlog::trace("Scene::Update()"); }

void Scene::Render() {
  this->shader.use();
  this->camera.Use();
  this->camera.Update();
  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, ebo);

  for (auto &gameObject : gameObjects) {
    if (gameObject->meshReferenceID == -1) {
      spdlog::error("GameObject has no mesh reference");
      continue;
    }

    MeshReference *meshReference =
        &this->meshReferences[gameObject->meshReferenceID];

    glm::mat4 model = glm::mat4(1.0f);
    model =
        glm::translate(model, glm::vec3(gameObject->transform.position, 0.0f));
    model = glm::rotate(model, glm::radians(-gameObject->transform.rotation),
                        glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::scale(model, glm::vec3(gameObject->transform.scale, 1.0f));

    shader.set("u_model", model);
    shader.set("u_color", gameObject->color);

    if (!gameObject->texturePath.empty()) {
      this->textures[gameObject->texturePath]->bind(0);
      shader.set("u_texture", 0);
    }

    bool hasTexture = !gameObject->texturePath.empty();
    shader.set("u_hasTexture", hasTexture);
    shader.set("u_mixColor", gameObject->mixColor && hasTexture);

    glDrawElementsBaseVertex(GL_TRIANGLES, meshReference->indexCount,
                             GL_UNSIGNED_INT, 0, meshReference->baseVertex);
  }
}

struct GOBJSAVABLE {
  Jenjin::GameObject::Transform transform;
  glm::vec3 color;

  char texturePath[128];
  char name[128];
};

void Scene::Save(const std::string &path) {
  spdlog::trace("Scene::Save({})", path);

  std::ofstream file(path, std::ios::binary);
  Save(file);
}

void Scene::Save(std::ofstream &file) {
  spdlog::trace("Scene::Save({})", (void *)&file);

  if (this->gameObjects.empty()) {
    spdlog::debug("No game objects to save");
    return;
  }

  for (auto &go : this->gameObjects) {
    GOBJSAVABLE gobj = {.transform = go->transform,
                        .color = go->color,

                        .texturePath = {0},
                        .name = {0}};

    strncpy(gobj.name, go->name.c_str(), sizeof(gobj.name));
    strncpy(gobj.texturePath, go->texturePath.c_str(),
            sizeof(gobj.texturePath));
    gobj.name[sizeof(gobj.name) - 1] = 0;
    gobj.texturePath[sizeof(gobj.texturePath) - 1] = 0;

    file.write(reinterpret_cast<char *>(&gobj), sizeof(GOBJSAVABLE));
  }
}

void Scene::Load(const std::string &path) {
  spdlog::trace("Scene::Load({})", path);

  std::ifstream file(path, std::ios::binary);
  this->Load(file);
}

void Scene::Load(std::ifstream &file) {
  spdlog::trace("Scene::Load({})", (void *)&file);

  this->gameObjects.clear();

  for (GOBJSAVABLE gobj;
       file.read(reinterpret_cast<char *>(&gobj), sizeof(GOBJSAVABLE));) {
    auto go = std::make_shared<GameObject>(
        gobj.name, Jenjin::Helpers::CreateQuad(2.0f, 2.0f));
    go->transform = gobj.transform;
    go->color = gobj.color;
    go->texturePath = gobj.texturePath;

    // Load texture if it exists
    if (std::strlen(go->texturePath.data()) > 0) {
      this->SetGameObjectTexture(go, go->texturePath.data());
    }

    this->AddGameObject(go);
  }

  this->Build();
}
