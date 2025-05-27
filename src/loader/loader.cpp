#include "loader.hpp"
#include <stb_image.h>

Vertex::Vertex() : position(0, 0, 0),
                   uv(0, 0),
                   normal(0, 0, 0) {}

Object::Object(const std::string &name,
               const std::vector<uint32_t> &triangles,
               Material *material) : name(name), triangles(triangles), _material(material) {
    if (_material)
        _material->verify();
    vao = 0;
}

Material *Object::material() const {
    return _material;
}

Mesh::Mesh(const Path &path) {
    mtl = std::make_unique<MaterialLib>();
    printf("Mesh_loader: Load from %s\n", path.u8string().c_str());
    clock_t begin_time = clock();
    auto filename = path.filename().u8string();
    FILE *f = fopen(path.u8string().c_str(), "r");
    if(f == nullptr) 
        throw "fail to open file";
    if(filename.size() < 4 || filename.substr(filename.size() - 4, 4) != ".obj") 
        warn(2, "%s: not a obj file", filename.c_str());
    static char buf[BUFFLEN];
    Material *cur = nullptr;
    int count = 0;
    std::vector <glm::vec3> positions;
    std::vector <glm::vec2> uvs;
    std::vector <glm::vec3> normals;
    std::vector <uint32_t> triangles;
    std::map <VertexIndices, uint32_t> map;
    std::string name;
    while(std::fgets(buf, BUFFLEN, f) != nullptr) {
        size_t len = strlen(buf);
        while(len && isspace(buf[len - 1])) buf[--len] = 0;
        if(len == BUFFLEN) warn(3, "Obj: Line length exceeded bufflen");
        char *pos = strstr(buf, "#");
        if(pos != nullptr) *pos = '\0';
        pos = buf;
        while(*pos && isspace(*pos)) pos++;
        len = strlen(pos);
        if(len <= 1) continue;
        if(str_equal(pos, "mtllib ")) {
            mtl -> load((path.parent_path() /= Path(unescape(pos + 7))));
        } else if(str_equal(pos, "o ")) {
            if(count) {
                objects.emplace_back(name, triangles, cur);
                triangles.clear();
            }
            name = pos + 2;
            printf("new object: %s\n", pos + 2);
            count++;
        } else if(str_equal(pos, "v ")) {
            glm::vec3 v;
            readvec3(pos + 2, &v, "vertex position");
            positions.push_back(v);
        } else if(str_equal(pos, "vt ")) {
            glm::vec2 vt;
            readvec2(pos + 3, &vt, "texture coords");
            uvs.push_back(vt);
        } else if(str_equal(pos, "vn ")) {
            glm::vec3 vn;
            readvec3(pos + 3, &vn, "normal");
            normals.push_back(vn);
        } else if(str_equal(pos, "s ")) {
            warn(0, "Ignore: %s", pos);
        } else if(str_equal(pos, "usemtl")) {
            cur = (*mtl)[pos + 7];
        } else if(str_equal(pos, "f ")) {
            pos += 2;
            while(*pos && isspace(*pos)) pos++;
            std::vector <uint32_t> face;
            while(*pos) {
                VertexIndices ind{0,0,0};
                char *nxt = strstr(pos, " ");
                if(nxt == nullptr) {
                    nxt = pos;
                    while(*nxt) nxt++;
                }
                char *p1 = strstr(pos, "/");
                char *p2 = p1 && p1 < nxt? strstr(p1 + 1, "/") : nullptr;
                sscanf_s(pos, "%u", &ind.positionIndex);
                if(p1 && p1 + 1 != p2) {
                    sscanf_s(p1 + 1, "%u", &ind.uvIndex);
                    if(p2) {
                        sscanf_s(p2 + 1, "%u", &ind.normalIndex);
                    }
                }
                pos = nxt;
                while(*pos && isspace(*pos)) pos++;
                if(!map.count(ind)) {
                    if(!ind.positionIndex) {
                        throw "error: face: v=0";
                    }
                    if(ind.positionIndex > positions.size()) {
                        throw "error: face: v > size";
                    }
                    if(ind.uvIndex > uvs.size()) {
                        throw "error: face: vt > size";
                    }
                    if(ind.normalIndex > normals.size()) {
                        throw "error: face: vn > size";
                    }
                    Vertex v;
                    v.position = positions[ind.positionIndex - 1];
                    if(ind.uvIndex) v.uv = uvs[ind.uvIndex - 1];
                    if(ind.normalIndex) v.normal = normals[ind.normalIndex- 1];
                    map[ind] = (uint32_t)vertices.size();
                    vertices.push_back(v);
                }
                face.push_back(map[ind]);
            }
            for(int i = 1; i < (int)face.size() - 1; ++i) {
                /* split face into triangle */
                triangles.push_back(face[0]);
                triangles.push_back(face[i]);
                triangles.push_back(face[i + 1]);
            }
        } else {
            char *tmp = strstr(pos, " ");
            if(tmp != nullptr) *tmp = '\0';
            warn(1, "Obj: unsupported argument: %s", pos);
        }
    }
    if (count) {
        objects.emplace_back(name, triangles, cur);
    }
    printf("Obj loaded, time: %lfs\n", 1. * (clock() - begin_time) / CLOCKS_PER_SEC);
    shader = std::make_unique <PhongShader> ();

    vertex_buffer = 0;
}

const char* Object::c_name() const {
    return name.c_str();
}
void Object::init_draw() {
    glGenVertexArrays(1, &vao);
    /*
    vao stores:
        (1) Vertex Attribute State [Pointers, Enable/Disable]
        (2) Element Array Buffer Binding (GL_ELEMENT_ARRAY_BUFFER)
    */
    glBindVertexArray(vao);

    GLuint element_buffer;
    glGenBuffers(1, &element_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, element_buffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 sizeof(uint32_t) * triangles.size(),
                 triangles.data(),
                 GL_STATIC_DRAW);
    
    glVertexAttribPointer(
        0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, position));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, uv));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        2, 3, GL_FLOAT, GL_TRUE, sizeof(Vertex), (void *)offsetof(Vertex, normal));
    glEnableVertexAttribArray(2);
    
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}
void Object::draw() const {
    assert(vao != 0);
    // printf("(%u %d)", vao, (int)triangles.size());
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, (GLsizei)triangles.size(), GL_UNSIGNED_INT, nullptr);
}

Bound Mesh::bound() {
    Bound b;
    for(const auto i: vertices) b += i.position;
    return b;
}
void Mesh::apply_transform(glm::mat4 trans) {
    for(auto &vertex: vertices) 
        vertex.position = apply_transform_vec3(vertex.position, trans);
}
void Mesh::init_draw() {
    glGenBuffers(1, &vertex_buffer);
    CheckGLError();
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    CheckGLError();
    glBufferData(GL_ARRAY_BUFFER,
                 sizeof(Vertex) * vertices.size(),
                 vertices.data(),
                 GL_STATIC_DRAW);
    CheckGLError();
    
    for(auto &object: objects) object.init_draw();
    // The Array buffer must be present when vao is specified.
    // Therefore unbind after objects initiated.
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    CheckGLError();
}
void Mesh::draw(const glm::mat4 &trans, const glm::vec3 &camera, const LightSource &light) const {
    shader->use();
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    shader->set_light(light);
    shader->set_camera(camera);
    shader->set_transform(trans);
    for(const auto &object: objects) {
        shader->set_material(object.material());
        object.draw();
    }
}





