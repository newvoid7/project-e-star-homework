$input v_pos, v_toeye, v_normal, v_tangent, v_bitangent, v_texc, v_shadowc

#include "../bgfx/examples/common/common.sh"

#define MAX_LIGHTS 4

SAMPLER2D(s_texColor, 0);
SAMPLER2D(s_texNormal, 1);
SAMPLER2D(s_texAORM, 2);
SAMPLERCUBE(s_cube, 3);
SAMPLERCUBE(s_irr, 4);
SAMPLER2D(s_brdflut, 5);
SAMPLER2DSHADOW(s_shadow, 6);

uniform vec4 u_builtinAORM;
uniform vec4 u_builtinColor;
uniform vec4 u_texOrBuiltin;
uniform mat4 u_lights[MAX_LIGHTS];

const float kPi = 3.14159265359;

vec3 BlinnPhong(vec3 lightStrength, vec3 toLight, vec3 normal, vec3 toEye,
	vec3 diffuseAlbedo, float metallic, float roughness)
{
	float nl = dot(normal, toLight);
	vec3 h = normalize(toEye + toLight);
	float nh = dot(normal, h);

	vec3 specAlbedo = diffuseAlbedo;
	float gloss = 10.0 * (1.0 - roughness);

	vec3 diffuse = lightStrength * diffuseAlbedo / kPi * max(nl, 0.0);
	vec3 specular = lightStrength * specAlbedo / kPi * pow(max(nh, 0.0), gloss);

	return diffuse + specular;
}

vec3 Fresnel(vec3 F0, float HdotV)
{
	// Unreal Engine version, faster
	vec3 F = F0 + (vec3_splat(1.0) - F0) * exp2((-5.55473 * max(HdotV, 0.0) - 6.98316) * max(HdotV, 0.0));
	// Schlick version
	// vec3 F = F0 + (1.0 - F0) * pow((1.0 - max(hv, 0.0)), 5.0);
	return F;
}

float NDF(float NdotH, float alpha2)
{
	// Trowbridge-Reitz GGX
	float denominator = max(NdotH, 0.0) * max(NdotH, 0.0) * (alpha2 - 1.0) + 1.0;
	denominator = kPi * denominator * denominator;
	float D = alpha2 / denominator;
	return D;
}

vec3 PBRDirect(vec3 lightStrength, vec3 toLight, vec3 normal, vec3 toEye,
	vec3 diffuseAlbedo, float metallic, float roughness)
{
	// All directional vectors are normalized

	// BRDF: Cook-Torrance

	float nl = dot(normal, toLight);
	float nv = dot(normal, toEye);
	vec3 h = normalize(toEye + toLight);
	float nh = dot(normal, h);
	float hv = dot(h, toEye);
	
	float alpha = roughness * roughness;
	float alpha2 = alpha * alpha;
	float k = (roughness + 1.0) * (roughness + 1.0) / 8.0;
	vec3 F0 = diffuseAlbedo * metallic + vec3_splat(0.04) * (1.0 - metallic);

	// Fresnel factor:
	vec3 F = Fresnel(F0, hv);

	// Distribution of normal:
	float D = NDF(nh, alpha2);

	// Geometry factor: Smith, Schlick-GGX
	// float Gv = max(nv, 0.0) / (max(nv, 0.0) * (1.0 - k) + k);
	// float Gl = max(nl, 0.0) / (max(nl, 0.0) * (1.0 - k) + k);
	// float G = Gv * Gl;
	// float Vis = G / (4.0 * max(nv, 0.0) * max(nl, 0.0) + 0.001);
	float Vis = 0.25 / ((max(nv, 0.0) * (1.0 - k) + k) * (max(nl, 0.0) * (1.0 - k) + k));

	// Direct lighting, kd * diffuse + ks * specular
	vec3 fs = F * D * Vis;
	vec3 fd = (vec3_splat(1.0) - F) * (1.0 - metallic) * diffuseAlbedo / kPi;
	
	return (fd + fs) * lightStrength * max(nl, 0.0);
}

