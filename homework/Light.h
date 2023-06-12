#pragma once
#include <bgfx/bgfx.h>
#include <bx/math.h>
#include <map>
#include <string>
#include <array>

using std::map;
using std::string;
using std::array;

// Keep it same as in shader.
constexpr int MAX_LIGHTS = 4;

class Light
{
	friend void lightsToUniform(const map<string, Light>& lights, 
		bgfx::UniformHandle u_lights);
public:
	enum class LIGHT_TYPE {
		Directional,
		Point,
		Spot,
		Count
	};
private:
	bx::Vec3 strength;				// D/P/S
	float falloffStart;				// P/S
	bx::Vec3 direction;				// D/S
	float falloffEnd;				// P/S
	bx::Vec3 position;				// P/S
	float spotPower;				// S
	LIGHT_TYPE type;
public:
	Light():
		strength(0.0f, 0.0f, 0.0f),
		falloffStart(0.0f),
		direction(0.0f, 0.0f, 0.0f),
		falloffEnd(0.0f),
		position(0.0f, 0.0f, 0.0f),
		spotPower(0.0f),
		type(LIGHT_TYPE::Count)
	{}
	Light(LIGHT_TYPE directional, 
		const bx::Vec3& p_strength, 
		const bx::Vec3& p_direction):
		strength(p_strength),
		direction(p_direction),
		position(0.0f, 0.0f, 0.0f),
		falloffStart(0.0f),
		falloffEnd(0.0f),
		spotPower(0.0f),
		type(LIGHT_TYPE::Directional)
	{
		if (directional != LIGHT_TYPE::Directional) {
			// NOT RIGHT
		}
	}
	Light(LIGHT_TYPE point,
		const bx::Vec3& p_strength,
		const bx::Vec3& p_position,
		float p_falloffStart,
		float p_falloffEnd) :
		strength(p_strength),
		position(p_position),
		falloffStart(p_falloffStart),
		falloffEnd(p_falloffEnd),
		spotPower(0.0f),
		direction(0.0f, 0.0f, 0.0f),
		type(LIGHT_TYPE::Point)
	{
		if (point != LIGHT_TYPE::Point) {
			// NOT RIGHT
		}
	}
	Light(LIGHT_TYPE spot,
		const bx::Vec3& p_strength,
		const bx::Vec3& p_direction,
		const bx::Vec3& p_position,
		float p_falloffStart,
		float p_falloffEnd,
		float p_spotPower) :
		strength(p_strength),
		direction(p_direction),
		position(p_position),
		falloffStart(p_falloffStart),
		falloffEnd(p_falloffEnd),
		spotPower(p_spotPower),
		type(LIGHT_TYPE::Spot)
	{
		if (spot != LIGHT_TYPE::Spot) {
			// NOT RIGHT
		}
	}
	inline void setDirection(const bx::Vec3& p_dir) { direction = p_dir; }
	inline void setStrength(const bx::Vec3& p_str) { strength = p_str; }
	array<float, 16> getViewMtx() const
	{
		array<float, 16> view;
		bx::Vec3 eye = { 0.0f, 0.0f, 0.0f }, at = { 0.0f, 0.0f, 0.0f };
		switch (type)
		{
		case Light::LIGHT_TYPE::Directional:
			eye = bx::sub(at, bx::mul(direction, 50.0f));
			break;
		case Light::LIGHT_TYPE::Point:
			// Point light should generate a cube map shadow map
			break;
		case Light::LIGHT_TYPE::Spot:
			eye = position;
			at = bx::add(eye, direction);
			break;
		case Light::LIGHT_TYPE::Count:
			break;
		default:
			break;
		}
		bx::mtxLookAt(view.data(), eye, at);
		return view;
	}
	array<float, 16> getProjMtx() const
	{
		array<float, 16> proj;
		const bgfx::Caps* caps = bgfx::getCaps();
		switch (type)
		{
		case Light::LIGHT_TYPE::Directional:
			{
			const float area = 60.0f, near = 0.1f, far = 80.0f;
			bx::mtxOrtho(proj.data(), -area, area, -area, area,
			near, far, 0.0f, caps->homogeneousDepth); }
			break;
		case Light::LIGHT_TYPE::Point:
			break;
		case Light::LIGHT_TYPE::Spot:
		{	float fov = 60.0f;	// Should related with spotpower
			bx::mtxProj(proj.data(), fov, 1.0f, falloffStart, falloffEnd,
			caps->homogeneousDepth); }
			break;
		case Light::LIGHT_TYPE::Count:
			break;
		default:
			break;
		}
		return proj;
	}
	array<float, 16> getViewProjCropMtx() const
	{
		// View -> Projection -> NDC (non-device co
		array<float, 16> ret;
		const bgfx::Caps* caps = bgfx::getCaps();
		const float sy = caps->originBottomLeft ? 0.5f : -0.5f;
		const float sz = caps->homogeneousDepth ? 0.5f : 1.0f;
		const float tz = caps->homogeneousDepth ? 0.5f : 0.0f;
		const float cropMtx[16] =
		{
			0.5f, 0.0f, 0.0f, 0.0f,
			0.0f,   sy, 0.0f, 0.0f,
			0.0f, 0.0f, sz,   0.0f,
			0.5f, 0.5f, tz,   1.0f,
		};
		float viewProjMtx[16];
		bx::mtxMul(viewProjMtx, getViewMtx().data(), getProjMtx().data());
		bx::mtxMul(ret.data(), viewProjMtx, cropMtx);

		//// Because the ViewProjectionCrop matrix often go to uniform
		//array<float, 16> t_ret;
		//bx::mtxTranspose(t_ret.data(), ret.data());
		//switch (bgfx::getRendererType())
		//{
		//case bgfx::RendererType::Direct3D9:
		//case bgfx::RendererType::Direct3D11:
		//case bgfx::RendererType::Direct3D12:
		//	return t_ret;
		//	break;
		//case bgfx::RendererType::OpenGL:
		//case bgfx::RendererType::OpenGLES:
		//	return ret;
		//	break;
		//default:
		//	break;
		//}
		return ret;
	}
};

