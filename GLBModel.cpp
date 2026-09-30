// GLBModel.cpp
#include "GLBModel.h"
#include <iostream>
#include <cstring>

// ============================================================
// ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ
// ============================================================

static bool extractFloatData(const tinygltf::Model& model,
    const tinygltf::Accessor& acc,
    int componentsPerElement,
    std::vector<float>& out) {
    const tinygltf::BufferView& bv = model.bufferViews[acc.bufferView];
    const tinygltf::Buffer& buf = model.buffers[bv.buffer];

    size_t elementSize = componentsPerElement * sizeof(float);
    size_t stride = bv.byteStride ? bv.byteStride : elementSize;
    size_t offset = bv.byteOffset + acc.byteOffset;

    out.resize(acc.count * componentsPerElement);
    const unsigned char* base = buf.data.data() + offset;

    for (size_t i = 0; i < acc.count; i++) {
        const float* src = reinterpret_cast<const float*>(base + i * stride);
        for (int c = 0; c < componentsPerElement; c++) {
            out[i * componentsPerElement + c] = src[c];
        }
    }
    return true;
}

static bool extractIndexData(const tinygltf::Model& model,
    const tinygltf::Accessor& acc,
    std::vector<unsigned int>& out) {
    const tinygltf::BufferView& bv = model.bufferViews[acc.bufferView];
    const tinygltf::Buffer& buf = model.buffers[bv.buffer];

    size_t offset = bv.byteOffset + acc.byteOffset;
    const unsigned char* base = buf.data.data() + offset;
    out.resize(acc.count);

    if (acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
        const uint16_t* src = reinterpret_cast<const uint16_t*>(base);
        for (size_t i = 0; i < acc.count; i++) out[i] = src[i];
    }
    else if (acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
        const uint32_t* src = reinterpret_cast<const uint32_t*>(base);
        for (size_t i = 0; i < acc.count; i++) out[i] = src[i];
    }
    else if (acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
        for (size_t i = 0; i < acc.count; i++) out[i] = base[i];
    }
    else {
        std::cerr << "Неподдерживаемый тип индексов: " << acc.componentType << std::endl;
        return false;
    }
    return true;
}

static bool extractJoints(const tinygltf::Model& model,
    const tinygltf::Accessor& acc,
    std::vector<float>& out) {
    const tinygltf::BufferView& bv = model.bufferViews[acc.bufferView];
    const tinygltf::Buffer& buf = model.buffers[bv.buffer];

    size_t offset = bv.byteOffset + acc.byteOffset;
    const unsigned char* base = buf.data.data() + offset;

    size_t elemSize = 0;
    if (acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) elemSize = 1;
    else if (acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) elemSize = 2;
    else elemSize = 4;

    size_t stride = bv.byteStride ? bv.byteStride : elemSize * 4;
    out.resize(acc.count * 4);

    for (size_t i = 0; i < acc.count; i++) {
        const unsigned char* p = base + i * stride;
        if (acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
            for (int k = 0; k < 4; k++) out[i * 4 + k] = (float)p[k];
        }
        else if (acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
            const uint16_t* p16 = reinterpret_cast<const uint16_t*>(p);
            for (int k = 0; k < 4; k++) out[i * 4 + k] = (float)p16[k];
        }
    }
    return true;
}

// ============================================================
// КОНСТРУКТОР / ДЕСТРУКТОР
// ============================================================

GLBModel::GLBModel() {}

GLBModel::~GLBModel() {
    for (auto& p : primitives) {
        if (p.vao) glDeleteVertexArrays(1, &p.vao);
        if (p.vbo) glDeleteBuffers(1, &p.vbo);
        if (p.ebo) glDeleteBuffers(1, &p.ebo);
    }
    for (auto& m : materials) {
        delete m.baseColorTexture;
    }
    shader.destroy();
}

// ============================================================
// ЗАГРУЗКА
// ============================================================

