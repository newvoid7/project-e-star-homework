vec3 v_normal    : NORMAL    = vec3(0.0, 0.0, 1.0);
vec3 v_tangent   : TANGENT   = vec3(1.0, 0.0, 0.0);
vec3 v_bitangent : BINORMAL  = vec3(0.0, 1.0, 0.0);
vec2 v_texc		 : TEXCOORD0 = vec2(0.0, 0.0);
vec3 v_pos       : TEXCOORD1 = vec3(0.0, 0.0, 0.0);
vec3 v_toeye     : TEXCOORD2 = vec3(0.0, 0.0, 0.0);
vec4 v_shadowc	 : TEXCOORD3 = vec4(0.0, 0.0, 0.0, 0.0);

vec3 a_position  : POSITION;
vec3 a_normal    : NORMAL;
vec3 a_tangent   : TANGENT;
vec2 a_texcoord0 : TEXCOORD0;
vec4 i_data0     : TEXCOORD7;
vec4 i_data1     : TEXCOORD6;
vec4 i_data2     : TEXCOORD5;
vec4 i_data3     : TEXCOORD4;