// Transfer a map of lights to uniform
void lightsToUniform(const map<string, Light>& lights, bgfx::UniformHandle u_lights)
{
	float arrayLights[MAX_LIGHTS][16];
	auto begin = lights.begin();
	for (int i = 0; i < MAX_LIGHTS; ++i) {
		for (int j = 0; j < 16; ++j) {
			arrayLights[i][j] = 0.0f;
		}
	}
	for (int i = 0; i < MAX_LIGHTS && begin != lights.end(); ++i, ++begin) {
		arrayLights[i][0] = begin->second.strength.x;
		arrayLights[i][1] = begin->second.strength.y;
		arrayLights[i][2] = begin->second.strength.z;
		switch (begin->second.type)
		{
		case Light::LIGHT_TYPE::Directional:
			arrayLights[i][4] = begin->second.direction.x;
			arrayLights[i][5] = begin->second.direction.y;
			arrayLights[i][6] = begin->second.direction.z;
			arrayLights[i][12] = 1.0f;
			break;
		case Light::LIGHT_TYPE::Point:
			arrayLights[i][3] = begin->second.falloffStart;
			arrayLights[i][7] = begin->second.falloffEnd;
			arrayLights[i][8] = begin->second.position.x;
			arrayLights[i][9] = begin->second.position.y;
			arrayLights[i][10] = begin->second.position.z;
			arrayLights[i][13] = 1.0f;
			break;
		case Light::LIGHT_TYPE::Spot:
			arrayLights[i][3] = begin->second.falloffStart;
			arrayLights[i][4] = begin->second.direction.x;
			arrayLights[i][5] = begin->second.direction.y;
			arrayLights[i][6] = begin->second.direction.z;
			arrayLights[i][7] = begin->second.falloffEnd;
			arrayLights[i][8] = begin->second.position.x;
			arrayLights[i][9] = begin->second.position.y;
			arrayLights[i][10] = begin->second.position.z;
			arrayLights[i][11] = begin->second.spotPower;
			arrayLights[i][14] = 1.0f;
			break;
		default:
			break;
		}
	}
	float t_arrayLights[MAX_LIGHTS][16];
	for (int i = 0; i < MAX_LIGHTS; ++i) {
		bx::mtxTranspose(t_arrayLights[i], arrayLights[i]);
	}
	// Set a matrix uniform, DirectX and OpenGL are different
	switch (bgfx::getRendererType())
	{
	case bgfx::RendererType::Direct3D9:
	case bgfx::RendererType::Direct3D11:
	case bgfx::RendererType::Direct3D12:
		bgfx::setUniform(u_lights, t_arrayLights, MAX_LIGHTS);
		break;
	case bgfx::RendererType::OpenGL:
	case bgfx::RendererType::OpenGLES:
		bgfx::setUniform(u_lights, arrayLights, MAX_LIGHTS);
		break;
	default:
		break;
	}
}
