#pragma once
#include <vector>
#include <array>
#include <set>
#include "bgfx_utils.h"
using std::vector;
using std::array;
using std::set;

class RenderItem
{
	friend RenderItem BuildAvaRItem();
	friend RenderItem BuildPianoRItem();
	friend RenderItem BuildLawnRItem();
	friend RenderItem BuildPBRStoneRItem();
	friend RenderItem BuildCampfireRItem();
	friend void BuildTestSpheres(map<string, RenderItem>&);
private:
	bool						m_doShown = true;
	// Note: loading mesh using 'meshLoad' will genertate different groups
	//	defined in obj file, thus 'usemtl' and the mtl file don't work.
	//	So, the textureUse should be set manually.
	Mesh*						m_meshPointer;
	// Textures of color, normal (if any), aorm (if any).
	vector<bgfx::TextureHandle>	m_textures;
	// If don't use textures of color/AORM, provide some built-in parameters.
	vector<array<float, 4>>		m_builtinColor;
	vector<array<float, 4>>		m_builtinAORM;
	// Each '*Use' member is a vector, indicating 
	//  which texture (or built-in parameters) does each group use.
	// You should set them for each group.
	// If only 1 index is set, it means all groups use this.
	// Prefer tex*Idx when tex*Idx and builtin*Idx are both set.
	vector<int>					m_texColorIdx;
	vector<int>					m_texNormalIdx;
	vector<int>					m_texAORMIdx;
	vector<int>					m_builtinColorIdx;
	vector<int>					m_builtinAORMIdx;
	// Model matrix.
	array<float, 16>			m_modelMatrix;
	// If none instance matrices are set, just render 1 instance.
	// Instance matrix is used before model matrix.
	vector<array<float, 16>>	m_instanceMatrices;
	vector<array<float, 16>>	m_instxModel;
	// Indicate which groups are transparent.
	// Use std::set to search faster.
	set<int>					m_transparentGroups;
	// Whether use { texColor, texNormal, texAORM, cubeMap }
	float						m_texOrBuiltin[4];
	inline void mulInstModel()
	{
		if (m_instanceMatrices.empty()) {
			m_instxModel.push_back(m_modelMatrix);
		}
		else {
			for (size_t i = 0; i < m_instanceMatrices.size(); ++i) {
				array<float, 16> temp;
				bx::mtxMul(temp.data(),
					m_instanceMatrices[i].data(), m_modelMatrix.data());
				m_instxModel.push_back(temp);
			}
		}
	}
public:
	void show(bool p_show = true) { m_doShown = p_show; }
	void render(
		bgfx::ProgramHandle&	p_program, 
		int						p_passid,
		bgfx::UniformHandle&	p_s_colorSampler,
		bgfx::UniformHandle&	p_s_normalSampler,
		bgfx::UniformHandle&	p_s_aormSampler,
		bgfx::UniformHandle&	p_s_shadowSampler,
		bgfx::UniformHandle&	p_u_builtinColor,
		bgfx::UniformHandle&	p_u_builtinAORM,
		bgfx::UniformHandle&	p_u_texOrBuiltin,
		bgfx::TextureHandle&	p_shadowMap,
		bool					draw_transparent=false
	) {
		if (!m_doShown) { return; }
		mulInstModel();
		int nInstance = m_instanceMatrices.empty() ? 1 : (int)m_instanceMatrices.size();
		if (nInstance == bgfx::getAvailInstanceDataBuffer(nInstance, 64)) {
			auto groups = m_meshPointer->m_groups;
			for (int g_idx = 0; g_idx < groups.size(); ++g_idx) {
				bool is_transparent = m_transparentGroups.find(g_idx) != m_transparentGroups.end();
				if (draw_transparent != is_transparent) {
					continue;
				}
				bgfx::InstanceDataBuffer idb;
				// For each instance, transfer 64 bytes of an instance matrix,
				//  and 64 bytes of a normal matrix.
				bgfx::allocInstanceDataBuffer(&idb, nInstance, 64);
				uint8_t* data = idb.data;
				for (int ii = 0; ii < nInstance; ++ii) {
					for (int mat_i = 0; mat_i < 16; ++mat_i) {
						*(((float*)data) + mat_i) = m_instxModel[ii][mat_i];
					}
					data += 64;
				}
				bgfx::setInstanceDataBuffer(&idb, 0, nInstance);
				// bgfx::setTransform(m_modelMatrix.data());
				bgfx::setVertexBuffer(0, groups[g_idx].m_vbh);
				bgfx::setIndexBuffer(groups[g_idx].m_ibh);
				m_texOrBuiltin[0] = 0.0f;
				m_texOrBuiltin[1] = 0.0f;
				m_texOrBuiltin[2] = 0.0f;
				m_texOrBuiltin[3] = 0.0f;
				if (!m_texColorIdx.empty()) {
					int texIdx = g_idx < m_texColorIdx.size() ? 
						m_texColorIdx[g_idx] : m_texColorIdx[0];
					m_texOrBuiltin[0] = 1.0f;
					bgfx::setTexture(0, p_s_colorSampler, m_textures[texIdx]);
				}
				else {
					int builtinIdx = g_idx < m_builtinColorIdx.size() ?
						m_builtinColorIdx[g_idx] : m_builtinColorIdx[0];
					m_texOrBuiltin[0] = 0.0f;
					bgfx::setUniform(p_u_builtinColor, m_builtinColor[builtinIdx].data());
				}
				if (!m_texNormalIdx.empty()) {
					int texIdx = g_idx < m_texNormalIdx.size() ?
						m_texNormalIdx[g_idx] : m_texNormalIdx[0];
					m_texOrBuiltin[1] = 1.0f;
					bgfx::setTexture(1, p_s_normalSampler, m_textures[texIdx]);
				}
				else {
					m_texOrBuiltin[1] = 0.0f;
				}
				if (!m_texAORMIdx.empty()) {
					int texIdx = g_idx < m_texAORMIdx.size() ? 
						m_texAORMIdx[g_idx] : m_texAORMIdx[0];
					m_texOrBuiltin[2] = 1.0f;
					bgfx::setTexture(2, p_s_aormSampler, m_textures[texIdx]);
				}
				else {
					int aormIdx = g_idx < m_builtinAORMIdx.size() ? 
						m_builtinAORMIdx[g_idx] : m_builtinAORMIdx[0];
					m_texOrBuiltin[2] = 0.0f;
					bgfx::setUniform(p_u_builtinAORM, m_builtinAORM[aormIdx].data());
				}
				// Tell shader whether to use texture for color/normal/aorm.
				bgfx::setUniform(p_u_texOrBuiltin, m_texOrBuiltin);
				bgfx::setTexture(6, p_s_shadowSampler, p_shadowMap, UINT32_MAX);
				// Important!
				// When draw opaque objects, don't need to turn on the alpha blend
				// When draw transparent objects, turn down the depth writing
				//  but depth reading and test still enabled.
				// This is to make things look right when a transparent object
				//  covers another transparent object.
				// See D3D12 Redbook P368.
				if (draw_transparent) {
					bgfx::setState(
						0
						| BGFX_STATE_WRITE_RGB
						| BGFX_STATE_WRITE_A
						| BGFX_STATE_DEPTH_TEST_LESS
						| BGFX_STATE_MSAA
						| BGFX_STATE_BLEND_ALPHA
					);
				}
				else {
					bgfx::setState(
						0
						| BGFX_STATE_WRITE_RGB
						| BGFX_STATE_WRITE_A
						| BGFX_STATE_WRITE_Z
						| BGFX_STATE_DEPTH_TEST_LESS
						| BGFX_STATE_MSAA
					);
				}
				bgfx::submit(p_passid, p_program);
			}
		}
		else {
			// Draw instance fail.
		}
	}
	void renderShadow(
		bgfx::ProgramHandle&	p_program,
		int						p_passid
	) {
		if (!m_doShown) { return; }
		mulInstModel();
		int nInstance = m_instanceMatrices.empty() ? 1 : (int)m_instanceMatrices.size();
		if (nInstance == bgfx::getAvailInstanceDataBuffer(nInstance, 64)) {
			auto groups = m_meshPointer->m_groups;
			for (int g_idx = 0; g_idx < groups.size(); ++g_idx) {
				bgfx::InstanceDataBuffer idb;
				// For each instance, transfer 64 bytes of an instance matrix.
				bgfx::allocInstanceDataBuffer(&idb, nInstance, 64);
				uint8_t* data = idb.data;
				for (int ii = 0; ii < nInstance; ++ii) {
					for (int mat_i = 0; mat_i < 16; ++mat_i) {
						*(((float*)data) + mat_i) = m_instxModel[ii][mat_i];
					}
					data += 64;
				}
				bgfx::setInstanceDataBuffer(&idb, 0, nInstance);
				// bgfx::setTransform(m_modelMatrix.data());
				bgfx::setVertexBuffer(0, groups[g_idx].m_vbh);
				bgfx::setIndexBuffer(groups[g_idx].m_ibh);
				bgfx::setState(
					0
					| BGFX_STATE_WRITE_Z
					| BGFX_STATE_DEPTH_TEST_LESS
					| BGFX_STATE_MSAA
				);
				bgfx::submit(p_passid, p_program);
			}
		}
		else {
			// Instance not support
		}
	}
	~RenderItem() {
		// Destroy textures.
		//for (auto& t : m_textures) {
		//	bgfx::destroy(t);
		//}
	}
};

