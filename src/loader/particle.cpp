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
    float angle=angle_bias+v_angle*speed;
    o_pos=center+vec3(r*cos(angle),0,r*sin(angle))+direction*particle_size*size;
    gl_Position=transform*vec4(o_pos,1);
    o_norm=direction;
    o_diffuse=diffuse;
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
    vec3 lo=light.position-o_pos;
    float r=dot(lo,lo);
    lo=normalize(lo);
    vec3 intense=light.intense/r;
    vec3 diffuse=o_diffuse*intense*max(0,dot(lo,o_norm));
    float alpha=dot(o_norm,normalize((lo+normalize(camera-o_pos))/2));
    vec3 specular;
    if(alpha < 0)specular=vec3(0,0,0);
    else specular=intense*pow(max(0, alpha), 100);
    frag_color=vec4(diffuse+specular,0);
}
)";
double ParticleSystem::_rand(double l, double r){
    return std::uniform_real_distribution <> (l, r)(eng);
}
ParticleSystem::~ParticleSystem() {
    if(vao)glDeleteVertexArrays(1,&vao);
    if(vbo)glDeleteBuffers(1,&vbo);
}
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
void ParticleSystem::build(int size) {
    vertices.resize(size*4);
    indices.resize(size*12);
    double c=cos(PI/6),s=sin(PI/6);
    glm::vec3 ve[4]={
        glm::vec3(0,c,0),
        glm::vec3(-c,0,-1/4),
        glm::vec3(c,0,-1/4),
        glm::vec3(0,0,s)
    };
    for(int i=0;i<size;++i){
        glm::mat4 transform =
            glm::rotate(glm::rotate(glm::rotate(glm::mat4(1.f), (float)_rand(0, 2 * PI), glm::vec3(0, 0, 1)),
                        (float)_rand(0, 2 * PI), glm::vec3(0, 1, 0)),
                        (float)_rand(0, 2 * PI), glm::vec3(0, 0, 1));
        
        glm::vec3 diffuse(_rand(50,100),_rand(50,150),_rand(50,200));
        diffuse/=255;
        float angle=_rand(0,2*PI);
        float dis=(float)_rand(lr,rr),s=(float)_rand(0.5,3),sz=(float)_rand(0.5,2);
        for(int j=0;j<4;++j)vertices[i<<2|j]={(transform* glm::vec4(ve[j],1)).xyz(),
            diffuse,angle,dis,s,sz};
        for(int j=0;j<4;++j)for(int k=0;k<3;++k) indices[i*12+(j*3+k)]=(j+k)%4+i*4;
    }
    if(vao)glDeleteVertexArrays(1, &vao);
    if(vbo)glDeleteBuffers(1, &vbo);
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 sizeof(Vertexinfo) * vertices.size(),
                 vertices.data(),
                 GL_STATIC_DRAW);
    
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    GLuint element_buffer;
    glGenBuffers(1, &element_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, element_buffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 sizeof(GLuint) * indices.size(),
                 indices.data(),
                 GL_STATIC_DRAW);
    glVertexAttribPointer(
        0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertexinfo), (void *)offsetof(Vertexinfo, direction));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertexinfo), (void *)offsetof(Vertexinfo, diffuse));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        2, 1, GL_FLOAT, GL_FALSE, sizeof(Vertexinfo), (void *)offsetof(Vertexinfo, angle_bias));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(
        3, 1, GL_FLOAT, GL_FALSE, sizeof(Vertexinfo), (void *)offsetof(Vertexinfo, r));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(
        4, 1, GL_FLOAT, GL_FALSE, sizeof(Vertexinfo), (void *)offsetof(Vertexinfo, speed));
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(
        5, 1, GL_FLOAT, GL_FALSE, sizeof(Vertexinfo), (void *)offsetof(Vertexinfo, size));
    glEnableVertexAttribArray(5);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}
void ParticleSystem::draw(int size, glm::mat4 transform, glm::vec3 camera, float v_angle, LightSource light){
    glBindVertexArray(vao);
    shader->use();
    shader->set(center,particle_size,transform,v_angle,camera,light);
    glDrawElements(GL_TRIANGLES, (GLsizei)size * 12, GL_UNSIGNED_INT, nullptr);
}