bool GLBModel::load(const tinygltf::Model& model) {
    // Загружаем шейдер
    if (!shader.loadFromFiles("resources/skinning.vert",
        "resources/skinning.frag")) {
        std::cerr << "Не удалось загрузить шейдер скиннинга" << std::endl;
        return false;
    }

    buildNodeHierarchy(model);
    buildSkins(model);
    buildAnimations(model);

    for (size_t i = 0; i < model.materials.size(); i++)
        buildMaterial(model, (int)i);

    // Карта mesh -> skin
    std::vector<int> meshToSkin(model.meshes.size(), -1);
    for (const auto& node : model.nodes) {
        if (node.mesh >= 0 && node.skin >= 0)
            meshToSkin[node.mesh] = node.skin;
    }

    for (size_t meshIdx = 0; meshIdx < model.meshes.size(); meshIdx++) {
        const auto& mesh = model.meshes[meshIdx];
        int skinIndex = meshToSkin[meshIdx];
        for (const auto& prim : mesh.primitives) {
            GLBPrimitive p;
            buildPrimitive(model, prim, prim.material, skinIndex, p);
            if (p.vao != 0) primitives.push_back(p);
        }
    }

    updateNodeMatrices();
    loaded = !primitives.empty();
    return loaded;
}
// ============================================================
// ИЕРАРХИЯ УЗЛОВ
// ============================================================

void GLBModel::buildNodeHierarchy(const tinygltf::Model& model) {
    parentOf.assign(model.nodes.size(), -1);
    baseTRS.resize(model.nodes.size());

    for (size_t i = 0; i < model.nodes.size(); i++) {
        for (int child : model.nodes[i].children) {
            if (child >= 0 && child < (int)parentOf.size()) {
                parentOf[child] = (int)i;
            }
        }

        const tinygltf::Node& node = model.nodes[i];
        if (node.translation.size() == 3) {
            baseTRS[i].translation = glm::vec3(
                node.translation[0], node.translation[1], node.translation[2]);
        }
        if (node.scale.size() == 3) {
            baseTRS[i].scale = glm::vec3(
                node.scale[0], node.scale[1], node.scale[2]);
        }
        if (node.rotation.size() == 4) {
            baseTRS[i].rotation = glm::quat(
                (float)node.rotation[3],
                (float)node.rotation[0],
                (float)node.rotation[1],
                (float)node.rotation[2]);
        }
    }

    localMatrices.assign(model.nodes.size(), glm::mat4(1.0f));
    globalMatrices.assign(model.nodes.size(), glm::mat4(1.0f));
    int violations = 0;
    for (size_t i = 0; i < parentOf.size(); i++) {
        if (parentOf[i] > (int)i) violations++;
    }
    fprintf(stderr, "Нарушений порядка (parent > child): %d\n", violations);
}

// ============================================================
// СКЕЛЕТЫ
// ============================================================

void GLBModel::buildSkins(const tinygltf::Model& model) {
    for (const auto& skin : model.skins) {
        GLBSkin s;
        s.jointNodes = skin.joints;

        if (skin.inverseBindMatrices >= 0) {
            const tinygltf::Accessor& acc = model.accessors[skin.inverseBindMatrices];
            std::vector<float> raw;
            extractFloatData(model, acc, 16, raw);

            s.inverseBindMatrices.resize(s.jointNodes.size());
            for (size_t i = 0; i < s.jointNodes.size(); i++) {
                glm::mat4 m;
                for (int r = 0; r < 4; r++) {
                    for (int c = 0; c < 4; c++) {
                        m[c][r] = raw[i * 16 + c * 4 + r];
                    }
                }
                s.inverseBindMatrices[i] = m;
            }
        }
        skins.push_back(s);
    }
}

// ============================================================
// АНИМАЦИИ
// ============================================================