RenderItem BuildAvaRItem()
{
	/// <summary>
	/// https://www.aplaybox.com/details/model/WIDIgZNV0kpl
	/// </summary>
	/// <returns></returns>
	static RenderItem r_ava;
	r_ava.m_meshPointer = meshLoad("..\\resource\\ava\\ava.bin");					// 30 groups
	r_ava.m_textures.push_back(loadTexture("..\\resource\\ava\\body1001.dds"));		// 0
	r_ava.m_textures.push_back(loadTexture("..\\resource\\ava\\face1001.dds"));		// 1
	r_ava.m_textures.push_back(loadTexture("..\\resource\\ava\\body1002.dds"));		// 2
	r_ava.m_textures.push_back(loadTexture("..\\resource\\ava\\daily1045.dds"));	// 3
	r_ava.m_textures.push_back(loadTexture("..\\resource\\ava\\sss.dds"));			// 4
	r_ava.m_textures.push_back(loadTexture("..\\resource\\ava\\mu.dds"));			// 5
	r_ava.m_textures.push_back(loadTexture("..\\resource\\ava\\eye1011.dds"));		// 6
	r_ava.m_textures.push_back(loadTexture("..\\resource\\ava\\gg.dds"));			// 7
	r_ava.m_textures.push_back(loadTexture("..\\resource\\ava\\hair1031.dds"));		// 8
	r_ava.m_textures.push_back(loadTexture("..\\resource\\ava\\daily1044.dds"));	// 9
	r_ava.m_textures.push_back(loadTexture("..\\resource\\ava\\daily1041.dds"));	// 10
	r_ava.m_textures.push_back(loadTexture("..\\resource\\ava\\daily1042.dds"));	// 11
	r_ava.m_textures.push_back(loadTexture("..\\resource\\ava\\daily1043.dds"));	// 12
	r_ava.m_builtinColor.clear();
	r_ava.m_builtinAORM.push_back({ 0.3f, 1.0f, 0.0f, 0.0f });						// 0
	r_ava.m_builtinAORM.push_back({ 0.3f, 0.1f, 0.0f, 0.0f });						// 1
	r_ava.m_builtinAORM.push_back({ 0.3f, 0.3f, 0.1f, 0.0f });						// 2
	r_ava.m_texColorIdx = vector<int>{
		7, 5, 5, 4, 4, 5, 2, 3, 11, 11,
		11, 11, 11, 11, 11, 11, 11, 2, 12, 11,
		11, 11, 10, 9, 8, 1, 6, 7, 5, 0
	};
	r_ava.m_texNormalIdx.clear();
	r_ava.m_texAORMIdx.clear();
	r_ava.m_texNormalIdx.clear();
	r_ava.m_builtinColorIdx.clear();
	r_ava.m_builtinAORMIdx = vector<int>{
		1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 2, 1, 0, 0, 1, 0, 0
	};
	r_ava.m_modelMatrix = array<float, 16>{
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.5f, 0.0f,
		-1.0f, -4.0f, 0.0f, 1.0f
	};
	r_ava.m_instanceMatrices.clear();
	r_ava.m_transparentGroups = set<int>{
		1, 15, 27
	};
	return std::move(r_ava);
}

