
#include "GLBModel.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <functional>
#include <limits>
#include <glm/gtx/matrix_decompose.hpp>
// ============================================================
// ACCESSOR HELPERS
// ============================================================

static bool readAccessorFloats(
    const tinygltf::Model& model,
    int accessorIndex,
    int components,
    std::vector<float>& out)
{
    out.clear();

    if (accessorIndex < 0 ||
        accessorIndex >= static_cast<int>(model.accessors.size()))
        return false;

    const auto& acc = model.accessors[accessorIndex];

    if (acc.bufferView < 0 ||
        acc.bufferView >= static_cast<int>(model.bufferViews.size()))
        return false;

    const auto& view = model.bufferViews[acc.bufferView];

    if (view.buffer < 0 ||
        view.buffer >= static_cast<int>(model.buffers.size()))
        return false;

    const auto& buffer = model.buffers[view.buffer];

    const size_t componentSize =
        acc.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT ? 4 :
        acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE ? 1 :
        acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT ? 2 : 0;

    if (componentSize == 0 || acc.type == TINYGLTF_TYPE_SCALAR && components != 1)
        return false;

    const size_t packedSize = componentSize * components;
    const size_t stride = view.byteStride ? view.byteStride : packedSize;
    const size_t offset = view.byteOffset + acc.byteOffset;

    if (stride < packedSize)
        return false;

    if (acc.count > 0) {
        const size_t lastOffset = offset + (acc.count - 1) * stride;
        if (lastOffset > buffer.data.size() ||
            packedSize > buffer.data.size() - lastOffset)
            return false;
    }

    out.resize(acc.count * components);

    for (size_t i = 0; i < acc.count; ++i) {
        const unsigned char* src =
            buffer.data.data() + offset + i * stride;

        for (int c = 0; c < components; ++c) {
            float value = 0.0f;

            if (acc.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) {
                std::memcpy(&value, src + c * 4, sizeof(float));
            }
            else if (acc.componentType ==
                TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
                value = static_cast<float>(src[c]);
            }
            else {
                uint16_t v;
                std::memcpy(&v, src + c * 2, sizeof(v));
                value = static_cast<float>(v);
            }

            out[i * components + c] = value;
        }
    }

    // Sparse accessors are not supported by this helper.
    if (acc.sparse.isSparse)
        return false;

    return true;
}

static bool readIndices(
    const tinygltf::Model& model,
    int accessorIndex,
    std::vector<unsigned int>& out)
{
    out.clear();

    if (accessorIndex < 0 ||
        accessorIndex >= static_cast<int>(model.accessors.size()))
        return false;

    const auto& acc = model.accessors[accessorIndex];

    if (acc.bufferView < 0 ||
        acc.bufferView >= static_cast<int>(model.bufferViews.size()))
        return false;

    const auto& view = model.bufferViews[acc.bufferView];

    if (view.buffer < 0 ||
        view.buffer >= static_cast<int>(model.buffers.size()))
        return false;

    const auto& buffer = model.buffers[view.buffer];

    if (acc.type != TINYGLTF_TYPE_SCALAR || acc.sparse.isSparse)
        return false;

    size_t componentSize = 0;

    switch (acc.componentType) {
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
        componentSize = 1;
        break;
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
        componentSize = 2;
        break;
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
        componentSize = 4;
        break;
    default:
        return false;
    }

    const size_t stride =
        view.byteStride ? view.byteStride : componentSize;

    const size_t offset = view.byteOffset + acc.byteOffset;

    if (stride < componentSize)
        return false;

    if (acc.count > 0) {
        const size_t lastOffset = offset + (acc.count - 1) * stride;

        if (lastOffset > buffer.data.size() ||
            componentSize > buffer.data.size() - lastOffset)
            return false;
    }

    out.resize(acc.count);

    for (size_t i = 0; i < acc.count; ++i) {
        const unsigned char* src =
            buffer.data.data() + offset + i * stride;

        if (componentSize == 1) {
            out[i] = src[0];
        }
        else if (componentSize == 2) {
            uint16_t v;
            std::memcpy(&v, src, sizeof(v));
            out[i] = v;
        }
        else {
            uint32_t v;
            std::memcpy(&v, src, sizeof(v));
            out[i] = v;
        }
    }

    return true;
}

