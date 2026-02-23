/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#include <Model.h>
#include <Buffer.h>
#include <fraze/common/Exception.h>
#include <fraze/memory/ScopedAllocator.h>
#include <fraze/program/Program.h>
#include <fraze/program/Dispatcher.h>
#include <WorkerThread.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <future>
#include <print>

namespace fraze {

struct NativeVertex
{
    float x, y, z;
    float u, v;
    float nx, ny, nz;
};

Class* BuildNode(const aiScene* scene, IAllocator& allocator, const std::vector<Class*>& allMeshes, aiNode* pNode)
{
    aiVector3D localScale;
    aiQuaternion localRotation;
    aiVector3D localPosition;
    pNode->mTransformation.Decompose(localScale, localRotation, localPosition);

    Transform transform;
    transform.pos.x = localPosition.x;
    transform.pos.y = localPosition.y;
    transform.pos.z = localPosition.z;
    transform.rot.v.x = localRotation.x;
    transform.rot.v.y = localRotation.y;
    transform.rot.v.z = localRotation.z;
    transform.rot.w   = localRotation.w;
    transform.scale.x = localScale.x;
    transform.scale.y = localScale.y;
    transform.scale.z = localScale.z;

    Class* node = NEW_FRAZE_CLASS(allocator, "ModelNode");
    node->SetField("name", NEW_FRAZE_STRING(allocator, pNode->mName.C_Str()));
    node->SetField("transform", transform);

    Array<>* meshes = NEW_FRAZE_ARRAY(allocator, "Mesh[]", pNode->mNumMeshes);

    for(uint32_t i = 0; i != pNode->mNumMeshes; ++i)
    {
        uint32_t meshIndex = pNode->mMeshes[i];
        meshes->At(i) = allMeshes[meshIndex];
    }

    node->SetField("meshes", meshes);

    Array<>* children = NEW_FRAZE_ARRAY(allocator, "ModelNode[]", pNode->mNumChildren);

    for(uint32_t i = 0; i != pNode->mNumChildren; ++i)
    {
        aiNode* pChild = pNode->mChildren[i];
        Class* childNode = BuildNode(scene, allocator, allMeshes, pChild);
        children->At(i) = childNode;
    }

    node->SetField("children", children);

    return node;
}

Mat4 FromAIMat4x4(const aiMatrix4x4& mtx)
{
    auto m = aiMatrix4x4(mtx).Transpose();

    return {
        m.a1, m.a2, m.a3, m.a4,
        m.b1, m.b2, m.b3, m.b4,
        m.c1, m.c2, m.c3, m.c4,
        m.d1, m.d2, m.d3, m.d4,
    };
}

Vec3 FromAIVector3D(const aiVector3D& vec)
{
    return Vec3{ vec.x, vec.y, vec.z };
}

Quat FromAIQuaternion(const aiQuaternion& quat)
{
    return Quat{ { quat.x, quat.y, quat.z }, quat.w };
}

Mat4 GetGlobalTransform(const aiNode* node)
{
    aiMatrix4x4 transform = node->mTransformation;
    while(node->mParent) {
        node = node->mParent;
        transform = node->mTransformation * transform;
    }
    return FromAIMat4x4(transform);
}

aiNode* FindNodeForMesh(aiNode* node, unsigned int meshIndex)
{
    for(uint32_t i = 0; i < node->mNumMeshes; ++i)
    {
        if (node->mMeshes[i] == meshIndex)
            return node;
    }

    for(uint32_t i = 0; i < node->mNumChildren; ++i)
    {
        aiNode* result = FindNodeForMesh(node->mChildren[i], meshIndex);
        if(result)
            return result;
    }

    return nullptr;
}

Class* ModelImporter::ImportModel(IAllocator& allocator, Graphics* graphics, const std::string& path)
{
    Assimp::Importer importer;
    importer.SetPropertyInteger(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, 0);

    const aiScene* scene = importer.ReadFile(path,
        aiProcess_Triangulate
        | aiProcess_FlipUVs
        | aiProcess_LimitBoneWeights
        //| aiProcess_MakeLeftHanded
        //| aiProcess_FlipWindingOrder
    );

    if (!scene || !scene->HasMeshes())
        Throw("Failed to load mesh: {}", importer.GetErrorString());

    std::vector<NativeVertex> vertices;
    std::vector<uint32_t> indices;
    
    std::vector<Class*> allMeshes;
    allMeshes.reserve(scene->mNumMeshes);

    for(uint32_t m = 0; m != scene->mNumMeshes; ++m)
    {
        // extract data
        aiMesh* pMesh = scene->mMeshes[m];
        
        vertices.clear();
        vertices.reserve(pMesh->mNumVertices);
        
        for(uint32_t i = 0; i < pMesh->mNumVertices; ++i)
        {
            NativeVertex v;
            v.x = pMesh->mVertices[i].x;
            v.y = pMesh->mVertices[i].y;
            v.z = pMesh->mVertices[i].z;

            if (pMesh->HasTextureCoords(0)) {
                v.u = pMesh->mTextureCoords[0][i].x;
                v.v = pMesh->mTextureCoords[0][i].y;
            } else {
                v.u = 0.0f;
                v.v = 0.0f;
            }

            if(pMesh->HasNormals()) {
                v.nx = pMesh->mNormals[i].x;
                v.ny = pMesh->mNormals[i].y;
                v.nz = pMesh->mNormals[i].z;
            }
            else {
                v.nx = 0.0f;
                v.ny = 0.0f;
                v.nz = 0.0f;
            }

            vertices.push_back(v);
        }

        indices.clear();
        indices.reserve(pMesh->mNumFaces * 3);
        
        for(uint32_t f = 0; f < pMesh->mNumFaces; ++f)
        {
            const aiFace& face = pMesh->mFaces[f];

            for(int i = 0; i != face.mNumIndices; ++i)
            {
                indices.push_back(face.mIndices[i]);
            }
        }

        Class* vertexBuffer = NEW_FRAZE_CLASS(allocator, "Buffer");
        Object* nativeVertexBuffer = NEW_FRAZE_EXTERN_CLASS(allocator, Buffer, "NativeBuffer",
            graphics,
            BufferType::Vertex,
            BufferUsage::Dynamic,
            BufferCPUAccess::WriteOnly,
            vertices.data(),
            vertices.size() * sizeof(NativeVertex),
            sizeof(NativeVertex)
        );
        vertexBuffer->SetField("nativeBuffer", nativeVertexBuffer);

        Class* indexBuffer = NEW_FRAZE_CLASS(allocator, "Buffer");
        Object* nativeIndexBuffer = NEW_FRAZE_EXTERN_CLASS(allocator, Buffer, "NativeBuffer",
            graphics,
            BufferType::Index,
            BufferUsage::Static,
            BufferCPUAccess::None,
            indices.data(),
            indices.size() * sizeof(uint32_t),
            0
        );
        indexBuffer->SetField("nativeBuffer", nativeIndexBuffer);

        // one element per vertex
        // each element contains set of index/weight pairs
        std::vector<std::vector<std::pair<uint32_t, float>>> vertWeightSets;
        vertWeightSets.resize(vertices.size());
        
        Array<Bone>* bones = NEW_FRAZE_ARRAY_T(allocator, Bone, "Bone[]", pMesh->mNumBones);

        for(uint32_t b = 0; b < pMesh->mNumBones; ++b)
        {
            aiBone* pBone = pMesh->mBones[b];
            
            for(uint32_t w = 0; w != pBone->mNumWeights; ++w)
            {
                aiVertexWeight weight = pBone->mWeights[w];
                vertWeightSets[weight.mVertexId].push_back(std::make_pair(b, weight.mWeight));
            }
            
            const aiNode* pNode = scene->mRootNode->findBoneNode(pBone);
            String* linkNodeName = NEW_FRAZE_STRING(allocator, pNode->mName.C_Str());
            
            const aiNode* pMeshNode = FindNodeForMesh(scene->mRootNode, m);
            Mat4 meshBindMatrix = GetGlobalTransform(pMeshNode);
            
            Mat4 invBoneBindMatrix = FromAIMat4x4(pBone->mOffsetMatrix);
            
            Class* bone = NEW_FRAZE_CLASS(allocator, "Bone");
            bone->SetField("linkNodeName", linkNodeName);
            bone->SetField("meshBindMatrix", meshBindMatrix);
            bone->SetField("invBoneBindMatrix", invBoneBindMatrix);
            bones->At(b) = Word(bone);
        }

        // sort all vertex sets in descending order of weight so top 4 can be chosen
        for (auto& weightSet : vertWeightSets)
        {
            std::sort(weightSet.begin(), weightSet.end(),
                [](std::pair<uint32_t, float>& x, std::pair<uint32_t, float>& y) {
                    return x.second > y.second;
                });
        }

        Array<IVec4>* boneIndices = NEW_FRAZE_ARRAY_T(allocator, IVec4, "IVec4[]", vertices.size()); // 4 indices per vertex
        Array<Vec4>* boneWeights = NEW_FRAZE_ARRAY_T(allocator, Vec4, "Vec4[]", vertices.size()); // 4 weights per vertex

        constexpr size_t MaxBones = 4;
        
        // normalize and store the bone links.
        for(int v = 0; v < vertices.size(); ++v)
        {
            auto& weightSet = vertWeightSets[v];

            // attach unbound vertices to bone 0.
            if(weightSet.size() == 0)
            {
                weightSet.push_back(std::pair<uint32_t, float>(0, 1.0f));
            }

            // fill up extra bone slots with zero weights
            while(weightSet.size() < MaxBones)
            {
                weightSet.push_back(std::pair<uint32_t, float>(0, 0.0f));
            }

            // normalize bone weights
            float totalWeight = 0.0f;

            for(uint32_t i = 0; i < MaxBones; ++i)
                totalWeight += weightSet[i].second;

            if(totalWeight > 0.0f)
            {
                for (uint32_t i = 0; i < MaxBones; ++i)
                    weightSet[i].second /= totalWeight;
            }

            (*boneIndices)[v] = IVec4{ weightSet[0].first, weightSet[1].first, weightSet[2].first, weightSet[3].first };
            (*boneWeights)[v] = Vec4{ weightSet[0].second, weightSet[1].second, weightSet[2].second, weightSet[3].second };
        }

        Array<Vertex>* vertexArray = NEW_FRAZE_ARRAY_T(allocator, Vertex, "Vertex[]", vertices.size());
        
        for(size_t i = 0; i != vertices.size(); ++i)
        {
            NativeVertex& v1 = vertices[i];

            (*vertexArray)[i] = Vertex{
                v1.x, v1.y, v1.z,
                v1.u, v1.v,
                v1.nx, v1.ny, v1.nz,
            };
        }

        Class* mesh = NEW_FRAZE_CLASS(allocator, "Mesh");
        mesh->SetField("vertices", vertexArray);
        mesh->SetField("vertexBuffer", vertexBuffer);
        mesh->SetField("indexBuffer", indexBuffer);
        mesh->SetField("drawMode", static_cast<Integer>(DrawMode::Triangles));
        mesh->SetField("bones", bones);
        mesh->SetField("boneIndices", boneIndices);
        mesh->SetField("boneWeights", boneWeights);

        allMeshes.push_back(mesh);
    }
    
    Array<AnimationClip>* clips = NEW_FRAZE_ARRAY_T(allocator, AnimationClip, "AnimationClip[]", scene->mNumAnimations);

    for(int i = 0; i != scene->mNumAnimations; ++i)
    {
        aiAnimation* anim = scene->mAnimations[i];

        String* name = NEW_FRAZE_STRING(allocator, anim->mName.C_Str());
        Number length = anim->mDuration / anim->mTicksPerSecond;

        Array<AnimationTrack>* tracks = NEW_FRAZE_ARRAY_T(allocator, AnimationTrack, "AnimationTrack[]", anim->mNumChannels);

        for(int c = 0; c != anim->mNumChannels; ++c)
        {
            aiNodeAnim* channel = anim->mChannels[c];
            assert(channel->mNumPositionKeys >= 1);
            assert(channel->mNumRotationKeys >= 1);
            assert(channel->mNumScalingKeys >= 1);

            String* nodeName = NEW_FRAZE_STRING(allocator, channel->mNodeName.C_Str());

            uint32_t positionKeyCount = channel->mNumPositionKeys;
            uint32_t rotationKeyCount = channel->mNumRotationKeys;
            uint32_t scaleKeyCount = channel->mNumScalingKeys;
            uint32_t keyCount = std::max(std::max(positionKeyCount, rotationKeyCount), scaleKeyCount);

            Array<Keyframe>* frames = NEW_FRAZE_ARRAY_T(allocator, Keyframe, "Keyframe[]", keyCount);

            for(uint32_t k = 0; k != keyCount; ++k)
            {
                Keyframe keyframe;

                keyframe.time = channel->mPositionKeys[std::min(k, positionKeyCount - 1)].mTime / anim->mTicksPerSecond;
                keyframe.value.pos = FromAIVector3D(channel->mPositionKeys[std::min(k, positionKeyCount - 1)].mValue);
                keyframe.value.rot = FromAIQuaternion(channel->mRotationKeys[std::min(k, rotationKeyCount - 1)].mValue);
                keyframe.value.scale = FromAIVector3D(channel->mScalingKeys[std::min(k, scaleKeyCount - 1)].mValue);

                (*frames)[k] = keyframe;
            }
            
            Class* track = NEW_FRAZE_CLASS(allocator, "AnimationTrack");
            assert(nodeName);
            track->SetField("nodeName", nodeName);
            track->SetField("frames", frames);

            tracks->At(c) = track;
        }

        Class* clip = NEW_FRAZE_CLASS(allocator, "AnimationClip");
        clip->SetField("name", name);
        clip->SetField("length", length);
        clip->SetField("tracks", tracks);
        clips->At(i) = Word(clip);
    }

    Class* rootNode = BuildNode(scene, allocator, allMeshes, scene->mRootNode);
    
    Class* model = NEW_FRAZE_CLASS(allocator, "Model");
    model->SetField("rootNode", rootNode);
    model->SetField("clips", clips);

    return model;
}

void ModelImporter::ImportModelAsync(Program* program, Class& task, Graphics* graphics, const std::string& path)
{
    Class* taskPtr = &task;
    program->PinMemory(taskPtr);

    sptr<Dispatcher> dispatcher = Dispatcher::GetCurrent();

    WorkerThread::GetInstance().InvokeAsync([=](){
        sptr<ScopedAllocator> allocator = spnew<ScopedAllocator>(program);
        Class* model = ModelImporter::ImportModel(*allocator, graphics, path);

        dispatcher->InvokeAsync([=, allocator=allocator]{
            taskPtr->SetField("$position", Integer(-1));
            taskPtr->SetField("$value", model);
            program->Invoke("OnAwaitableCompleted", taskPtr);
            program->UnpinMemory(taskPtr);
        });
    });
}

constexpr float Pi = 3.141592654f;
constexpr float DegToRad = Pi / 180.0f;

Class* ModelImporter::CreateSphereMesh(Program* program, Graphics* graphics, Number radius, Integer segments, Integer rings, bool invert)
{
    ScopedAllocator allocator(program);

    Integer vertexCount = (rings + 1) * (segments + 1);
    Integer indexCount = rings * segments * 6;

    Array<Vertex>* vertices = NEW_FRAZE_ARRAY_T(allocator, Vertex, "Vertex[]", vertexCount);
    Array<Integer>* indices = NEW_FRAZE_ARRAY_T(allocator, Integer, "int[]", indexCount);

    size_t vert = 0;

    for(Integer i = 0; i <= rings; ++i)
    {
        Number v = Number(i) / rings;
        Number theta = v * Pi;

        Number sinTheta = sin(theta);
        Number cosTheta = cos(theta);

        for(Integer j = 0; j <= segments; ++j)
        {
            Number u = Number(j) / segments;
            Number phi = u * 2.0 * Pi;

            Number sinPhi = sin(phi);
            Number cosPhi = cos(phi);

            Vec3 pos = {
                radius * sinTheta * cosPhi,
                radius * cosTheta,
                radius * sinTheta * sinPhi
            };

            Vec3 norm = {
                sinTheta * cosPhi,
                cosTheta,
                sinTheta * sinPhi
            };

            Vec2 tex = { u, v };

            vertices->GetElement(vert++) = { pos, tex, norm };
        }
    }

    size_t index = 0;

    for(Integer i = 0; i < rings; ++i)
    {
        for(Integer j = 0; j < segments; ++j)
        {
            Integer row1 = i * (segments + 1);
            Integer row2 = (i + 1) * (segments + 1);

            Integer a = row2 + j + 1;
            Integer b = row2 + j;
            Integer c = row1 + j;
            Integer d = row1 + j + 1;

            if(invert)
            {
                indices->GetElement(index++) = c;
                indices->GetElement(index++) = b;
                indices->GetElement(index++) = a;

                indices->GetElement(index++) = c;
                indices->GetElement(index++) = a;
                indices->GetElement(index++) = d;
            }
            else
            {
                indices->GetElement(index++) = a;
                indices->GetElement(index++) = b;
                indices->GetElement(index++) = c;

                indices->GetElement(index++) = d;
                indices->GetElement(index++) = a;
                indices->GetElement(index++) = c;
            }
        }
    }

    std::vector<NativeVertex> nativeVertices;
    std::vector<uint32_t> nativeIndices;

    nativeVertices.reserve(vertexCount);
    nativeIndices.reserve(indexCount);

    for(size_t i = 0; i != vertexCount; ++i)
    {
        Vertex& vertex = vertices->GetElement(i);

        nativeVertices.push_back(NativeVertex{
            (float)vertex.pos.x, (float)vertex.pos.y, (float)vertex.pos.z,
            (float)vertex.tex.x, (float)vertex.tex.y,
            (float)vertex.norm.x, (float)vertex.norm.y, (float)vertex.norm.z,
        });
    }

    for(size_t i = 0; i != indexCount; ++i)
    {
        Integer index = indices->GetElement(i);
        nativeIndices.push_back((uint32_t)index);
    }

    Class* vertexBuffer = NEW_FRAZE_CLASS(allocator, "Buffer");
    Object* nativeVertexBuffer = NEW_FRAZE_EXTERN_CLASS(allocator, Buffer, "NativeBuffer",
        graphics,
        BufferType::Vertex,
        BufferUsage::Static,
        BufferCPUAccess::None,
        nativeVertices.data(),
        nativeVertices.size() * sizeof(NativeVertex),
        sizeof(NativeVertex)
    );
    vertexBuffer->SetField("nativeBuffer", nativeVertexBuffer);

    Class* indexBuffer = NEW_FRAZE_CLASS(allocator, "Buffer");
    Object* nativeIndexBuffer = NEW_FRAZE_EXTERN_CLASS(allocator, Buffer, "NativeBuffer",
        graphics,
        BufferType::Index,
        BufferUsage::Static,
        BufferCPUAccess::None,
        nativeIndices.data(),
        nativeIndices.size() * sizeof(uint32_t),
        0
    );
    indexBuffer->SetField("nativeBuffer", nativeIndexBuffer);

    Class* mesh = NEW_FRAZE_CLASS(allocator, "Mesh");
    mesh->SetField("vertices", vertices);
    mesh->SetField("vertexBuffer", vertexBuffer);
    mesh->SetField("indexBuffer", indexBuffer);
    mesh->SetField("drawMode", static_cast<Integer>(DrawMode::Triangles));

    return mesh;
}

} // fraze
