#include "shader.hpp"

GLuint load_shader_from_text(const char *source, GLenum type) {
    GLuint shader = glCreateShader(type);
    const char *sources[] = {source};
    glShaderSource(shader, 1, sources, NULL);
    glCompileShader(shader);

    GLint is_compiled = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &is_compiled);
    if (is_compiled == GL_FALSE){
        GLint max_length = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &max_length);
        // The maxLength includes the NULL character
        std::string error_log; error_log.resize(max_length);
        glGetShaderInfoLog(shader, max_length, &max_length, &error_log[0]);
        glDeleteShader(shader); // Don't leak the shader.
        throw error_log;
    }
    return shader;
}
GLuint load_shader_from_path(const char *path, GLenum type) {
    auto source = readFile(path);
    source.push_back('\0');
    return load_shader_from_text(source.data(), type);
}

GLuint link_program(GLuint *shaders, uint32_t shader_count) {
    GLuint program = glCreateProgram();
    for (uint32_t i = 0; i < shader_count; i++) {
      glAttachShader(program, shaders[i]);
    }
    glLinkProgram(program);

  int success;
  // check for linking errors
  glGetProgramiv(program, GL_LINK_STATUS, &success);
  if (!success) {
    GLint max_length = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &max_length);
    std::vector<GLchar> error_log(max_length);
    glGetProgramInfoLog(program, max_length, &max_length, &error_log[0]);
    throw error_log.data();
    glDeleteProgram(program);
  }
  return program;
}
GLuint prepare_shader(const char *vert, const char *frag)
{
    GLuint shaders[2];
    shaders[0] = load_shader_from_text(vert, GL_VERTEX_SHADER);
    CheckGLError();
    shaders[1] = load_shader_from_text(frag, GL_FRAGMENT_SHADER);
    CheckGLError();
    return link_program(shaders, 2);
}



Shader::Shader(const char *vert, const char *frag)
{
    _program = prepare_shader(vert, frag);
    printf("Shader loaded\n");
}
void Shader::use() {
    glUseProgram(_program);
}

Shader::~Shader() {
    printf("shader destroyed\n");
    glDeleteShader(_program);
}


namespace Phong {
static const char *vertex_shader_text = R"(
#version 330 core
#extension GL_ARB_explicit_uniform_location : enable
layout(location = 0) in vec3 position;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec3 normal;
layout(location = 0) uniform mat4 transform;
out vec2 o_uv;
out vec3 o_pos;
out vec3 o_norm;
void main() {
    gl_Position = transform * vec4(position, 1.);
    o_uv=uv;
    o_pos=position;
    o_norm=normal;
}
)";

static const char *fragment_shader_text = R"(
#version 330 core
#extension GL_ARB_explicit_uniform_location : enable
in vec2 o_uv;
in vec3 o_pos;
in vec3 o_norm;
uniform sampler2D tex;
layout(location = 1) uniform vec3 m_ka;
layout(location = 2) uniform vec3 m_kd;
layout(location = 5) uniform vec3 camera;
struct LightSource {vec3 position,intense;};
layout(location = 6) uniform LightSource light;
layout(location = 0) out vec4 frag_color;
void main() {
    vec3 color=m_kd;
    vec3 lo=light.position-o_pos;
    float r=dot(lo,lo);
    lo=normalize(lo);
    vec3 intense=light.intense/r;
    vec3 diffuse=color*intense*max(0,dot(lo,o_norm));
    float alpha=dot(o_norm,normalize((lo+normalize(camera-o_pos))/2));
    vec3 specular;
    if(alpha < 0)specular=vec3(0,0,0);
    else specular=intense*pow(max(0, alpha), 100);
    frag_color=vec4(diffuse+specular,0);
}
)";
}
PhongShader::PhongShader(): Shader(Phong::vertex_shader_text, Phong::fragment_shader_text) {}
void PhongShader::set_material(Material *material) {
    glUniform1i(type,0);
    glUniform3f(Ka,material->Ka.x,material->Ka.y,material->Ka.z);
    glUniform3f(Kd,material->Kd.x,material->Kd.y,material->Kd.z);
}
void PhongShader::set_transform(glm::mat4 transform) {
    glUniformMatrix4fv(trans,1,false,(GLfloat *)&transform);
}
void PhongShader::set_light(LightSource l) {
    glUniform3f(light,l.position.x,l.position.y,l.position.z);
    glUniform3f(light+1,l.intense.x,l.intense.y,l.intense.z);
}
void PhongShader::set_camera(glm::vec3 cam) {
    glUniform3f(camera,cam.x, cam.y, cam.z);
}