vec3 PBRAmbient(vec3 normal, vec3 toEye,
	vec3 diffuseAlbedo, float ao, float metallic, float roughness)
{
	float nv = dot(normal, toEye);

	vec3 F0 = diffuseAlbedo * metallic + vec3_splat(0.04) * (1.0 - metallic);
	vec3 F = Fresnel(F0, nv);

	vec3 irr = toLinear(textureCube(s_irr, normal)).xyz;
	vec3 diffuseAmbient = diffuseAlbedo / kPi * irr;

	vec2 envBRDFtexc = vec2(max(nv, 0.0), roughness);
	vec2 envBRDF = toLinear(texture2D(s_brdflut, envBRDFtexc)).xy;
	vec3 specularBRDF = F * envBRDF.x + envBRDF.y;
	vec3 radianceVec = 2.0 * nv * normal - toEye;
	// max mip: 12, thus roughness * 12
	vec3 prefilteredEnv = toLinear(textureCubeLod(s_cube, radianceVec, roughness * 12.0)).xyz;
	vec3 specularAmbient = prefilteredEnv * specularBRDF;

	vec3 ambient = (vec3_splat(1.0) - F) * (1.0 - metallic) * diffuseAmbient + specularAmbient;

	return ao * ambient;
}

vec3 hardShadow(vec4 shadowc, float bias)
{
	vec3 c = shadowc.xyz / shadowc.w;
	// Can not directly return the float. Don't know why.
	float factor = shadow2D(s_shadow, vec3(c.xy, c.z - bias));
	return vec3_splat(factor);
}

vec3 PCF(vec4 shadowc, float bias)
{
	vec2 c = shadowc.xy / shadowc.w;
	if (c.x > 1.0 || c.x < 0.0 || c.y > 1.0 || c.y < 0.0)
	{
		return vec3_splat(1.0);
	}

	vec3 result = vec3_splat(0.0);
	vec2 texelSize =  vec2_splat(1.0/2000.0);
	vec2 offset = texelSize * shadowc.w;

	result += hardShadow(shadowc + vec4(vec2(-1.5, -1.5) * offset, 0.0, 0.0), bias);
	result += hardShadow(shadowc + vec4(vec2(-1.5, -0.5) * offset, 0.0, 0.0), bias);
	result += hardShadow(shadowc + vec4(vec2(-1.5,  0.5) * offset, 0.0, 0.0), bias);
	result += hardShadow(shadowc + vec4(vec2(-1.5,  1.5) * offset, 0.0, 0.0), bias);

	result += hardShadow(shadowc + vec4(vec2(-0.5, -1.5) * offset, 0.0, 0.0), bias);
	result += hardShadow(shadowc + vec4(vec2(-0.5, -0.5) * offset, 0.0, 0.0), bias);
	result += hardShadow(shadowc + vec4(vec2(-0.5,  0.5) * offset, 0.0, 0.0), bias);
	result += hardShadow(shadowc + vec4(vec2(-0.5,  1.5) * offset, 0.0, 0.0), bias);

	result += hardShadow(shadowc + vec4(vec2(0.5, -1.5) * offset, 0.0, 0.0), bias);
	result += hardShadow(shadowc + vec4(vec2(0.5, -0.5) * offset, 0.0, 0.0), bias);
	result += hardShadow(shadowc + vec4(vec2(0.5,  0.5) * offset, 0.0, 0.0), bias);
	result += hardShadow(shadowc + vec4(vec2(0.5,  1.5) * offset, 0.0, 0.0), bias);

	result += hardShadow(shadowc + vec4(vec2(1.5, -1.5) * offset, 0.0, 0.0), bias);
	result += hardShadow(shadowc + vec4(vec2(1.5, -0.5) * offset, 0.0, 0.0), bias);
	result += hardShadow(shadowc + vec4(vec2(1.5,  0.5) * offset, 0.0, 0.0), bias);
	result += hardShadow(shadowc + vec4(vec2(1.5,  1.5) * offset, 0.0, 0.0), bias);

	return result / 16.0;
}

