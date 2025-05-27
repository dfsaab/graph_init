#pragma once
#include <GL/glew.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include "common.hpp"
#include "material.hpp"
class Shader {
    GLuint _program;
public:
    Shader(const char* vert, const char* frag);
    ~Shader();
    void use();
};

struct LightSource {
    glm::vec3 position;
    glm::vec3 intense;
    glm::vec3 color;
};
class PhongShader: public Shader {
    static const GLint trans = 0, Ka = 1, Kd = 2, scale = 3, type = 4, camera = 5, light = 6;
public:
    PhongShader();
    void set_transform(glm::mat4);
    void set_material(Material *);
    void set_camera(glm::vec3);
    void set_light(LightSource);
};




