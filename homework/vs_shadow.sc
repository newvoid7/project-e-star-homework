$input a_position, i_data0, i_data1, i_data2, i_data3

#include "../bgfx/examples/common/common.sh"

void main()
{
	mat4 modelMtx = mtxFromCols(i_data0, i_data1, i_data2, i_data3);
	vec3 pos = mul(modelMtx, vec4(a_position, 1.0)).xyz;

	gl_Position = mul(u_viewProj, vec4(pos, 1.0));
}