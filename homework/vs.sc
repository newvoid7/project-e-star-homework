$input a_position, a_normal, a_tangent, a_texcoord0, i_data0, i_data1, i_data2, i_data3
$output v_pos, v_toeye, v_normal, v_tangent, v_bitangent, v_texc, v_shadowc

// The input texc variable must be named as 'a_texcoord0',
// or all the texture coordinates will always read as vec2(0.0, 0.0).
// See https://github.com/bkaradzic/bgfx/issues/1579

#include "../bgfx/examples/common/common.sh"

uniform mat4 u_lightMtx;

void main()
{
	mat4 modelMtx = mtxFromCols(i_data0, i_data1, i_data2, i_data3);

	vec3 pos = mul(modelMtx, vec4(a_position, 1.0)).xyz;
	// pos = mul(u_model[0], vec4(pos, 1.0)).xyz;
	v_pos = pos;

	gl_Position = mul(u_viewProj, vec4(pos, 1.0));

	vec3 normal = a_normal * 2.0 - 1.0;
	normal = mul(modelMtx, vec4(normal, 0.0)).xyz;
	// normal = normalize(mul(u_model[0], vec4(normal, 0.0)).xyz);
	v_normal = normal;

	vec3 tangent = a_tangent * 2.0 - 1.0;
	tangent = mul(modelMtx, vec4(tangent, 0.0)).xyz;
	// tangent = normalize(mul(u_model[0], vec4(tangent, 0.0)).xyz);
	v_tangent = tangent;

	v_bitangent = cross(v_normal, v_tangent);

	vec3 eye_pos = u_invView[3].xyz;
	v_toeye = normalize(eye_pos - v_pos);

	v_texc = a_texcoord0;

	vec3 pos_offset = pos + normal * 0.001;
	v_shadowc = mul(u_lightMtx, vec4(pos_offset, 1.0));
}