vec3 ComputeLight(vec3 pos, vec3 toEye, vec3 normal, vec4 color, vec3 aorm, vec4 shadowc)
{
	float ao = aorm.x;
	float roughness = aorm.y;
	float metallic = aorm.z;

	vec3 color_rgb = color.xyz;
	float alpha = color.w;

	vec3 result = vec3_splat(0.0);
	vec3 toLight = vec3_splat(0.0);
	vec3 lightStrength = vec3_splat(0.0);
	for (int lightIdx = 0; lightIdx < MAX_LIGHTS; ++lightIdx)
	{
		vec3 strength		= u_lights[lightIdx][0].xyz;			// D/P/S
		float falloffStart	= u_lights[lightIdx][0].w;				// P/S
		vec3 direction		= u_lights[lightIdx][1].xyz;			// D/S
		float falloffEnd	= u_lights[lightIdx][1].w;				// P/S
		vec3 position		= u_lights[lightIdx][2].xyz;			// P/S
		float spotPower		= u_lights[lightIdx][2].w;				// S
		vec4 type			= u_lights[lightIdx][3];				// (D, P, S, 0)
		if (type.x == 1.0)					// Directional light
		{
			lightStrength = strength;
			toLight = normalize(-direction);
		}
		else if (type.y == 1.0)				// Point light
		{
			float d = length(position - pos);
			toLight = normalize(position - pos);
			lightStrength = strength;
			lightStrength *= (falloffEnd - d) / (falloffEnd - falloffStart);
			lightStrength = max(lightStrength, 0.0);
		}
		else if (type.z == 1.0)				// Spot light
		{
			toLight = normalize(position - pos);
			float d = length(position - pos);
			lightStrength = strength;
			lightStrength *= (falloffEnd - d) / (falloffEnd - falloffStart);
			lightStrength *= pow(max(0.0, dot(-toLight, normalize(direction))), spotPower);
			lightStrength = max(lightStrength, 0.0);
		}
		else
		{
			break;
		}
		result += PBRDirect(lightStrength, toLight, normal, toEye, color_rgb, metallic, roughness);
		result *= PCF(shadowc, 0.0025);
	}
	result += PBRAmbient(normal, toEye, color_rgb, ao, metallic, roughness);
	return result;
}

// https://zhuanlan.zhihu.com/p/21983679
vec3 ToneMapping(vec3 color, float adapted_lum)
{
	const float A = 2.51f;
	const float B = 0.03f;
	const float C = 2.43f;
	const float D = 0.59f;
	const float E = 0.14f;
	color *= adapted_lum;
	return (color * (A * color + B)) / (color * (C * color + D) + E);
}

void main()
{
	vec4 color = u_builtinColor;
	vec3 normal = v_normal;
	vec3 aorm = u_builtinAORM.xyz;
	if (u_texOrBuiltin.x == 1.0)		// color
	{
		color = toLinear(texture2D(s_texColor, v_texc));
	}
	// If normal offset is indicated, tangent must be input too.
	if (u_texOrBuiltin.y == 1.0)		// normal
	{
		vec3 deltaNormal = texture2D(s_texNormal, v_texc).xyz * 2.0 - 1.0;
		mat3 tbn = mtxFromCols(v_tangent, v_bitangent, v_normal);
		normal += mul(deltaNormal, tbn);
		normal = normalize(normal);
	}
	if (u_texOrBuiltin.z == 1.0)		// aorm
	{
		aorm = texture2D(s_texAORM, v_texc).xyz;
	}
	vec3 fragColor = ComputeLight(v_pos, v_toeye, normal, color, aorm, v_shadowc);
	fragColor = ToneMapping(fragColor, 1.0);
	gl_FragColor.xyz = fragColor.xyz;
	gl_FragColor.w = color.w;

	gl_FragColor = toGamma(gl_FragColor);
}