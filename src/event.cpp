#include <windows.h>
#include <cstdlib>
#include <cstdio>
#include "loader/common.hpp"
#include "loader/shader.hpp"
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>
#ifndef M_PI
#define M_PI (3.1415926535897932)
#endif

#include <iostream>


#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/quaternion.hpp>
using namespace glm;
glm::vec3 cameraPos=glm::vec3(0.0f,0.0f,3.0f);
glm::vec3 cameraFront=glm::vec3(0.0f,0.0f,-1.0f);
glm::vec3 cameraUp=glm::vec3(0.0f,1.0f,0.0f);
float cameraSpeed = 0.03f;
static void rotate(glm::fvec3 u,glm::fvec3  &v,float t){
    quat MyQuaternionl=angleAxis(radians(t)/3,u);
    v=rotate(MyQuaternionl,v);
}
quat RotationBetweenVectors(vec3 start, vec3 dest){
    start = normalize(start);
    dest = normalize(dest);
    float cosTheta = dot(start, dest);
    vec3 rotationAxis;
    if (cosTheta < -1 + 0.001f){
        rotationAxis = cross(vec3(0.0f, 0.0f, 1.0f), start);
        if (length2(rotationAxis) < 0.01 ) // bad luck, they were parallel, try again!
        rotationAxis = cross(vec3(1.0f, 0.0f, 0.0f), start);
        rotationAxis = normalize(rotationAxis);
        return angleAxis(180.0f, rotationAxis);
    }

    rotationAxis = cross(start, dest);
    float s = sqrt( (1+cosTheta)*2 );
    float invs = 1 / s;
    return quat(
        s * 0.5f,
        rotationAxis.x * invs,
        rotationAxis.y * invs,
        rotationAxis.z * invs
    );
}
bool flag;
static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)glfwSetWindowShouldClose(window, GLFW_TRUE);
    else if(key==GLFW_KEY_W)cameraPos += 3*cameraSpeed * cameraFront;
    else if(key==GLFW_KEY_S)cameraPos -= 3*cameraSpeed * cameraFront;
    else if(key==GLFW_KEY_A)rotate(cameraFront,cameraUp,cameraSpeed*30);
    else if(key==GLFW_KEY_D)rotate(cameraFront,cameraUp,-cameraSpeed*30);
    else if(key==GLFW_KEY_M&&action==GLFW_PRESS) {
        flag^= 1;
        if(flag)glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        else glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
}
bool f=0;
double lastx,lasty;
static void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if(!f){
        lastx=xpos;
        lasty=ypos;
        f=1;
    }
    if(flag){
        vec3 ls=cross(cameraFront,cameraUp);
        rotate(cameraUp,cameraFront,-xpos+lastx);
        rotate(ls,cameraUp,-ypos+lasty);
        rotate(ls,cameraFront,-ypos+lasty);
    }
    lastx=xpos;lasty=ypos;
}
static void Mouse_callback(GLFWwindow *window, int buttons, int action, int mods) {
    if(buttons==GLFW_MOUSE_BUTTON_LEFT&&action==GLFW_PRESS)f=1;
    if(buttons==GLFW_MOUSE_BUTTON_LEFT&&action==GLFW_RELEASE)f=0;
}
void init_control(GLFWwindow *window) {
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window,Mouse_callback);
    glfwSetCursorPosCallback(window, mouse_callback);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls
    // Setup Dear ImGui style
    // ImGui::StyleColorsDark();
    ImGui::StyleColorsLight();
    //  Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true); // Second param install_callback=true will install GLFW callbacks and chain to existing ones.
    ImGui_ImplOpenGL3_Init();
}
bool ui_window=1;
float fps;
float particle_size=1.5;
float rot_speed=1;
int particle_number=5e3;
LightSource light;
void ui() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    if (ui_window) {
        ImGui::Begin("Earth", &ui_window);
        ImGui::Text("Particle System");
        ImGui::SliderFloat("Particle size:", &particle_size, 0.1, 10);
        ImGui::SliderFloat("Rotate speed", &rot_speed, 0.1, 5);
        ImGui::SliderInt("Particle number", &particle_number, 1000, 1e5);
        ImGui::Text("Light");
        ImGui::SliderFloat("l_pos.x:", &light.position.x, -100, 100);
        ImGui::SliderFloat("l_pos.y:", &light.position.y, -100, 100);
        ImGui::SliderFloat("l_pos.z:", &light.position.z, -100, 100);
        ImGui::SliderFloat("intense.x:", &light.intense.x, 0, 2000);
        ImGui::SliderFloat("intense.y:", &light.intense.y, 0, 2000);
        ImGui::SliderFloat("intense.z:", &light.intense.z, 0, 2000);
        ImGui::End();
    }
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
void render_ui() {
    ui();
}
glm::mat4 get_view_matrix() {
    return glm::lookAt(cameraPos, cameraPos+cameraFront, cameraUp);
}