void GLBModel::buildAnimations(const tinygltf::Model& model) {
    for (const auto& anim : model.animations) {
        GLBAnimation a;
        a.name = anim.name;

        for (const auto& samp : anim.samplers) {
            GLBAnimationSampler s;
            s.interpolation = samp.interpolation.empty() ? "LINEAR" : samp.interpolation;

            if (samp.input >= 0) {
                const tinygltf::Accessor& acc = model.accessors[samp.input];
                extractFloatData(model, acc, 1, s.times);
            }

            if (samp.output >= 0) {
                const tinygltf::Accessor& acc = model.accessors[samp.output];
                int comps = 4;
                switch (acc.type) {
                case TINYGLTF_TYPE_VEC3:   comps = 3; break;
                case TINYGLTF_TYPE_VEC4:   comps = 4; break;
                case TINYGLTF_TYPE_SCALAR: comps = 1; break;
                default:                   comps = 4; break;
                }
                s.components = comps;
                extractFloatData(model, acc, comps, s.values);
            }

            a.samplers.push_back(s);
        }

        for (const auto& ch : anim.channels) {
            GLBAnimationChannel c;
            c.targetNode = ch.target_node;
            c.path = ch.target_path;
            c.samplerIndex = ch.sampler;
            a.channels.push_back(c);
        }

        float maxTime = 0.0f;
        for (const auto& s : a.samplers) {
            if (!s.times.empty()) {
                maxTime = std::max(maxTime, s.times.back());
            }
        }
        a.duration = maxTime;

        animations.push_back(a);
    }
}

// ============================================================
// МАТЕРИАЛЫ
// ============================================================

void GLBModel::buildMaterial(const tinygltf::Model& model, int matIndex) {
    GLBMaterial mat;

    if (matIndex < 0 || matIndex >= (int)model.materials.size()) {
        materials.push_back(mat);
        return;
    }

    const tinygltf::Material& m = model.materials[matIndex];
    const auto& pbr = m.pbrMetallicRoughness;

    if (pbr.baseColorFactor.size() == 4) {
        for (int i = 0; i < 4; i++) {
            mat.baseColorFactor[i] = (float)pbr.baseColorFactor[i];
        }
    }

    if (pbr.baseColorTexture.index >= 0) {
        int texIndex = pbr.baseColorTexture.index;
        if (texIndex < (int)model.textures.size()) {
            int imageIndex = model.textures[texIndex].source;
            if (imageIndex >= 0 && imageIndex < (int)model.images.size()) {
                const tinygltf::Image& img = model.images[imageIndex];

                GLBTexture* tex = new GLBTexture();
                if (tex->loadFromPixels(img.image.data(),
                    img.width,
                    img.height,
                    img.component)) {
                    mat.baseColorTexture = tex;
                    mat.hasTexture = true;
                }
                else {
                    delete tex;
                }
            }
        }
    }

    materials.push_back(mat);
}

// ============================================================
// ПРИМИТИВЫ
// ============================================================