// ============================================================
// LIFETIME
// ============================================================

GLBModel::GLBModel() = default;

GLBModel::~GLBModel()
{
    clear();
    shader.destroy();
}

void GLBModel::clear()
{
    for (auto& p : primitives) {
        if (p.ebo) glDeleteBuffers(1, &p.ebo);
        if (p.vbo) glDeleteBuffers(1, &p.vbo);
        if (p.vao) glDeleteVertexArrays(1, &p.vao);
    }

    primitives.clear();

    for (auto& m : materials)
        delete m.baseColorTexture;

    materials.clear();
    skins.clear();
    animations.clear();

    parentOf.clear();
    baseTRS.clear();
    localMatrices.clear();
    globalMatrices.clear();
    skinMatrices.clear();

    loaded = false;
    animTime = 0.0f;
    currentAnimation = -1;
}

// ============================================================
// LOAD
// ============================================================

bool GLBModel::load(const tinygltf::Model& model)
{
    clear();

    if (!shader.loadFromFiles(
        "resources/skinning.vert",
        "resources/skinning.frag")) {
        std::cerr << "[GLBModel] Failed to load shaders\n";
        return false;
    }

    buildNodeHierarchy(model);
    buildSkins(model);
    buildAnimations(model);

    materials.reserve(model.materials.size());

    for (int i = 0; i < static_cast<int>(model.materials.size()); ++i)
        buildMaterial(model, i);

    // Create primitives per NODE, not merely per mesh.
    // A mesh may be instanced by several nodes with different transforms.
    for (int nodeIndex = 0;
        nodeIndex < static_cast<int>(model.nodes.size());
        ++nodeIndex) {

        const auto& node = model.nodes[nodeIndex];

        if (node.mesh < 0 ||
            node.mesh >= static_cast<int>(model.meshes.size()))
            continue;

        if (node.skin >= static_cast<int>(model.skins.size()) ||
            node.skin < -1)
            continue;

        const auto& mesh = model.meshes[node.mesh];

        for (const auto& prim : mesh.primitives) {
            GLBPrimitive p;

            buildPrimitive(
                model,
                prim,
                prim.material,
                node.skin,
                nodeIndex,
                p
            );

            if (p.vao != 0)
                primitives.push_back(p);
        }
    }

    updateNodeMatrices();

    loaded = !primitives.empty();

    std::cout
        << "[GLBModel] Nodes: " << model.nodes.size()
        << ", meshes: " << model.meshes.size()
        << ", primitives: " << primitives.size()
        << ", materials: " << materials.size()
        << ", animations: " << animations.size()
        << '\n';

    if (!loaded)
        std::cerr << "[GLBModel] No renderable primitives found\n";

    return loaded;
}

// ============================================================
// NODE HIERARCHY
// ============================================================

void GLBModel::buildNodeHierarchy(const tinygltf::Model& model)
{
    const size_t count = model.nodes.size();

    parentOf.assign(count, -1);
    baseTRS.assign(count, NodeTRS{});

    for (size_t i = 0; i < count; ++i) {
        const auto& node = model.nodes[i];

        if (node.translation.size() == 3) {
            baseTRS[i].translation = glm::vec3(
                static_cast<float>(node.translation[0]),
                static_cast<float>(node.translation[1]),
                static_cast<float>(node.translation[2])
            );
        }

        if (node.scale.size() == 3) {
            baseTRS[i].scale = glm::vec3(
                static_cast<float>(node.scale[0]),
                static_cast<float>(node.scale[1]),
                static_cast<float>(node.scale[2])
            );
        }

        if (node.rotation.size() == 4) {
            baseTRS[i].rotation = glm::quat(
                static_cast<float>(node.rotation[3]),
                static_cast<float>(node.rotation[0]),
                static_cast<float>(node.rotation[1]),
                static_cast<float>(node.rotation[2])
            );
        }

        // A glTF node may provide a matrix instead of TRS.
        // Convert the matrix into equivalent TRS values.
        if (node.matrix.size() == 16) {
            glm::mat4 m(1.0f);

            for (int c = 0; c < 4; ++c)
                for (int r = 0; r < 4; ++r)
                    m[c][r] = static_cast<float>(
                        node.matrix[c * 4 + r]);

            glm::vec3 skew;
            glm::vec4 perspective;
            glm::decompose(
                m,
                baseTRS[i].scale,
                baseTRS[i].rotation,
                baseTRS[i].translation,
                skew,
                perspective
            );

            baseTRS[i].rotation =
                glm::normalize(baseTRS[i].rotation);
        }
    }

    for (size_t i = 0; i < count; ++i) {
        for (int child : model.nodes[i].children) {
            if (child < 0 || child >= static_cast<int>(count))
                continue;

            if (parentOf[child] != -1) {
                std::cerr
                    << "[GLBModel] Node has multiple parents: "
                    << child << '\n';
                continue;
            }

            parentOf[child] = static_cast<int>(i);
        }
    }

    localMatrices.assign(count, glm::mat4(1.0f));
    globalMatrices.assign(count, glm::mat4(1.0f));
}

