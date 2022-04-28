/*
 * Copyright 2011-2022 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx/blob/master/LICENSE
 */

#include <bx/uint32_t.h>
#include <bx/math.h>
#include "common.h"
#include "bgfx_utils.h"
#include "bgfx_logo.h"
#include "imgui/imgui.h"
#include "entry/input.h"
//#include "camera.h"

#include "../build/Light.h"
#include "../build/RenderItem.h"
#include "../build/Camera.h"

#include <vector>
#include <array>
#include <map>
#include <string>
using std::vector;
using std::array;
using std::map;
using std::string;

namespace
{
	constexpr int SHADOW_PASS_ID = 0;
	constexpr int SCENE_PASS_ID = 1;
class EStarHomework : public entry::AppI
{
private:
	bgfx::UniformHandle					u_texOrNot;
	bgfx::UniformHandle					u_lights;
	bgfx::UniformHandle					u_builtinAORM;
	bgfx::UniformHandle					u_builtinColor;
	bgfx::UniformHandle					u_lightMtx;
	bgfx::UniformHandle					s_colorSampler;
	bgfx::UniformHandle					s_normalSampler;
	bgfx::UniformHandle					s_aormSampler;
	bgfx::UniformHandle					s_cubeMapSampler;
	bgfx::UniformHandle					s_irrSampler;
	bgfx::UniformHandle					s_BRDFLUTSampler;
	bgfx::UniformHandle					s_shadowSampler;
	bgfx::FrameBufferHandle				m_shadowFB;
	bgfx::TextureHandle					m_shadowMap;
	bgfx::ProgramHandle					m_programScene;
	bgfx::ProgramHandle					m_programSky;
	bgfx::ProgramHandle					m_programShadow;
	
	// Use std::map to modify conveniently
	map<string, RenderItem>				m_renderItems;
	map<string, Light>					m_lights;
	map<string, Camera>					m_cameras;
	map<string, EnvRenderItem>			m_envs;

	// User Interface
	entry::MouseState					m_mouseState;
	entry::MouseState					m_lastMouseState;
	uint32_t							m_width;
	uint32_t							m_height;
	uint32_t							m_debug;
	uint32_t							m_reset;
	int64_t								m_timeOffset;

public:
	EStarHomework(const char* _name, const char* _description, const char* _url)
		: entry::AppI(_name, _description, _url)
	{
		m_width = 0;
		m_height = 0;
		m_debug = BGFX_DEBUG_NONE;
		m_reset = BGFX_RESET_NONE;
	}

	void init(int32_t _argc, const char* const* _argv, uint32_t _width, uint32_t _height) override
	{
		Args args(_argc, _argv);

		m_width  = _width;
		m_height = _height;
		m_debug  = BGFX_DEBUG_TEXT;
		m_reset  = BGFX_RESET_VSYNC;

		bgfx::Init init;
		init.type     = args.m_type;
		init.type = bgfx::RendererType::OpenGL;
		init.vendorId = args.m_pciId;
		init.resolution.width  = m_width;
		init.resolution.height = m_height;
		init.resolution.reset  = m_reset;
		bgfx::init(init);

		// Enable debug text.
		bgfx::setDebug(m_debug);

		// Set view 0 clear state.
		bgfx::setViewClear(0
			, BGFX_CLEAR_COLOR|BGFX_CLEAR_DEPTH
			, 0x303030ff
			, 1.0f
			, 0
			);

		// Load programs
		m_programScene = loadProgram("vs", "fs");
		m_programSky = loadProgram("vs_sky", "fs_sky");
		m_programShadow = loadProgram("vs_shadow", "fs_shadow");

		// Build render items
		m_renderItems["AvA"] = BuildAvaRItem();
		m_renderItems["lawn"] = BuildLawnRItem();
		m_renderItems["PBR stone"] = BuildPBRStoneRItem();
		m_renderItems["piano"] = BuildPianoRItem();
		m_renderItems["fire"] = BuildCampfireRItem();
		//BuildTestSpheres(m_renderItems);	// for test

		// Build sky
		m_envs["sky"] = BuildSkyEnv();

		// Create lights
		m_lights["main"] = Light(
			Light::LIGHT_TYPE::Directional,
			{ 6.0f, 4.0f, 3.0f },
			{ 1.0f, -1.0f, 1.0f }
		);
		//m_lights["fire"] = Light(
		//	Light::LIGHT_TYPE::Point,
		//	{ 5.0f, 4.0f, 1.0f },
		//	{ m_renderItems["fire"].m_modelMatrix[12],
		//	m_renderItems["fire"].m_modelMatrix[13],
		//	m_renderItems["fire"].m_modelMatrix[14] },
		//	0.0f,
		//	5.0f
		//);
		//m_lights["side"] = Light(
		//	Light::LIGHT_TYPE::Directional,
		//	{ 0.6f, 0.6f, 0.6f },
		//	{ -1.0f, 0.0f, 1.0f }
		//);
		//m_lights["top spot light"] = Light(
		//	Light::LIGHT_TYPE::Spot,
		//	{ 5.0f, 5.0f, 5.0f },
		//	{ 0.0f, -1.0f, 1.0f },
		//	{ 0.0f, 30.0f, -10.0f },
		//	0.0f,
		//	80.0f,
		//	1.05f
		//);
		
		// Create a main camera
		m_cameras["main"] = Camera(float(m_width) / float(m_height));

		// Create samplers uniform buffer
		s_colorSampler = bgfx::createUniform("s_texColor", bgfx::UniformType::Sampler);
		s_normalSampler = bgfx::createUniform("s_texNormal", bgfx::UniformType::Sampler);
		s_aormSampler = bgfx::createUniform("s_texAORM", bgfx::UniformType::Sampler);
		s_cubeMapSampler = bgfx::createUniform("s_cube", bgfx::UniformType::Sampler);
		s_irrSampler = bgfx::createUniform("s_irr", bgfx::UniformType::Sampler);
		s_BRDFLUTSampler = bgfx::createUniform("s_brdflut", bgfx::UniformType::Sampler);
		s_shadowSampler = bgfx::createUniform("s_shadow", bgfx::UniformType::Sampler);

		//
		m_shadowFB = BGFX_INVALID_HANDLE;
		m_shadowMap = BGFX_INVALID_HANDLE;

		// Create built-in color/AORM uniform buffer
		u_builtinColor = bgfx::createUniform("u_builtinColor", bgfx::UniformType::Vec4);
		u_builtinAORM = bgfx::createUniform("u_builtinAORM", bgfx::UniformType::Vec4);

		// Create a uniform buffer that indicates whether use built-in color/normal/AORM or not
		u_texOrNot = bgfx::createUniform("u_texOrBuiltin", bgfx::UniformType::Vec4);

		// Create light uniform buffer
		u_lights = bgfx::createUniform("u_lights", bgfx::UniformType::Mat4, MAX_LIGHTS);
		
		// Create light matrix uniform
		u_lightMtx = bgfx::createUniform("u_lightMtx", bgfx::UniformType::Mat4);

		//
		m_timeOffset = bx::getHPCounter();
		imguiCreate();
	}

