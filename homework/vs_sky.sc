$input a_position
$output v_pos

#include "../bgfx/examples/common/common.sh"

void main()
{
	v_pos = a_position;
	mat4 viewRotation = u_view;
	viewRotation[0].w = 0.0;
	viewRotation[1].w = 0.0;
	viewRotation[2].w = 0.0;
	viewRotation[3] = vec4(0.0, 0.0, 0.0, 0.0);
	// Delete the translation of view
	vec4 pos = mul(mul(u_proj, viewRotation), vec4(a_position, 1.0));
	gl_Position = pos.xyww;		// Ensure that depth = 1
}