// ============================================================
// SKINS
// ============================================================

void GLBModel::buildSkins(const tinygltf::Model& model)
{
    skins.clear();

    for (const auto& source : model.skins) {
        GLBSkin skin;
        skin.jointNodes = source.joints;

        skin.inverseBindMatrices.assign(
            skin.jointNodes.size(),
            glm::mat4(1.0f)
        );

        if (source.inverseBindMatrices >= 0) {
            std::vector<float> raw;

            if (readAccessorFloats(
                model,
                source.inverseBindMatrices,
                16,
                raw)) {

                const size_t count = std::min(
                    skin.jointNodes.size(),
                    raw.size() / 16
                );

                for (size_t i = 0; i < count; ++i) {
                    glm::mat4 m(1.0f);

                    for (int c = 0; c < 4; ++c)
                        for (int r = 0; r < 4; ++r)
                            m[c][r] = raw[i * 16 + c * 4 + r];

                    skin.inverseBindMatrices[i] = m;
                }
            }
        }

        skins.push_back(std::move(skin));
    }
}

// ============================================================
// ANIMATIONS
// ============================================================

void GLBModel::buildAnimations(const tinygltf::Model& model)
{
    animations.clear();

    for (const auto& source : model.animations) {
        GLBAnimation anim;
        anim.name = source.name;

        for (const auto& sourceSampler : source.samplers) {
            GLBAnimationSampler sampler;
            sampler.interpolation = sourceSampler.interpolation.empty()
                ? "LINEAR"
                : sourceSampler.interpolation;

            if (sourceSampler.input >= 0)
                readAccessorFloats(
                    model, sourceSampler.input, 1, sampler.times);

            if (sourceSampler.output >= 0 &&
                sourceSampler.output <
                static_cast<int>(model.accessors.size())) {

                const auto& acc = model.accessors[sourceSampler.output];

                switch (acc.type) {
                case TINYGLTF_TYPE_SCALAR:
                    sampler.components = 1;
                    break;
                case TINYGLTF_TYPE_VEC3:
                    sampler.components = 3;
                    break;
                case TINYGLTF_TYPE_VEC4:
                    sampler.components = 4;
                    break;
                default:
                    sampler.components = 0;
                    break;
                }

                if (sampler.components > 0)
                    readAccessorFloats(
                        model,
                        sourceSampler.output,
                        sampler.components,
                        sampler.values
                    );
            }

            if (!sampler.times.empty())
                anim.duration = std::max(
                    anim.duration, sampler.times.back());

            anim.samplers.push_back(std::move(sampler));
        }

        for (const auto& sourceChannel : source.channels) {
            GLBAnimationChannel channel;
            channel.targetNode = sourceChannel.target_node;
            channel.path = sourceChannel.target_path;
            channel.samplerIndex = sourceChannel.sampler;

            anim.channels.push_back(std::move(channel));
        }

        animations.push_back(std::move(anim));
    }

    currentAnimation = animations.empty() ? -1 : 0;
    animTime = 0.0f;
}

// ============================================================
// MATERIALS
// ============================================================

