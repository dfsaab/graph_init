#include "particle.hpp"
static const char *vertex_shader_text = R"(
#version 330 core
#extension GL_ARB_explicit_uniform_location : enable


layout(location = 0) uniform vec3 center;
layout(location = 1) uniform float v_angle;
layout(location = 2) uniform float particle_size;
layout(location = 3) uniform mat4 transform;

layout(location = 0) in vec3 direction;
layout(location = 1) in vec3 diffuse;
layout(location = 2) in float angle_bias;
layout(location = 3) in float r;
layout(location = 4) in float speed;
layout(location = 5) in float size;

out vec3 o_pos;
out vec3 o_norm;
out vec3 o_diffuse;

void main() {
    float angle = angle_bias + v_angle * speed;
    o_pos = center + vec3(r * cos(angle), 0, r * sin(angle) ) + direction * particle_size * size;
    gl_Position = transform * vec4(o_pos, 1);
    o_norm = direction;
    o_diffuse = diffuse;
}
)";

static const char *fragment_shader_text = R"(
#version 330 core
#extension GL_ARB_explicit_uniform_location : enable

in vec3 o_pos;
in vec3 o_norm;
in vec3 o_diffuse;

layout(location = 4) uniform vec3 camera;
struct LightSource {
    vec3 position;
    vec3 intense;
};
layout(location = 5) uniform LightSource light;

layout(location = 0) out vec4 frag_color;


void main() {
    vec3 i = light.position - o_pos;
    float r = dot(i, i);
    i = normalize(i);
    vec3 v = normalize(camera - o_pos);
    
    vec3 h = normalize((i + v) / 2);
    float theta = dot(i, o_norm);

    vec3 intense = light.intense / r;
    vec3 diffuse = o_diffuse * intense * max(0, theta);

    float alpha = dot(o_norm, h);
    
    vec3 specular = intense * pow(max(0, alpha), 100);
    vec3 ambient = vec3(0, 0, 0);

    frag_color = vec4(diffuse + specular + ambient, 0);
}
)";
ParticleSystem::ParticleSystem(glm::vec3 center,float lr,float rr,float particle_size) 
    :  center(center),particle_size(particle_size),lr(lr), rr(rr),eng(1234){
    shader=std::make_unique<ParticleShader>();
    vbo=vao=0;
}
void ParticleSystem::set_particle_size(float size){
    particle_size=size;
}
ParticleShader::ParticleShader() : Shader(vertex_shader_text, fragment_shader_text) {;}
void ParticleShader::set(glm::vec3 center,float particle_size,glm::mat4 transform,float v_angle,glm::vec3 camera,LightSource light){    
    glUniform3f(_center,center.x,center.y,center.z);
    glUniform1f(_particle_size, particle_size);
    glUniformMatrix4fv(_transform,1,false,(GLfloat*)&transform);
    glUniform1f(_v_angle,v_angle);
    glUniform3f(_camera,camera.x,camera.y,camera.z);
    glUniform3f(_light,light.position.x,light.position.y,light.position.z);
    glUniform3f(_light+1,light.intense.x, light.intense.y,light.intense.z);
}
void ParticleSystem::draw(int size, glm::mat4 transform, glm::vec3 camera, float v_angle, LightSource light){
    glBindVertexArray(vao);
    shader->use();
    shader->set(center,particle_size,transform,v_angle,camera,light);
    glDrawElements(GL_TRIANGLES, (GLsizei)size * 12, GL_UNSIGNED_INT, nullptr);
}