	virtual int shutdown() override
	{
		imguiDestroy();

		bgfx::destroy(u_texOrNot);
		bgfx::destroy(u_lights);
		bgfx::destroy(u_builtinAORM);
		bgfx::destroy(u_builtinColor);
		bgfx::destroy(u_lightMtx);
		bgfx::destroy(s_colorSampler);
		bgfx::destroy(s_normalSampler);
		bgfx::destroy(s_aormSampler);
		bgfx::destroy(s_cubeMapSampler);
		bgfx::destroy(s_irrSampler);
		bgfx::destroy(s_BRDFLUTSampler);
		bgfx::destroy(s_shadowSampler);
		bgfx::destroy(m_shadowFB);
		bgfx::destroy(m_shadowMap);
		bgfx::destroy(m_programScene);
		bgfx::destroy(m_programSky);
		bgfx::destroy(m_programShadow);

		// Shutdown bgfx.
		bgfx::shutdown();

		return 0;
	}

	bool update() override
	{
		if (!entry::processEvents(m_width, m_height, m_debug, m_reset, &m_mouseState) )
		{
			imguiBeginFrame(m_mouseState.m_mx
				,  m_mouseState.m_my
				, (m_mouseState.m_buttons[entry::MouseButton::Left  ] ? IMGUI_MBUT_LEFT   : 0)
				| (m_mouseState.m_buttons[entry::MouseButton::Right ] ? IMGUI_MBUT_RIGHT  : 0)
				| (m_mouseState.m_buttons[entry::MouseButton::Middle] ? IMGUI_MBUT_MIDDLE : 0)
				,  m_mouseState.m_mz
				, uint16_t(m_width)
				, uint16_t(m_height)
				);

			showExampleDialog(this);

			imguiEndFrame();

			// Process the message
			int64_t now = bx::getHPCounter();
			const double freq = double(bx::getHPFrequency());
			float time = float((now - m_timeOffset) / freq);
			msgProc(time);

			// This dummy draw call is here to make sure that view 0 is cleared
			// if no other draw calls are submitted to view 0.
			bgfx::touch(0);

			// Use debug font to print information about this example.
			bgfx::dbgTextClear();

			// Clear
			if (bgfx::isValid(m_shadowFB)) { bgfx::destroy(m_shadowFB); }
			m_shadowMap = BGFX_INVALID_HANDLE;

			// Shadow pass
			//bool f = 0 != (bgfx::getCaps()->supported & BGFX_CAPS_TEXTURE_COMPARE_LEQUAL);
			int m_shadowMapSize = 2000;
			bgfx::TextureHandle fbtextures[] =
			{
				bgfx::createTexture2D(
					m_shadowMapSize, 
					m_shadowMapSize, 
					false, 
					1, 
					bgfx::TextureFormat::D32, 
					BGFX_TEXTURE_RT | BGFX_SAMPLER_COMPARE_LEQUAL
					),
			};
			m_shadowMap = fbtextures[0];
			m_shadowFB = bgfx::createFrameBuffer(BX_COUNTOF(fbtextures), fbtextures, true);
			bgfx::setViewRect(SHADOW_PASS_ID, 0, 0, m_shadowMapSize, m_shadowMapSize);
			bgfx::setViewFrameBuffer(SHADOW_PASS_ID, m_shadowFB);
			bgfx::setViewTransform(SHADOW_PASS_ID, m_lights["main"].getViewMtx().data(),
				m_lights["main"].getProjMtx().data());
			bgfx::setViewClear(SHADOW_PASS_ID, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH,
				0x303030ff, 1.0f, 0);
			for (auto& ritem : m_renderItems) {
				ritem.second.renderShadow(m_programShadow, SHADOW_PASS_ID);
			}

			// Scene pass
			// Set viewport
			bgfx::setViewRect(SCENE_PASS_ID, 0, 0, uint16_t(m_width), uint16_t(m_height));
			// Set ViewProjection
			bgfx::setViewTransform(SCENE_PASS_ID, m_cameras["main"].getViewMtx(),
				m_cameras["main"].getProjMtx());
			bgfx::setViewClear(SCENE_PASS_ID, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 
				0x303030ff, 1.0f, 0);

			// Prepare lights
			lightsToUniform(m_lights, u_lights);

			// Set light matrix uniform
			bgfx::setUniform(u_lightMtx, m_lights["main"].getViewProjCropMtx().data());
			
			// Set ambient related textures
			m_envs["sky"].setAmbient(s_irrSampler, s_BRDFLUTSampler);

			// Draw opaque firstly, draw transparent after that.
			for (auto& rItem : m_renderItems) {
				rItem.second.render(m_programScene, SCENE_PASS_ID,
					s_colorSampler, s_normalSampler, s_aormSampler, s_shadowSampler,
					u_builtinColor, u_builtinAORM, u_texOrNot,
					m_shadowMap, false);
			}
			for (auto& rItem : m_renderItems) {
				rItem.second.render(m_programScene, SCENE_PASS_ID,
					s_colorSampler, s_normalSampler, s_aormSampler, s_shadowSampler,
					u_builtinColor, u_builtinAORM, u_texOrNot,
					m_shadowMap, true);
			}

			// Draw sky after other objects are rendered
			m_envs["sky"].renderEnv(m_programSky, SCENE_PASS_ID, s_cubeMapSampler);

			// Advance to next frame. Rendering thread will be kicked to
			// process submitted rendering primitives.
			bgfx::frame();

			m_lastMouseState = m_mouseState;
			return true;
		}

		return false;
	}