void GLBModel::buildMaterial(
    const tinygltf::Model& model,
    int materialIndex)
{
    GLBMaterial mat;

    if (materialIndex < 0 ||
        materialIndex >= static_cast<int>(model.materials.size())) {
        materials.push_back(mat);
        return;
    }

    const auto& source = model.materials[materialIndex];
    const auto& pbr = source.pbrMetallicRoughness;

    if (pbr.baseColorFactor.size() == 4) {
        for (int i = 0; i < 4; ++i)
            mat.baseColorFactor[i] =
            static_cast<float>(pbr.baseColorFactor[i]);
    }

    const int textureIndex = pbr.baseColorTexture.index;

    if (textureIndex >= 0 &&
        textureIndex < static_cast<int>(model.textures.size())) {

        const int imageIndex = model.textures[textureIndex].source;

        if (imageIndex >= 0 &&
            imageIndex < static_cast<int>(model.images.size())) {

            const auto& image = model.images[imageIndex];

            if (!image.image.empty() &&
                image.width > 0 &&
                image.height > 0 &&
                image.component >= 1 &&
                image.component <= 4) {

                GLBTexture* texture = new GLBTexture();

                if (texture->loadFromPixels(
                    image.image.data(),
                    image.width,
                    image.height,
                    image.component)) {

                    mat.baseColorTexture = texture;
                    mat.hasTexture = true;
                }
                else {
                    delete texture;
                }
            }
        }
    }

    materials.push_back(mat);
}

// ============================================================
// PRIMITIVES
// ============================================================