void GLBModel::buildPrimitive(const tinygltf::Model& model,
    const tinygltf::Primitive& prim,
    int materialIndex,
    int skinIndex,
    GLBPrimitive& outPrim) {
    auto itPos = prim.attributes.find("POSITION");
    if (itPos == prim.attributes.end()) return;

    const tinygltf::Accessor& posAcc = model.accessors[itPos->second];
    std::vector<float> positions;
    extractFloatData(model, posAcc, 3, positions);

    std::vector<float> normals;
    auto itNorm = prim.attributes.find("NORMAL");
    if (itNorm != prim.attributes.end())
        extractFloatData(model, model.accessors[itNorm->second], 3, normals);

    std::vector<float> uvs;
    auto itUv = prim.attributes.find("TEXCOORD_0");
    if (itUv != prim.attributes.end())
        extractFloatData(model, model.accessors[itUv->second], 2, uvs);

    std::vector<unsigned int> indices;
    if (prim.indices >= 0) {
        extractIndexData(model, model.accessors[prim.indices], indices);
    }
    else {
        indices.resize(posAcc.count);
        for (size_t i = 0; i < indices.size(); i++) indices[i] = (unsigned int)i;
    }

    std::vector<float> joints, weights;
    auto itJ = prim.attributes.find("JOINTS_0");
    if (itJ != prim.attributes.end())
        extractJoints(model, model.accessors[itJ->second], joints);

    auto itW = prim.attributes.find("WEIGHTS_0");
    if (itW != prim.attributes.end())
        extractFloatData(model, model.accessors[itW->second], 4, weights);

    // --- Interleaved vertex data: pos(3) + normal(3) + uv(2) + joints(4) + weights(4) = 16 ---
    size_t vertexCount = posAcc.count;
    std::vector<float> vertexData(vertexCount * 16);

    for (size_t i = 0; i < vertexCount; i++) {
        vertexData[i * 16 + 0] = positions[i * 3 + 0];
        vertexData[i * 16 + 1] = positions[i * 3 + 1];
        vertexData[i * 16 + 2] = positions[i * 3 + 2];

        if (!normals.empty()) {
            vertexData[i * 16 + 3] = normals[i * 3 + 0];
            vertexData[i * 16 + 4] = normals[i * 3 + 1];
            vertexData[i * 16 + 5] = normals[i * 3 + 2];
        }
        else {
            vertexData[i * 16 + 3] = 0; vertexData[i * 16 + 4] = 1; vertexData[i * 16 + 5] = 0;
        }

        if (!uvs.empty()) {
            vertexData[i * 16 + 6] = uvs[i * 2 + 0];
            vertexData[i * 16 + 7] = uvs[i * 2 + 1];
        }
        else {
            vertexData[i * 16 + 6] = 0; vertexData[i * 16 + 7] = 0;
        }

        for (int k = 0; k < 4; k++) {
            vertexData[i * 16 + 8 + k] = (i * 4 + k < joints.size()) ? joints[i * 4 + k] : 0.0f;
            vertexData[i * 16 + 12 + k] = (i * 4 + k < weights.size()) ? weights[i * 4 + k] : 0.0f;
        }
    }

    // --- VAO / VBO / EBO ---
    glGenVertexArrays(1, &outPrim.vao);
    glBindVertexArray(outPrim.vao);

    glGenBuffers(1, &outPrim.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, outPrim.vbo);
    glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float),
        vertexData.data(), GL_STATIC_DRAW);

    glGenBuffers(1, &outPrim.ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, outPrim.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int),
        indices.data(), GL_STATIC_DRAW);

    GLsizei stride = 16 * sizeof(float);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));

    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, stride, (void*)(8 * sizeof(float)));

    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, stride, (void*)(12 * sizeof(float)));

    glBindVertexArray(0);

    outPrim.indexCount = (GLsizei)indices.size();
    outPrim.materialIndex = materialIndex;
    outPrim.skinIndex = skinIndex;

    switch (prim.mode) {
    case TINYGLTF_MODE_TRIANGLES:      outPrim.mode = GL_TRIANGLES;      break;
    case TINYGLTF_MODE_TRIANGLE_STRIP: outPrim.mode = GL_TRIANGLE_STRIP; break;
    case TINYGLTF_MODE_TRIANGLE_FAN:   outPrim.mode = GL_TRIANGLE_FAN;   break;
    default:                           outPrim.mode = GL_TRIANGLES;      break;
    }
}
// ============================================================
// АНИМАЦИЯ — вычисление матриц
// ============================================================

glm::mat4 GLBModel::getNodeLocalMatrix(int nodeIndex) {
    if (nodeIndex < 0 || nodeIndex >= (int)baseTRS.size()) {
        return glm::mat4(1.0f);
    }

    glm::vec3 T = baseTRS[nodeIndex].translation;
    glm::quat R = baseTRS[nodeIndex].rotation;
    glm::vec3 S = baseTRS[nodeIndex].scale;

    if (currentAnimation >= 0 && currentAnimation < (int)animations.size()) {
        const GLBAnimation& a = animations[currentAnimation];

        for (const auto& ch : a.channels) {
            if (ch.targetNode != nodeIndex) continue;
            if (ch.samplerIndex < 0 || ch.samplerIndex >= (int)a.samplers.size()) continue;

            const GLBAnimationSampler& s = a.samplers[ch.samplerIndex];
            if (s.times.empty()) continue;

            float t = animTime;
            size_t i0 = 0, i1 = 0;
            float alpha = 0.0f;

            for (size_t k = 0; k + 1 < s.times.size(); k++) {
                if (t >= s.times[k] && t <= s.times[k + 1]) {
                    i0 = k;
                    i1 = k + 1;
                    float dt = s.times[i1] - s.times[i0];
                    alpha = (dt > 0.0f) ? (t - s.times[i0]) / dt : 0.0f;
                    break;
                }
            }
            if (t >= s.times.back()) {
                i0 = i1 = s.times.size() - 1;
                alpha = 0.0f;
            }

            if (ch.path == "translation") {
                glm::vec3 v0(s.values[i0 * 3 + 0], s.values[i0 * 3 + 1], s.values[i0 * 3 + 2]);
                glm::vec3 v1(s.values[i1 * 3 + 0], s.values[i1 * 3 + 1], s.values[i1 * 3 + 2]);
                T = glm::mix(v0, v1, alpha);
            }
            else if (ch.path == "scale") {
                glm::vec3 v0(s.values[i0 * 3 + 0], s.values[i0 * 3 + 1], s.values[i0 * 3 + 2]);
                glm::vec3 v1(s.values[i1 * 3 + 0], s.values[i1 * 3 + 1], s.values[i1 * 3 + 2]);
                S = glm::mix(v0, v1, alpha);
            }
            else if (ch.path == "rotation") {
                glm::quat q0(s.values[i0 * 4 + 3], s.values[i0 * 4 + 0],
                    s.values[i0 * 4 + 1], s.values[i0 * 4 + 2]);
                glm::quat q1(s.values[i1 * 4 + 3], s.values[i1 * 4 + 0],
                    s.values[i1 * 4 + 1], s.values[i1 * 4 + 2]);
                R = glm::slerp(q0, q1, alpha);
            }
        }
    }

    glm::mat4 m = glm::translate(glm::mat4(1.0f), T);
    m *= glm::mat4_cast(R);
    m = glm::scale(m, S);
    return m;
}

