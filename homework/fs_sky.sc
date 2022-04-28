$input v_pos

#include "../bgfx/examples/common/common.sh"

SAMPLERCUBE(s_cube, 3);

void main()
{
	// texture lod parameter: mip 0 is necessary here!!!
	gl_FragColor = toLinear(textureCubeLod(s_cube, v_pos, 0.0));
	gl_FragColor = toGamma(gl_FragColor);
}