void GLBModel::buildPrimitive(
    const tinygltf::Model& model,
    const tinygltf::Primitive& prim,
    int materialIndex,
    int skinIndex,
    int nodeIndex,
    GLBPrimitive& out)
{
    auto positionIt = prim.attributes.find("POSITION");

    if (positionIt == prim.attributes.end())
        return;

    std::vector<float> positions;

    if (!readAccessorFloats(
        model, positionIt->second, 3, positions))
        return;

    const auto& posAccessor = model.accessors[positionIt->second];
    const size_t vertexCount = posAccessor.count;

    if (positions.size() != vertexCount * 3 || vertexCount == 0)
        return;

    std::vector<float> normals;
    std::vector<float> uvs;
    std::vector<float> joints;
    std::vector<float> weights;

    auto normalIt = prim.attributes.find("NORMAL");
    if (normalIt != prim.attributes.end())
        readAccessorFloats(model, normalIt->second, 3, normals);

    auto uvIt = prim.attributes.find("TEXCOORD_0");
    if (uvIt != prim.attributes.end())
        readAccessorFloats(model, uvIt->second, 2, uvs);

    auto jointsIt = prim.attributes.find("JOINTS_0");
    if (jointsIt != prim.attributes.end())
        readAccessorFloats(model, jointsIt->second, 4, joints);

    auto weightsIt = prim.attributes.find("WEIGHTS_0");
    if (weightsIt != prim.attributes.end())
        readAccessorFloats(model, weightsIt->second, 4, weights);

    std::vector<unsigned int> indices;

    if (prim.indices >= 0) {
        if (!readIndices(model, prim.indices, indices))
            return;
    }
    else {
        indices.resize(vertexCount);
        for (size_t i = 0; i < vertexCount; ++i)
            indices[i] = static_cast<unsigned int>(i);
    }

    if (indices.empty() ||
        indices.size() > static_cast<size_t>(
            std::numeric_limits<GLsizei>::max()))
        return;

    // pos(3), normal(3), uv(2), joints(4), weights(4)
    constexpr int FLOATS_PER_VERTEX = 16;
    std::vector<float> data(vertexCount * FLOATS_PER_VERTEX, 0.0f);

    for (size_t i = 0; i < vertexCount; ++i) {
        for (int c = 0; c < 3; ++c) {
            data[i * 16 + c] = positions[i * 3 + c];

            if (normals.size() >= (i + 1) * 3)
                data[i * 16 + 3 + c] = normals[i * 3 + c];
        }

        if (normals.size() < (i + 1) * 3)
            data[i * 16 + 4] = 1.0f;

        if (uvs.size() >= (i + 1) * 2) {
            data[i * 16 + 6] = uvs[i * 2];
            data[i * 16 + 7] = uvs[i * 2 + 1];
        }

        for (int c = 0; c < 4; ++c) {
            if (joints.size() >= (i + 1) * 4)
                data[i * 16 + 8 + c] = joints[i * 4 + c];

            if (weights.size() >= (i + 1) * 4)
                data[i * 16 + 12 + c] = weights[i * 4 + c];
        }

        // Static meshes need a valid default influence if the vertex
        // shader always performs skinning.
        if (weights.size() < (i + 1) * 4) {
            data[i * 16 + 12] = 1.0f;
        }
    }

    for (unsigned int index : indices) {
        if (index >= vertexCount) {
            std::cerr << "[GLBModel] Invalid mesh index\n";
            return;
        }
    }

    glGenVertexArrays(1, &out.vao);
    glGenBuffers(1, &out.vbo);
    glGenBuffers(1, &out.ebo);

    glBindVertexArray(out.vao);

    glBindBuffer(GL_ARRAY_BUFFER, out.vbo);
    glBufferData(
        GL_ARRAY_BUFFER,
        data.size() * sizeof(float),
        data.data(),
        GL_STATIC_DRAW
    );

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, out.ebo);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        indices.size() * sizeof(unsigned int),
        indices.data(),
        GL_STATIC_DRAW
    );

    const GLsizei stride = FLOATS_PER_VERTEX * sizeof(float);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1, 3, GL_FLOAT, GL_FALSE, stride,
        (void*)(3 * sizeof(float)));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(
        2, 2, GL_FLOAT, GL_FALSE, stride,
        (void*)(6 * sizeof(float)));

    glEnableVertexAttribArray(3);
    glVertexAttribPointer(
        3, 4, GL_FLOAT, GL_FALSE, stride,
        (void*)(8 * sizeof(float)));

    glEnableVertexAttribArray(4);
    glVertexAttribPointer(
        4, 4, GL_FLOAT, GL_FALSE, stride,
        (void*)(12 * sizeof(float)));

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    out.indexCount = static_cast<GLsizei>(indices.size());
    out.materialIndex = materialIndex;
    out.skinIndex = skinIndex;
    out.nodeIndex = nodeIndex;

    switch (prim.mode) {
    case 4: // TRIANGLES
        out.mode = GL_TRIANGLES;
        break;

    case 5: // TRIANGLE_STRIP
        out.mode = GL_TRIANGLE_STRIP;
        break;

    case 6: // TRIANGLE_FAN
        out.mode = GL_TRIANGLE_FAN;
        break;

    case 1: // LINES
        out.mode = GL_LINES;
        break;

    case 2: // LINE_LOOP
        out.mode = GL_LINE_LOOP;
        break;

    case 3: // LINE_STRIP
        out.mode = GL_LINE_STRIP;
        break;

    case 0: // POINTS
        out.mode = GL_POINTS;
        break;

    default:
        std::cerr << "[GLBModel] Unsupported primitive mode: "
            << prim.mode << '\n';
        return;
    }
}

// ============================================================
// NODE MATRICES
// ============================================================

