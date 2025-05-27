#include <GL/glew.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#define GLM_FORCE_SWIZZLE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include "util.cpp"
#include <optional>
#include "loader/loader.hpp"
#include "loader/bound.hpp"
#include "loader/particle.hpp"
#include "event.cpp"
static const int width=1200,height=800;

class Application {
    GLFWwindow *window;
    std::unique_ptr <Mesh> mesh;
    std::unique_ptr <ParticleSystem> ps;
public:
    Application(): mesh(nullptr) { }
    ~Application() {
        mesh=nullptr,ps=nullptr;
        glfwDestroyWindow(window);
        glfwTerminate();
    }
    void init() {
        window=window_init(width,height,"earth");
        glew_init();
        light.position=glm::vec3(30, 3, 30);
        light.intense=glm::vec3(1000, 1000, 1000);
        init_control(window);
    }
    void load(const Path &path) {
        mesh=std::make_unique<Mesh>(path);
        mesh->apply_transform(mesh->bound().to_local());
        mesh->init_draw();
        ps=std::make_unique<ParticleSystem>(glm::vec3(0,0,0),1,2,1e-2);
        ps->build(1e5);
    }
    void run(const Path &path) {
        init();load(path);
        double last=glfwGetTime();
        int frame_count=0;
        while(!glfwWindowShouldClose(window)) {
            double now=glfwGetTime();
            int width, height;
            glfwGetFramebufferSize(window, &width, &height);
            glViewport(0, 0, width, height);
            glClearColor(0.2, 0.3, 0.3, 1.0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_LESS);
            auto vp=glm::perspective(45.f, 1.f * width / height, .1f, 100.f)*get_view_matrix();
            mesh->draw(vp*glm::rotate(glm::mat4(1.f),float(now/50)*rot_speed,glm
            ::vec3(0, 1, 0)),cameraPos, light);
            ps->set_particle_size(1e-2*particle_size);
            ps->draw(particle_number,vp,cameraPos,float(now/100)*rot_speed,light);
            render_ui();
            glfwPollEvents();
            glfwSwapBuffers(window);
        }
    }
}app;
int main(int argc, char **argv) {
    if(argc<=1){
        puts("You must provide a path to .obj file.");
        std::string str = "../../data/models/65-mercury_1k/Mercury 1K.obj";
        fs::path filePath(str);
        app.run(filePath);
        return 0;
    }
    app.run(argv[1]);
}