RenderItem BuildPianoRItem()
{
	/// <summary>
	/// https://www.cgtrader.com/items/2877907/download-page
	/// </summary>
	/// <returns></returns>
	static RenderItem r_piano;
	r_piano.m_meshPointer = meshLoad("..\\resource\\piano\\piano.bin");				// 9 groups
	r_piano.m_textures.push_back(loadTexture("..\\resource\\piano\\piano0.dds"));	// 0
	r_piano.m_textures.push_back(loadTexture("..\\resource\\piano\\piano1.dds"));	// 1
	r_piano.m_textures.push_back(loadTexture("..\\resource\\piano\\piano2.dds"));	// 2
	r_piano.m_textures.push_back(loadTexture("..\\resource\\piano\\piano3.dds"));	// 3
	r_piano.m_textures.push_back(loadTexture("..\\resource\\piano\\piano4.dds"));	// 4
	r_piano.m_textures.push_back(loadTexture("..\\resource\\piano\\piano5.dds"));	// 5
	r_piano.m_textures.push_back(loadTexture("..\\resource\\piano\\piano6.dds"));	// 6
	r_piano.m_textures.push_back(loadTexture("..\\resource\\piano\\piano7.dds"));	// 7
	r_piano.m_textures.push_back(loadTexture("..\\resource\\piano\\piano8.dds"));	// 8
	r_piano.m_textures.push_back(loadTexture("..\\resource\\piano\\piano9.dds"));	// 9
	r_piano.m_builtinColor.clear();
	r_piano.m_builtinAORM.push_back({ 0.5f, 0.8f, 0.0f, 0.0f });					// 0
	r_piano.m_builtinAORM.push_back({ 0.4f, 0.1f, 0.6f, 0.0f });					// 1
	r_piano.m_texColorIdx = vector<int>{
		8, 7, 5, 4, 3, 2, 1, 3, 0
	};
	r_piano.m_texNormalIdx.clear();
	r_piano.m_texAORMIdx.clear();
	r_piano.m_builtinColorIdx.clear();
	r_piano.m_builtinAORMIdx = vector<int>{
		0, 1, 0, 0, 1, 1, 1, 1, 1
	};
	r_piano.m_modelMatrix = array<float, 16>{
		0.0f, 0.0f, 0.12f, 0.0f,
		0.0f, 0.12f, 0.0f, 0.0f,
		-0.12f, 0.0f, 0.0f, 0.0f,
		10.0f, 2.0f, 0.0f, 1.0f
	};
	r_piano.m_instanceMatrices.clear();
	r_piano.m_transparentGroups.clear();
	return std::move(r_piano);
}