void GLBModel::updateNodeMatrices() {
    for (size_t i = 0; i < globalMatrices.size(); i++) {
        localMatrices[i] = getNodeLocalMatrix((int)i);
        int parent = parentOf[i];
        globalMatrices[i] = (parent >= 0)
            ? globalMatrices[parent] * localMatrices[i]
            : localMatrices[i];
    }

    // Один скин — skins[0] (все 21 используют один скелет)
    if (!skins.empty()) {
        const GLBSkin& skin = skins[0];
        skinMatrices.resize(skin.jointNodes.size());
        for (size_t i = 0; i < skin.jointNodes.size(); i++) {
            int node = skin.jointNodes[i];
            if (node >= 0 && node < (int)globalMatrices.size()) {
                skinMatrices[i] = globalMatrices[node] * skin.inverseBindMatrices[i];
            }
        }
    }
}
// ============================================================
// ОБНОВЛЕНИЕ АНИМАЦИИ
// ============================================================

void GLBModel::update(float deltaTime) {
    if (animations.empty()) return;
    animTime += deltaTime;
    float duration = animations[currentAnimation].duration;
    if (duration > 0.0f && animTime > duration)
        animTime = fmodf(animTime, duration);
    updateNodeMatrices();
}

void GLBModel::setAnimation(int index) {
    if (index < 0 || index >= (int)animations.size()) return;
    currentAnimation = index;
    animTime = 0.0f;
    updateNodeMatrices();
}
// ============================================================
// РЕНДЕР
// ============================================================

void GLBModel::render(const glm::mat4& mvp) {
    if (!loaded) return;

    shader.use();
    shader.setMat4("uMVP", mvp);

    // Матрицы костей (один скин)
    if (!skinMatrices.empty()) {
        shader.setMat4Array("uBones", skinMatrices.data(),
            (int)skinMatrices.size());
    }

    for (const auto& p : primitives) {
        // Материал
        glm::vec4 color(1.0f);
        bool hasTex = false;

        if (p.materialIndex >= 0 && p.materialIndex < (int)materials.size()) {
            const GLBMaterial& mat = materials[p.materialIndex];
            color = glm::vec4(mat.baseColorFactor[0], mat.baseColorFactor[1],
                mat.baseColorFactor[2], mat.baseColorFactor[3]);
            hasTex = mat.hasTexture && mat.baseColorTexture != nullptr;

            if (hasTex) {
                glActiveTexture(GL_TEXTURE0);
                mat.baseColorTexture->bind();
                shader.setInt("uTexture", 0);
            }
        }

        shader.setVec4("uColor", color);
        shader.setBool("uHasTexture", hasTex);

        glBindVertexArray(p.vao);
        glDrawElements(p.mode, p.indexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        glBindTexture(GL_TEXTURE_2D, 0);
    }

    glUseProgram(0);
}