glm::mat4 GLBModel::getNodeLocalMatrix(int nodeIndex)
{
    if (nodeIndex < 0 ||
        nodeIndex >= static_cast<int>(baseTRS.size()))
        return glm::mat4(1.0f);

    glm::vec3 T = baseTRS[nodeIndex].translation;
    glm::quat R = baseTRS[nodeIndex].rotation;
    glm::vec3 S = baseTRS[nodeIndex].scale;

    if (currentAnimation >= 0 &&
        currentAnimation < static_cast<int>(animations.size())) {

        const auto& anim = animations[currentAnimation];

        for (const auto& channel : anim.channels) {
            if (channel.targetNode != nodeIndex)
                continue;

            if (channel.samplerIndex < 0 ||
                channel.samplerIndex >=
                static_cast<int>(anim.samplers.size()))
                continue;

            const auto& sampler = anim.samplers[channel.samplerIndex];

            if (sampler.times.empty() ||
                sampler.components <= 0 ||
                sampler.values.empty())
                continue;

            const size_t keyCount = sampler.times.size();

            if (sampler.values.size() <
                keyCount * sampler.components)
                continue;

            size_t i0 = 0;
            size_t i1 = 0;
            float alpha = 0.0f;

            if (animTime >= sampler.times.back()) {
                i0 = i1 = keyCount - 1;
            }
            else if (animTime > sampler.times.front()) {
                for (size_t k = 0; k + 1 < keyCount; ++k) {
                    if (animTime >= sampler.times[k] &&
                        animTime <= sampler.times[k + 1]) {

                        i0 = k;
                        i1 = k + 1;

                        const float interval =
                            sampler.times[i1] - sampler.times[i0];

                        alpha = interval > 0.0f
                            ? (animTime - sampler.times[i0]) / interval
                            : 0.0f;

                        break;
                    }
                }
            }

            const int components = sampler.components;

            auto valueAt = [&](size_t key, int component) {
                return sampler.values[key * components + component];
                };

            if (channel.path == "translation" && components == 3) {
                glm::vec3 a(
                    valueAt(i0, 0), valueAt(i0, 1), valueAt(i0, 2));
                glm::vec3 b(
                    valueAt(i1, 0), valueAt(i1, 1), valueAt(i1, 2));

                T = glm::mix(a, b, alpha);
            }
            else if (channel.path == "scale" && components == 3) {
                glm::vec3 a(
                    valueAt(i0, 0), valueAt(i0, 1), valueAt(i0, 2));
                glm::vec3 b(
                    valueAt(i1, 0), valueAt(i1, 1), valueAt(i1, 2));

                S = glm::mix(a, b, alpha);
            }
            else if (channel.path == "rotation" && components == 4) {
                glm::quat a(
                    valueAt(i0, 3), valueAt(i0, 0),
                    valueAt(i0, 1), valueAt(i0, 2));

                glm::quat b(
                    valueAt(i1, 3), valueAt(i1, 0),
                    valueAt(i1, 1), valueAt(i1, 2));

                R = glm::normalize(glm::slerp(a, b, alpha));
            }
        }
    }

    glm::mat4 result = glm::translate(glm::mat4(1.0f), T);
    result *= glm::mat4_cast(R);
    result = glm::scale(result, S);

    return result;
}

void GLBModel::updateNodeRecursive(
    int nodeIndex,
    const glm::mat4& parentMatrix,
    std::vector<bool>& visited)
{
    if (nodeIndex < 0 ||
        nodeIndex >= static_cast<int>(globalMatrices.size()) ||
        visited[nodeIndex])
        return;

    visited[nodeIndex] = true;

    localMatrices[nodeIndex] = getNodeLocalMatrix(nodeIndex);
    globalMatrices[nodeIndex] =
        parentMatrix * localMatrices[nodeIndex];
}

void GLBModel::updateNodeMatrices()
{
    const size_t count = baseTRS.size();

    localMatrices.assign(count, glm::mat4(1.0f));
    globalMatrices.assign(count, glm::mat4(1.0f));

    std::vector<bool> visited(count, false);

    std::function<void(int, const glm::mat4&)> visit =
        [&](int node, const glm::mat4& parentMatrix) {
        if (node < 0 ||
            node >= static_cast<int>(count) ||
            visited[node])
            return;

        visited[node] = true;

        localMatrices[node] = getNodeLocalMatrix(node);
        globalMatrices[node] =
            parentMatrix * localMatrices[node];

        // The hierarchy can be traversed by parentOf even if
        // a child has a lower node index than its parent.
        for (int child = 0;
            child < static_cast<int>(parentOf.size());
            ++child) {
            if (parentOf[child] == node)
                visit(child, globalMatrices[node]);
        }
        };

    for (int i = 0; i < static_cast<int>(count); ++i) {
        if (parentOf[i] == -1)
            visit(i, glm::mat4(1.0f));
    }

    // Recover any unvisited nodes in malformed or disconnected graphs.
    for (int i = 0; i < static_cast<int>(count); ++i) {
        if (!visited[i])
            visit(i, glm::mat4(1.0f));
    }

    skinMatrices.clear();

    // The skin matrix array is populated for the active primitive's
    // skin during rendering; this avoids assuming all meshes share
    // the same skeleton.
}

