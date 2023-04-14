#ifndef PROJECT_BASE_LIGHTS_HPP
#define PROJECT_BASE_LIGHTS_HPP

#include <glm/glm.hpp>

struct PointLight {
    glm::vec3 position;

    glm::vec3 specular;
    glm::vec3 diffuse;
    glm::vec3 ambient;

    float constant;
    float linear;
    float quadratic;

    bool enabled;

    PointLight() :
        position(glm::vec3(0.0, 0.0, 4.0)),
        specular(glm::vec3(1.0f)),
        diffuse(glm::vec3(1.6f)),
        ambient(glm::vec3(0.2f)),
        constant(1.0f),
        linear(0.09f),
        quadratic(0.032f),
        enabled(false) {};
};

struct SpotLight {
    glm::vec3 position;
    glm::vec3 direction;
    float cutOff;
    float outerCutOff;

    float constant;
    float linear;
    float quadratic;

    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

    bool enabled;

    SpotLight() :
        position(glm::vec3(0.0, 0.0, 4.0)),
        direction(glm::vec3(0.0, 0.0, -1.0)),
        cutOff(glm::cos(glm::radians(5.5f))),
        outerCutOff(glm::cos(glm::radians(11.0f))),
        constant(1.0f),
        linear(0.09f),
        quadratic(0.032f),
        ambient(glm::vec3(0.4f)),
        diffuse(glm::vec3(3.0f)),
        specular(glm::vec3(3.0f)),
        enabled(false) {};
};

#endif //PROJECT_BASE_LIGHTS_HPP
