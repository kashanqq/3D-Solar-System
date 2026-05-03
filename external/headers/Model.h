#pragma once
#include <iostream>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <Mesh.h>
#include <Shader.h>
#include <stb_image.h>


class Model {
public:

	Model(std::string const& path) {
		loadModel(path);
	}

	void Draw(Shader& shader) {
		for (unsigned int j = 0; j < meshes.size(); j++) {
			meshes[j].Draw(shader);
		}
	}
private:
	vector<Mesh> meshes;
	string directory;

	void loadModel(string path) {
		Assimp::Importer importer;

		const aiScene* scene = importer.ReadFile(
			path.c_str(),
			aiProcess_Triangulate |
			aiProcess_CalcTangentSpace |
			aiProcess_GenNormals |
			aiProcess_MakeLeftHanded |
			aiProcess_PreTransformVertices
		);

		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
		std::cerr << "Error: " << importer.GetErrorString() << std::endl;
		}

		directory = path.substr(0, path.find_last_of('/'));
		processNode(scene->mRootNode, scene);
	}

	void processNode(aiNode* node, const aiScene* scene) {
		for (unsigned int i = 0; i < node->mNumMeshes; i++) {
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];

			aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
			aiString matName;
			material->Get(AI_MATKEY_NAME, matName);
			std::string materialName = matName.C_Str();
			std::string meshName = mesh->mName.C_Str();

			if (materialName.find("light") != std::string::npos || meshName.find("Light") != std::string::npos) {
				std::cout << "Skipped light source: " << meshName << std::endl;
				continue;
			}

			meshes.push_back(processMesh(mesh, scene));
		}

		for (unsigned int i = 0; i < node->mNumChildren; i++) {
			processNode(node->mChildren[i], scene);
		}
	}

	Mesh processMesh(aiMesh *mesh, const aiScene* scene) {
		vector<Vertex> vertices;
		vector<unsigned int> indices;
		vector<Texture> textures;

		for (unsigned int v = 0; v < mesh->mNumVertices; v++) {
			Vertex vertex;

			vertex.Position = glm::vec3(mesh->mVertices[v].x, mesh->mVertices[v].y, mesh->mVertices[v].z);


			if (mesh->HasNormals()) {
				vertex.Normal = glm::vec3(mesh->mNormals[v].x, mesh->mNormals[v].y, mesh->mNormals[v].z);
			}


			if (mesh->mTextureCoords[0]) {
				vertex.TexCoords = glm::vec2(mesh->mTextureCoords[0][v].x, mesh->mTextureCoords[0][v].y);
			}
			else {
				vertex.TexCoords = glm::vec2(0.0f, 0.0f);
			}

			if (mesh->HasTangentsAndBitangents()) {
				vertex.Tangent = glm::vec3(mesh->mTangents[v].x, mesh->mTangents[v].y, mesh->mTangents[v].z);
			}
			else {
				vertex.Tangent = glm::vec3(0.0f);
			}

			vertices.push_back(vertex);
		}

		for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
			aiFace face = mesh->mFaces[i];
			for (unsigned int j = 0; j < face.mNumIndices; j++)
				indices.push_back(face.mIndices[j]);
		}


		aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

		aiString matName;
		material->Get(AI_MATKEY_NAME, matName);
		std::cout << "\n=== Material scaner: " << matName.C_Str() << " ===" << std::endl;
		std::cout << "DIFFUSE textures: " << material->GetTextureCount(aiTextureType_DIFFUSE) << std::endl;
		std::cout << "BASE_COLOR textures: " << material->GetTextureCount(aiTextureType_BASE_COLOR) << std::endl;
		std::cout << "NORMALS textures: " << material->GetTextureCount(aiTextureType_NORMALS) << std::endl;
		std::cout << "HEIGHT textures: " << material->GetTextureCount(aiTextureType_HEIGHT) << std::endl;
		std::cout << "UNKNOWN textures: " << material->GetTextureCount(aiTextureType_UNKNOWN) << std::endl;
		std::cout << "=======================================" << std::endl;


		vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse", scene);
		textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

		if (diffuseMaps.empty())
		{
			vector<Texture> baseMaps = loadMaterialTextures(material, aiTextureType_BASE_COLOR, "texture_diffuse", scene);
			textures.insert(textures.end(), baseMaps.begin(), baseMaps.end());
		}

		vector<Texture> normalMaps = loadMaterialTextures(material, aiTextureType_NORMALS, "texture_normal", scene);
		textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());

		if (normalMaps.empty())
		{
			vector<Texture> normalMaps2 = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal", scene);
			textures.insert(textures.end(), normalMaps2.begin(), normalMaps2.end());
		}

		vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular", scene);
		textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());

		return Mesh(vertices, indices, textures);
	}

	unsigned int TextureFromFile(const char* path, const string& directory)
	{
		string filename = string(path);
		filename = directory + '/' + filename;

		unsigned int textureID;
		glGenTextures(1, &textureID);

		int width, height, nrComponents;

		stbi_set_flip_vertically_on_load(true);
		unsigned char* data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);

		if (data)
		{
			GLenum format;
			if (nrComponents == 1) format = GL_RED;
			else if (nrComponents == 3) format = GL_RGB;
			else if (nrComponents == 4) format = GL_RGBA;

			glBindTexture(GL_TEXTURE_2D, textureID);
			glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
			glGenerateMipmap(GL_TEXTURE_2D);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			stbi_image_free(data);
		}
		else
		{
			std::cout << "Texture failed to load at path: " << path << std::endl;
			stbi_image_free(data);
		}

		return textureID;
	}

	vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, string typeName, const aiScene* scene)
	{
		vector<Texture> textures;
		for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
		{
			aiString str;
			mat->GetTexture(type, i, &str);

			std::cout << "Trying to load texture: " << str.C_Str() << " (Type: " << typeName << ")" << std::endl;

			Texture texture;
			texture.type = typeName;
			texture.path = str.C_Str();

			// Check, if FBX already have texture
			const aiTexture* embeddedTexture = scene->GetEmbeddedTexture(str.C_Str());

			if (embeddedTexture) {
				// Download from RAM
				std::cout << "SUCCESS: Texture found in memory!" << std::endl;
				texture.id = TextureFromMemory(embeddedTexture);
			} else {
				std::cout << "WARNING: Not in memory, trying to find on disk..." << std::endl;
				texture.id = TextureFromFile(str.C_Str(), this->directory);
			}

			textures.push_back(texture);
		}
		return textures;
	}

	unsigned int TextureFromMemory(const aiTexture* texture)
	{
		unsigned int textureID;
		glGenTextures(1, &textureID);

		int width, height, nrComponents;
		unsigned char* data = nullptr;

		if (texture->mHeight == 0) {
			data = stbi_load_from_memory(reinterpret_cast<unsigned char*>(texture->pcData), texture->mWidth, &width, &height, &nrComponents, 0);
		}
		else {
			data = stbi_load_from_memory(reinterpret_cast<unsigned char*>(texture->pcData), texture->mWidth * texture->mHeight * 4, &width, &height, &nrComponents, 0);
		}

		if (data)
		{
			GLenum format;
			if (nrComponents == 1) format = GL_RED;
			else if (nrComponents == 3) format = GL_RGB;
			else if (nrComponents == 4) format = GL_RGBA;

			glBindTexture(GL_TEXTURE_2D, textureID);
			glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
			glGenerateMipmap(GL_TEXTURE_2D);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			stbi_image_free(data);
		}
		else
		{
			std::cout << "Failed to load embedded texture!" << std::endl;
		}

		return textureID;
	}
	
};