// ============================================================
// UPDATE
// ============================================================

void GLBModel::update(float deltaTime)
{
    if (!std::isfinite(deltaTime) || deltaTime < 0.0f)
        return;

    if (currentAnimation >= 0 &&
        currentAnimation < static_cast<int>(animations.size())) {

        const auto& anim = animations[currentAnimation];

        animTime += deltaTime;

        if (anim.duration > 0.0f) {
            if (loopAnimation) {
                animTime = std::fmod(animTime, anim.duration);
            }
            else {
                animTime = std::min(animTime, anim.duration);
            }
        }
    }

    // Always update matrices, even for a static vehicle.
    updateNodeMatrices();
}

void GLBModel::setAnimation(int index)
{
    if (index < 0 ||
        index >= static_cast<int>(animations.size()))
        return;

    if (currentAnimation == index)
        return;

    currentAnimation = index;
    animTime = 0.0f;

    updateNodeMatrices();
}

// ============================================================
// RENDER
// ============================================================

void GLBModel::render(
    const glm::mat4& mvp,
    const glm::mat4& modelView)
{
    if (!loaded || primitives.empty())
        return;

    shader.use();

    glActiveTexture(GL_TEXTURE0);
    shader.setInt("uTexture", 0);

    shader.setVec4(
        "uFogColor",
        glm::vec4(0.5f, 0.6f, 0.7f, 1.0f)
    );

    for (const auto& p : primitives) {
        glm::mat4 nodeMatrix(1.0f);

        if (p.nodeIndex >= 0 &&
            p.nodeIndex < static_cast<int>(globalMatrices.size())) {
            nodeMatrix = globalMatrices[p.nodeIndex];
        }

        // Include the node transform in the MVP matrix.
        shader.setMat4("uMVP", mvp * nodeMatrix);
        shader.setMat4(
            "uModelView",
            modelView * nodeMatrix
        );

        glm::vec4 color(1.0f);
        bool hasTexture = false;

        if (p.materialIndex >= 0 &&
            p.materialIndex < static_cast<int>(materials.size())) {

            const auto& mat = materials[p.materialIndex];

            color = glm::vec4(
                mat.baseColorFactor[0],
                mat.baseColorFactor[1],
                mat.baseColorFactor[2],
                mat.baseColorFactor[3]
            );

            hasTexture =
                mat.hasTexture && mat.baseColorTexture != nullptr;

            if (hasTexture)
                mat.baseColorTexture->bind();
        }

        shader.setVec4("uColor", color);
        shader.setBool("uHasTexture", hasTexture);

        if (p.skinIndex >= 0 &&
            p.skinIndex < static_cast<int>(skins.size())) {

            const auto& skin = skins[p.skinIndex];

            std::vector<glm::mat4> bones(
                skin.jointNodes.size(),
                glm::mat4(1.0f)
            );

            for (size_t i = 0; i < skin.jointNodes.size(); ++i) {
                const int joint = skin.jointNodes[i];

                if (joint >= 0 &&
                    joint < static_cast<int>(globalMatrices.size())) {

                    bones[i] = globalMatrices[joint] *
                        skin.inverseBindMatrices[i];
                }
            }

            if (!bones.empty()) {
                shader.setMat4Array(
                    "uBones",
                    bones.data(),
                    static_cast<int>(bones.size())
                );
            }
        }
        else {
            // A static primitive must not inherit a previous mesh's
            // bone transforms. The vertex shader must respect weights.
            const glm::mat4 identity(1.0f);
            shader.setMat4Array("uBones", &identity, 1);
        }

        glBindVertexArray(p.vao);

        glDrawElements(
            p.mode,
            p.indexCount,
            GL_UNSIGNED_INT,
            nullptr
        );

        glBindVertexArray(0);

        glBindTexture(GL_TEXTURE_2D, 0);
    }

    glUseProgram(0);
}
void GLBModel::render(const glm::mat4& mvp)
{
    render(mvp, glm::mat4(1.0f));
}