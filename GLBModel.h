#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>      // <-- для glm::quat
#include <glm/gtx/quaternion.hpp>      // <-- для slerp, mat4_cast
#include <glm/gtx/transform.hpp>       // <-- для translate/scale/rotate
#include <vector>
#include <string>
#include "tinygltf/tiny_gltf.h"
#include "GLBTexture.h"
#include "ShaderProgram.h"

struct GLBPrimitive {
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    GLsizei indexCount = 0;
    GLenum mode = GL_TRIANGLES;
    int materialIndex = -1;
    int skinIndex = -1;
};

struct GLBMaterial {
    GLBTexture* baseColorTexture = nullptr;
    float baseColorFactor[4] = { 1, 1, 1, 1 };
    bool hasTexture = false;
};

struct GLBSkin {
    std::vector<int> jointNodes;
    std::vector<glm::mat4> inverseBindMatrices;
};

struct GLBAnimationChannel {
    int targetNode = -1;
    std::string path;
    int samplerIndex = -1;
};

struct GLBAnimationSampler {
    std::vector<float> times;
    std::vector<float> values;
    int components = 0;
    std::string interpolation;
};

struct GLBAnimation {
    std::string name;
    float duration = 0.0f;
    std::vector<GLBAnimationChannel> channels;
    std::vector<GLBAnimationSampler> samplers;
};

struct NodeTRS {
    glm::vec3 translation = glm::vec3(0.0f);
    glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec3 scale = glm::vec3(1.0f);
};


class GLBModel {
public:
    GLBModel();
    ~GLBModel();

    bool load(const tinygltf::Model& model);
    void render(const glm::mat4& mvp);

    void update(float deltaTime);
    void setAnimation(int index);
    int getAnimationCount() const { return (int)animations.size(); }

private:
    std::vector<GLBPrimitive> primitives;
    std::vector<GLBMaterial> materials;
    std::vector<GLBSkin> skins;
    std::vector<GLBAnimation> animations;
    bool loaded = false;

    std::vector<int> parentOf;
    std::vector<NodeTRS> baseTRS;
    std::vector<glm::mat4> localMatrices;
    std::vector<glm::mat4> globalMatrices;
    std::vector<glm::mat4> skinMatrices; // один скин

    float animTime = 0.0f;
    int currentAnimation = 0;

    ShaderProgram shader;

    void buildNodeHierarchy(const tinygltf::Model& model);
    void buildSkins(const tinygltf::Model& model);
    void buildAnimations(const tinygltf::Model& model);
    void buildMaterial(const tinygltf::Model& model, int matIndex);
    void buildPrimitive(const tinygltf::Model& model,
        const tinygltf::Primitive& prim,
        int materialIndex,
        int skinIndex,
        GLBPrimitive& outPrim);

    glm::mat4 getNodeLocalMatrix(int nodeIndex);
    void updateNodeMatrices();
};