RenderItem BuildLawnRItem()
{
	/// <summary>
	/// https://www.cgtrader.com/items/2644859/download-page
	/// </summary>
	/// <returns></returns>
	static RenderItem r_lawn;
	r_lawn.m_meshPointer = meshLoad("..\\resource\\lawn\\lawn.bin");			// 66 groups
	r_lawn.m_textures.push_back(loadTexture("..\\resource\\lawn\\lawn0.dds"));	// 0
	r_lawn.m_builtinColor.clear();
	r_lawn.m_builtinAORM.push_back({ 0.1f, 1.0f, 0.0f, 0.0f });
	r_lawn.m_texColorIdx = vector<int>{ 0 };
	r_lawn.m_texNormalIdx.clear();
	r_lawn.m_texAORMIdx.clear();
	r_lawn.m_builtinColorIdx.clear();
	r_lawn.m_builtinAORMIdx = vector<int>{ 0 };
	r_lawn.m_modelMatrix = array<float, 16>{
		0.18f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.15f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.18f, 0.0f,
		0.0f, -4.6f, 0.0f, 1.0f
	};
	r_lawn.m_instanceMatrices.push_back({
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		100.0f, 0.0f, 100.0f, 1.0f
		});
	r_lawn.m_instanceMatrices.push_back({
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		100.0f, 0.0f, -100.0f, 1.0f
		});
	r_lawn.m_instanceMatrices.push_back({
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		-100.0f, 0.0f, 100.0f, 1.0f
		});
	r_lawn.m_instanceMatrices.push_back({
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		-100.0f, 0.0f, -100.0f, 1.0f
		});
	r_lawn.m_transparentGroups.clear();
	return std::move(r_lawn);
}

