#pragma once
#include "common.hpp"
#include "shader.hpp"
#include <random>
struct Vertexinfo{
    glm::vec3 direction,diffuse;
    float angle_bias,r,speed,size  ;
};
class ParticleShader:public Shader{
    static const int _center=0,_v_angle=1,_particle_size=2,_transform=3,_camera=4,_light=5;
public:
    ParticleShader();
    void set(glm::vec3 center,float particle_size,glm::mat4 transform,float v_angle,glm::vec3 camera,LightSource light);
};
class ParticleSystem{
    std::unique_ptr<ParticleShader> shader;
    std::vector<Vertexinfo> vertices;
    glm::vec3 center;
    float particle_size,lr,rr;
    GLuint vao,vbo;
    std::mt19937 eng;
    double _rand(double l,double r);
public:
    ParticleSystem(glm::vec3 center, float lr, float rr, float particle_size);
    ~ParticleSystem();
    void set_particle_size(float size);
    void draw(int size, glm::mat4 transform, glm::vec3 camera, float v_angle, LightSource light);
};