	void msgProc(float p_time)
	{
		// Camera control
		auto dx = m_mouseState.m_mx - m_lastMouseState.m_mx;
		auto dy = m_mouseState.m_my - m_lastMouseState.m_my;
		auto dz = m_mouseState.m_mz - m_lastMouseState.m_mz;
		///TODO: inertia
		if (m_mouseState.m_buttons[entry::MouseButton::Left]) {
			
		}
		else {
			dx = 0;
			dy = 0;
		}
		// This is not trackball, this is just viewing on a sphere
		// Azimuth, Elevation, Dolly, Roll (default = 0)
		m_cameras["main"].AEDR(0.002f * dx, 0.002f * dy, 0.6f * dz);
		// Keyboard
		float moveRight = 0.0f, moveUp = 0.0f;
		if (inputGetKeyState(entry::Key::KeyW)) {
			moveUp += 0.05f;
		}
		if (inputGetKeyState(entry::Key::KeyS)) {
			moveUp -= 0.05f;
		}
		if (inputGetKeyState(entry::Key::KeyA)) {
			moveRight -= 0.05f;
		}
		if (inputGetKeyState(entry::Key::KeyD)) {
			moveRight += 0.05f;
		}
		if (m_mouseState.m_buttons[entry::MouseButton::Middle]) {
			moveUp = (m_mouseState.m_my - m_lastMouseState.m_my) * 0.02f;
			moveRight = (m_mouseState.m_mx - m_lastMouseState.m_mx) * -0.02f;
		}
		m_cameras["main"].Move(moveUp, moveRight);

		// Light control: day-night circulation
		float dayLen = 20.0f, offset = 0.195913276f * dayLen;
		int cycle = (p_time + offset) / dayLen;
		float clock = (p_time + offset) - cycle * dayLen;
		float altitude = (clock / dayLen) * bx::kPi;
		m_lights["main"].setDirection(
			{ bx::cos(altitude) / 1.41421356f,
			-bx::sin(altitude),
			bx::cos(altitude) / 1.41421356f }
		);
		if (cycle % 2 == 0) {	// is day
			m_lights["main"].setStrength({ 6.0f, 4.0f, 3.0f });
		}
		else {					// is night
			m_lights["main"].setStrength({ 1.5f, 1.5f, 3.0f });
		}
	}
};

} // namespace

int _main_(int _argc, char** _argv)
{
	EStarHomework app("Ava and her piano", "", "");
	return entry::runApp(&app, _argc, _argv);
}