RenderItem BuildPBRStoneRItem()
{
	static RenderItem r_stone;
	r_stone.m_meshPointer = meshLoad("..\\resource\\pbr_stone\\pbr_stone.bin");						// 1 group
	r_stone.m_textures.push_back(loadTexture("..\\resource\\pbr_stone\\pbr_stone_base_color.tga"));	// 0
	r_stone.m_textures.push_back(loadTexture("..\\resource\\pbr_stone\\pbr_stone_normal.dds"));		// 1
	r_stone.m_textures.push_back(loadTexture("..\\resource\\pbr_stone\\pbr_stone_aorm.dds"));		// 2
	r_stone.m_builtinColor.clear();
	r_stone.m_builtinAORM.clear();
	r_stone.m_texColorIdx = vector<int>{ 0 };
	r_stone.m_texNormalIdx = vector<int>{ 1 };
	r_stone.m_texAORMIdx = vector<int>{ 2 };
	r_stone.m_builtinColorIdx.clear();
	r_stone.m_builtinAORMIdx.clear();
	r_stone.m_modelMatrix = array<float, 16>{
		0.8f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.8f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.8f, 0.0f,
		-8.0f, -3.5f, 0.0f, 1.0f
	};
	r_stone.m_instanceMatrices.clear();
	r_stone.m_transparentGroups.clear();
	return std::move(r_stone);
}

RenderItem BuildCampfireRItem()
{
	/// <summary>
	/// https://www.cgtrader.com/items/2765262/download-page
	/// TODO: show the flames!
	/// </summary>
	/// <returns></returns>
	static RenderItem r_fire;
	r_fire.m_meshPointer = meshLoad("..\\resource\\campfire\\campfire.bin");
	r_fire.m_textures.push_back(loadTexture("..\\resource\\campfire\\color.dds"));
	r_fire.m_textures.push_back(loadTexture("..\\resource\\campfire\\color1.dds"));
	r_fire.m_textures.push_back(loadTexture("..\\resource\\campfire\\aorm.dds"));
	r_fire.m_textures.push_back(loadTexture("..\\resource\\campfire\\normal.dds"));
	r_fire.m_builtinColor.clear();
	r_fire.m_builtinAORM.clear();
	r_fire.m_texColorIdx = vector<int>{ 1 };
	r_fire.m_texNormalIdx = vector<int>{ 3 };
	r_fire.m_texAORMIdx = vector<int>{ 2 };
	r_fire.m_builtinColorIdx.clear();
	r_fire.m_builtinAORMIdx.clear();
	r_fire.m_modelMatrix = array<float, 16>{
		0.02f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.02f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.02f, 0.0f,
		-2.0f, -3.8f, -4.0f, 1.0f
	};
	r_fire.m_instanceMatrices.clear();
	r_fire.m_transparentGroups.clear();
	return std::move(r_fire);
}

void BuildTestSpheres(map<string, RenderItem>& m)
{
	for (int i = 0; i < 6; ++i) {
		for (int j = 0; j < 6; ++j) {
			string name = std::to_string(i * 10 + j);
			RenderItem r_sphere;
			r_sphere.m_meshPointer = meshLoad("..\\resource\\basic_meshes\\orb.bin");
			r_sphere.m_textures.clear();
			r_sphere.m_builtinColor.push_back({ 1.0f, 0.71f, 0.29f, 1.0f });	// 0: gold
			r_sphere.m_builtinColor.push_back({ 0.95f, 0.93f, 0.88f, 1.0f });	// 1: silver
			r_sphere.m_builtinColor.push_back({ 0.95f, 0.64f, 0.54f, 1.0f });	// 2: copper
			r_sphere.m_builtinAORM.push_back({ 0.5f, 0.2f * i, 0.2f * j, 0.0f });
			r_sphere.m_texColorIdx.clear();
			r_sphere.m_texNormalIdx.clear();
			r_sphere.m_texAORMIdx.clear();
			r_sphere.m_builtinColorIdx = vector<int>{ 0 };
			r_sphere.m_builtinAORMIdx = vector<int>{ 0 };
			r_sphere.m_modelMatrix = array<float, 16>{
				1.0f, 0.0f, 0.0f, 0.0f,
				0.0f, 1.0f, 0.0f, 0.0f,
				0.0f, 0.0f, 1.0f, 0.0f,
				2.0f * i - 5.0f, 2.0f * j - 5.0f, 0.0f, 1.0f
			};
			r_sphere.m_instanceMatrices.clear();
			r_sphere.m_transparentGroups.clear();
			m[name] = std::move(r_sphere);
		}
	}
}

