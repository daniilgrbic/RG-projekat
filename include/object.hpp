#ifndef OBJECT_HPP
#define OBJECT_HPP

#include <glm/glm.hpp>

#include <learnopengl/model.h>
#include <learnopengl/shader.h>

#include <iostream>
#include <map>
#include <string>

class Object {
public:
    Object();
    ~Object();
    void setModel(Model* model);
    void setShader(Shader* shader);
    void setScale(glm::vec3 scale);
    void setRotation(glm::mat4 rotation);
    glm::vec3 getPosition();
    void render();
private:
    std::string name = "Object";
    Object* parent = nullptr;
    glm::vec3 position = glm::vec3(0);
    glm::mat4 rotation = glm::mat4(1.0f);
    bool positionRelative = false;
    glm::vec3 scale = glm::vec3(1);
    Model* model = nullptr;
    Shader* shader = nullptr;
};

Object::Object() = default;

Object::~Object() = default;

void Object::setModel(Model *model) {
    this->model = model;
    this->model->SetShaderTextureNamePrefix("material.");
}

void Object::setShader(Shader *shader) {
    this->shader = shader;
}

void Object::setScale(glm::vec3 scale) {
    this->scale = scale;
}

void Object::setRotation(glm::mat4 rotation) {
    this->rotation = rotation;
}

glm::vec3 Object::getPosition() {
    if(positionRelative) 
        return position + parent->getPosition();
    else
        return position;
}

void Object::render() {
    //std::cout << "rendering " << this->name << std::endl;
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    modelMatrix = modelMatrix * this->rotation;
    modelMatrix = glm::scale(modelMatrix, scale);
    modelMatrix = glm::translate(modelMatrix, getPosition());
    //shader->use();
    shader->setMat4("model", modelMatrix);
    model->Draw(*shader);
}

#endif //OBJECT_HPP
