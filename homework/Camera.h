#pragma once
#include <array>
#include <bx/math.h>
#include "bgfx_utils.h"
using std::array;

class Camera
{
private:
	array<float, 16>	m_projMtx;
	array<float, 16>	m_viewMtx;
	bx::Vec3			m_eye;
	bx::Vec3			m_at;
	bx::Vec3			m_up;
	float				m_radius;
	float				m_azimuth;
	float				m_elevation;
	bx::Vec3			m_realView = { 0.0f, 0.0f, 0.0f };
	bx::Vec3			m_realRight = { 0.0f, 0.0f, 0.0f };
	bx::Vec3			m_realUp = { 0.0f, 0.0f, 0.0f };
public:
	Camera(float p_aspect = 16.0f / 9.0f):
		m_projMtx(),
		m_viewMtx(),
		m_eye(0.0f, 0.0f, -20.0f),
		m_at(0.0f, 0.0f, 0.0f),
		m_up(0.0f, 1.0f, 0.0f),
		m_radius(20.0f),
		m_azimuth(bx::kPi),
		m_elevation(0.0f)
	{
		bx::mtxLookAt(
			m_viewMtx.data(),
			m_eye,
			m_at,
			m_up
		);
		bx::mtxProj(
			m_projMtx.data(),
			60.0f, 
			p_aspect, 
			0.1f, 
			100.0f, 
			bgfx::getCaps()->homogeneousDepth
		);
	}
	inline const float* getProjMtx()const {
		return m_projMtx.data();
	}
	inline const float* getViewMtx()const {
		return m_viewMtx.data();
	}
	void setProjection(float fov, float aspect, float near, float far) {
		bx::mtxProj(m_projMtx.data(), fov, aspect, near, far, bgfx::getCaps()->homogeneousDepth);
	}
private:
	void calcAxis() {
		m_realView = bx::normalize(bx::sub(m_at, m_eye));
		m_realRight = bx::normalize(bx::cross(m_up, m_realView));
		m_realUp = bx::cross(m_realView, m_realRight);
	}
	void updateViewMtx() {
		bx::mtxLookAt(m_viewMtx.data(), m_eye, m_at, m_up);
	}
public:
	void YPDR(float yall, float pitch, float dolly, float roll);
	void AEDR(float azimuth, float elevation, float dolly, float roll);
	void Move(float d_up, float d_right, float d_forward);
	void Trackball(float dx, float dy);
};

inline void Camera::YPDR(float yall, float pitch, float dolly, float roll)
{

}

inline void Camera::AEDR(float azimuth, float elevation, float dolly, float roll = 0.0f)
{
	/// <summary>
	/// This is not trackball, 'at' and 'up' doesn't change.
	/// In real trackball, 'up' should change.
	/// </summary>
	/// <param name="azimuth"></param>
	/// <param name="elevation"></param>
	/// <param name="dolly"></param>
	/// <param name="roll"></param>
	m_azimuth += azimuth;
	m_elevation += elevation;
	m_radius -= dolly;
	// Limitation
	if (m_elevation > bx::kPiHalf - 0.001f) {
		m_elevation = bx::kPiHalf - 0.001f;
	}
	if (m_elevation < -bx::kPiHalf + 0.001f) {
		m_elevation = -bx::kPiHalf + 0.001f;
	}
	if (m_radius > 100.0f) {
		m_radius = 100.0f;
	}
	if (m_radius < 0.1f) {
		m_radius = 0.1f;
	}
	m_eye.x = m_radius * bx::cos(m_elevation) * bx::sin(m_azimuth) + m_at.x;
	m_eye.y = m_radius * bx::sin(m_elevation) + m_at.y;
	m_eye.z = m_radius * bx::cos(m_elevation) * bx::cos(m_azimuth) + m_at.z;
	updateViewMtx();
}

inline void Camera::Move(float d_up, float d_right, float d_forward = 0.0f)
{
	/// <summary>
	/// Move 'eye' and 'at' at the same time
	/// </summary>
	/// <param name="d_up"></param>
	/// <param name="d_right"></param>
	/// <param name="d_forward"></param>
	calcAxis();
	bx::Vec3 translation = bx::add(
		bx::mul(m_realRight, d_right),
		bx::mul(m_realUp, d_up)
	);
	m_eye = bx::add(m_eye, translation);
	m_at = bx::add(m_at, translation);
	updateViewMtx();
}

inline void Camera::Trackball(float dx, float dy)
{
	///TODO
}