struct PosVertex
{
	float m_x;
	float m_y;
	float m_z;
	static bgfx::VertexLayout ms_layout;
	static void init()
	{
		ms_layout
			.begin()
			.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
			.end();
	}
};

// Define the static of skybox
bgfx::VertexLayout PosVertex::ms_layout;
static const PosVertex m_cubeVertices[8] = {
	{-1.0f,  1.0f,  1.0f },
	{ 1.0f,  1.0f,  1.0f },
	{-1.0f, -1.0f,  1.0f },
	{ 1.0f, -1.0f,  1.0f },
	{-1.0f,  1.0f, -1.0f },
	{ 1.0f,  1.0f, -1.0f },
	{-1.0f, -1.0f, -1.0f },
	{ 1.0f, -1.0f, -1.0f }
};
static const uint16_t m_cubeIndices[36] = {
	 0, 2, 1,
	 1, 2, 3,
	 0, 4, 1,
	 1, 4, 5,
	 4, 6, 7,
	 4, 7, 5,
	 2, 6, 7,
	 2, 7, 3,
	 0, 2, 4,
	 4, 2, 6,
	 1, 5, 7,
	 1, 7, 2
};

class EnvRenderItem
{
	friend EnvRenderItem BuildSkyEnv();
private:
	bgfx::VertexBufferHandle	m_vbh;
	bgfx::IndexBufferHandle		m_ibh;
	bgfx::TextureHandle			m_skyTex;
	bgfx::TextureHandle			m_irrTex;
	bgfx::TextureHandle			m_BRDFLUTTex;
public:
	void renderEnv(
		bgfx::ProgramHandle&	p_program,
		int						p_passid,
		bgfx::UniformHandle&	p_s_cubeSampler
	)
	{
		bgfx::setVertexBuffer(0, m_vbh);
		bgfx::setIndexBuffer(m_ibh);
		bgfx::setTexture(3, p_s_cubeSampler, m_skyTex);
		// To render skycube, change depth test condition to LEQUAL
		// See https://learnopengl.com/Advanced-OpenGL/Cubemaps
		bgfx::setState(
			0
			| BGFX_STATE_WRITE_RGB
			| BGFX_STATE_WRITE_Z
			| BGFX_STATE_DEPTH_TEST_LEQUAL
			| BGFX_STATE_MSAA
		);
		bgfx::submit(p_passid, p_program);
	}
	void setAmbient(
		bgfx::UniformHandle& p_s_irrSampler,
		bgfx::UniformHandle& p_s_BRDFLUT
	)
	{
		bgfx::setTexture(4, p_s_irrSampler, m_irrTex);
		bgfx::setTexture(5, p_s_BRDFLUT, m_BRDFLUTTex);
	}
};

EnvRenderItem BuildSkyEnv() {
	/// <summary>
	/// https://learnopengl.com/Advanced-OpenGL/Cubemaps
	/// </summary>
	/// <returns></returns>
	PosVertex::init();
	static EnvRenderItem cube;
	cube.m_vbh = bgfx::createVertexBuffer(
		bgfx::makeRef(m_cubeVertices, sizeof(m_cubeVertices)),
		PosVertex::ms_layout
	);
	cube.m_ibh = bgfx::createIndexBuffer(
		bgfx::makeRef(m_cubeIndices, sizeof(m_cubeIndices))
	);
	cube.m_skyTex = loadTexture("..\\resource\\env\\sky_prefilter.dds");
	cube.m_irrTex = loadTexture("..\\resource\\env\\sky_irr.dds");
	cube.m_BRDFLUTTex = loadTexture("..\\resource\\env\\brdflut.dds");
	return std::move(cube);
}
