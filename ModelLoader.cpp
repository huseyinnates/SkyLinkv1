#include "ModelLoader.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <iostream>

namespace SkyLink {
    namespace Model {

        ModelLoader::ModelLoader() {}

        ModelLoader::~ModelLoader() {}

        bool ModelLoader::loadModel(const std::string& path, Mesh& mesh) {
            Assimp::Importer importer;
            const aiScene* scene = importer.ReadFile(path,
                aiProcess_Triangulate | aiProcess_FlipUVs);

            if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE ||
                !scene->mRootNode) {
                std::cerr << "ERROR::ASSIMP::"
                    << importer.GetErrorString() << std::endl;
                return false;
            }

            aiMesh* ai_mesh = scene->mMeshes[0]; // Tek bir mesh varsayıyoruz
            for (unsigned int i = 0; i < ai_mesh->mNumVertices; i++) {
                // Vertex pozisyonları
                mesh.vertices.push_back(ai_mesh->mVertices[i].x);
                mesh.vertices.push_back(ai_mesh->mVertices[i].y);
                mesh.vertices.push_back(ai_mesh->mVertices[i].z);
                // Normaller
                mesh.vertices.push_back(ai_mesh->mNormals[i].x);
                mesh.vertices.push_back(ai_mesh->mNormals[i].y);
                mesh.vertices.push_back(ai_mesh->mNormals[i].z);
            }

            for (unsigned int i = 0; i < ai_mesh->mNumFaces; i++) {
                aiFace face = ai_mesh->mFaces[i];
                for (unsigned int j = 0; j < face.mNumIndices; j++)
                    mesh.indices.push_back(face.mIndices[j]);
            }

            // Malzeme
            if (scene->mMaterials[ai_mesh->mMaterialIndex]) {
                aiMaterial* material = scene->mMaterials[ai_mesh->mMaterialIndex];
                aiColor3D color(0.f, 0.f, 0.f);
                material->Get(AI_MATKEY_COLOR_DIFFUSE, color);
                mesh.diffuseColor = glm::vec3(color.r, color.g, color.b);
            }
            else {
                mesh.diffuseColor = glm::vec3(1.0f, 0.5f, 0.31f); // Varsayılan renk
            }

            mesh.setupMesh();

            return true;
        }

    } // namespace Model
} // namespace SkyLink
