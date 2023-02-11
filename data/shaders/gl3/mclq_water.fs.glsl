#version 330

in fs_block
{
	vec3 fs_position;
	vec3 fs_light_dir;
	vec3 fs_diffuse;
	vec3 fs_normal;
	float fs_alpha;
	vec2 fs_uv;
};

uniform sampler2D position_tex;
uniform sampler2D normal_tex;
uniform sampler2D color_tex;
uniform sampler2D tex;

layout(std140) uniform model_block
{
	mat4 p;
	mat4 v;
	mat4 mv;
	mat4 mvp;
};

layout(std140) uniform scene_block
{
	vec4 light_direction;
	vec4 diffuse_color;
	vec4 specular_color;
	vec4 base_color;
	vec4 final_color;
	vec4 fog_color;
	vec2 fog_range;
	vec2 alphas;
};

layout(location=0) out vec4 fragcolor;
layout(location=1) out vec4 fragnormal;
layout(location=2) out vec4 fragposition;

#if 0
vec3 get_reflection_color()
{
  float maxDistance = 15;
  float resolution  = 0.3;
  int   steps       = 10;
  float thickness   = 0.5;

  vec2 texSize  = textureSize(position_tex, 0).xy;
  vec2 texCoord = gl_FragCoord.xy / texSize;

  vec4 uvR = vec4(0.0);

  vec4 positionFrom = texture(position_tex, texCoord);

  if (positionFrom.w <= 0.0)
  	return vec3(0);

  vec3 unitPositionFrom = normalize(positionFrom.xyz);
  vec3 normal           = (v * vec4(0, 1, 0, 0)).xyz;//normalize(texture(normal_tex, texCoord).xyz);
  vec3 pivot            = normalize(reflect(unitPositionFrom, normal));

  vec4 positionTo = positionFrom;

  vec4 startView = vec4(positionFrom.xyz + (pivot *         0.0), 1.0);
  vec4 endView   = vec4(positionFrom.xyz + (pivot * maxDistance), 1.0);

  vec4 startFrag      = startView;
       startFrag      = p * startFrag;
       startFrag.xyz /= startFrag.w;
       startFrag.xy   = startFrag.xy * 0.5 + 0.5;
       startFrag.xy  *= texSize;

  vec4 endFrag      = endView;
       endFrag      = p * endFrag;
       endFrag.xyz /= endFrag.w;
       endFrag.xy   = endFrag.xy * 0.5 + 0.5;
       endFrag.xy  *= texSize;

  vec2 frag  = startFrag.xy;
       uvR.xy = frag / texSize;

  float deltaX    = endFrag.x - startFrag.x;
  float deltaY    = endFrag.y - startFrag.y;
  float useX      = abs(deltaX) >= abs(deltaY) ? 1.0 : 0.0;
  float delta     = clamp(mix(abs(deltaY), abs(deltaX), useX) * clamp(resolution, 0.0, 1.0), 0, 100);
  vec2  increment = vec2(deltaX, deltaY) / max(delta, 0.001);

  float search0 = 0;
  float search1 = 0;

  int hit0 = 0;
  int hit1 = 0;

  float viewDistance = startView.z;
  float depth        = thickness;

  float i = 0;

  for (i = 0; i < int(delta); ++i) {
    frag      += increment;
    uvR.xy      = frag / texSize;
    positionTo = texture(position_tex, uvR.xy);

    search1 =
      mix
        ( (frag.y - startFrag.y) / deltaY
        , (frag.x - startFrag.x) / deltaX
        , useX
        );

    search1 = clamp(search1, 0.0, 1.0);

    viewDistance = (startView.z * endView.z) / mix(endView.z, startView.z, search1);
    depth        = viewDistance - positionTo.z;

    if (depth > 0 && depth < thickness) {
      hit0 = 1;
      break;
    } else {
      search0 = search1;
    }
  }

  search1 = search0 + ((search1 - search0) / 2.0);

  steps *= hit0;

  for (i = 0; i < steps; ++i) {
    frag       = mix(startFrag.xy, endFrag.xy, search1);
    uvR.xy      = frag / texSize;
    positionTo = texture(position_tex, uvR.xy);

    viewDistance = (startView.z * endView.z) / mix(endView.z, startView.z, search1);
    depth        = viewDistance - positionTo.z;

    if (depth > 0 && depth < thickness) {
      hit1 = 1;
      search1 = search0 + ((search1 - search0) / 2);
    } else {
      float temp = search1;
      search1 = search1 + ((search1 - search0) / 2);
      search0 = temp;
    }
  }

  float visibility =
      hit1
    * positionTo.w
    * ( 1
      - max
         ( dot(-unitPositionFrom, pivot)
         , 0
         )
      )
    * ( 1
      - clamp
          ( depth / thickness
          , 0
          , 1
          )
      )
    * ( 1
      - clamp
          (   length(positionTo - positionFrom)
            / maxDistance
          , 0
          , 1
          )
      )
    * (uvR.x < 0 || uvR.x > 1 ? 0 : 1)
    * (uvR.y < 0 || uvR.y > 1 ? 0 : 1);

  visibility = clamp(visibility, 0, 1);

  uvR.ba = vec2(visibility);
  return texture(color_tex, uvR.ba).xyz;*/
}
#endif

uniform int binarySearchCount = 1;
uniform int rayMarchCount = 50;
uniform float rayStep = 0.1; // 0.025
uniform float LLimiter = 0.001;
uniform float minRayStep = 10;

#define getPosition(texCoord) texture(position_tex, texCoord).xyz

vec2 binarySearch(inout vec3 dir, inout vec3 hitCoord, inout float dDepth)
{
	float depth;

	vec4 projectedCoord;

	for (int i = 0; i < binarySearchCount; i++)
	{
		projectedCoord = p * vec4(hitCoord, 1.0);
		projectedCoord.xy /= projectedCoord.w;
		projectedCoord.xy = projectedCoord.xy * 0.5 + 0.5;

		depth = getPosition(projectedCoord.xy).z;

		dDepth = hitCoord.z - depth;

		dir *= 0.5;
		if (dDepth > 0.0)
			hitCoord += dir;
		else
			hitCoord -= dir;
	}

	projectedCoord = p * vec4(hitCoord, 1.0);
	projectedCoord.xy /= projectedCoord.w;
	projectedCoord.xy = projectedCoord.xy * 0.5 + 0.5;

	return vec2(projectedCoord.xy);
}

vec2 rayCast(vec3 dir, inout vec3 hitCoord, out float dDepth)
{
	dir *= rayStep;

	for (int i = 0; i < rayMarchCount; i++)
	{
		hitCoord += dir;

		vec4 projectedCoord = p * vec4(hitCoord, 1.0);
		projectedCoord.xy /= projectedCoord.w;
		projectedCoord.xy = projectedCoord.xy * 0.5 + 0.5;

		if (projectedCoord.y < 0 || projectedCoord.y > 1
		 || projectedCoord.x < 0 || projectedCoord.x > 1)
			return vec2(-1.0);
		float depth = getPosition(projectedCoord.xy).z;
		dDepth = hitCoord.z - depth;

		if ((dir.z - dDepth) < 1.2 && dDepth <= 0.0)
			return binarySearch(dir, hitCoord, dDepth);
	}

	return vec2(-1.0);
}

#define scale vec3(.8, .8, .8)
#define k 19.19

vec3 hash(vec3 a)
{
	a = fract(a * scale);
	a += dot(a, a.yxz + k);
	return fract((a.xxy + a.yxx)*a.zyx);
}

vec4 get_reflection_color()
{
	vec2 texSize  = textureSize(position_tex, 0).xy;
	vec2 texCoord = gl_FragCoord.xy / texSize;

	//float r = 0;//(uv.x + uv.y) * 3.1415 + sin(uv.x * 3.1415) * 2;
	//float c = abs(cos(r)) * .1;
	//float s = abs(sin(r)) * .1 + .9;
	//vec3 normal = (v * vec4(c, s, 0, 0)).xyz;//texture(normal_tex, texCoord).xyz;

	vec3 worldPos = vec3(vec4(fs_position, 1.0) * inverse(v));
	vec3 jitt = hash(worldPos);

	// Reflection vector
	vec3 reflected = normalize(reflect(normalize(fs_position), normalize(fs_normal)));

	// Ray cast
	vec3 hitPos = fs_position;
	float dDepth; 
	vec2 coords = rayCast(jitt + reflected * max(-fs_position.z, minRayStep), hitPos, dDepth);
	if (coords == vec2(-1.0))
		return vec4(0);

	float L = length(getPosition(coords) - fs_position);
	L = clamp(L * LLimiter, 0, 1);
	float error = 1 - L;

	
	if (texture(position_tex, coords.xy).w <= 0)
		return vec4(0);

	vec4 color = texture(color_tex, coords.xy);
	return vec4(color.rgb, error);
}

float fresnel(vec3 direction, vec3 normal)
{
	vec3 halfDirection = normalize(normal + direction);

	float cosine = dot(halfDirection, direction);
	float product = max(cosine, 0.0);
	float factor = 1.0 - pow(product, 5);

	return factor;
}

vec3 lettier_reflection(vec4 positionFrom, vec3 normal)
{
	float maxDistance = 100;
  float resolution  = 0.3;
  int   steps       = 20;
  float thickness   = 0.5;

  vec2 texSize  = textureSize(position_tex, 0).xy;
  vec2 texCoord = gl_FragCoord.xy / texSize;

  vec4 uvv = vec4(0.0);

 // vec4 positionFrom = texture(position_tex, texCoord);

  vec3 unitPositionFrom = normalize(positionFrom.xyz);
 // vec3 normal           = normalize(texture(normal_tex, texCoord).xyz);
  vec3 pivot            = normalize(reflect(unitPositionFrom, normal));

  vec4 positionTo = positionFrom;

  vec4 startView = vec4(positionFrom.xyz + (pivot *         0.0), 1.0);
  vec4 endView   = vec4(positionFrom.xyz + (pivot * maxDistance), 1.0);

  vec4 startFrag      = startView;
       startFrag      = p * startFrag;
       startFrag.xyz /= startFrag.w;
       startFrag.xy   = startFrag.xy * 0.5 + 0.5;
       startFrag.xy  *= texSize;

  vec4 endFrag      = endView;
       endFrag      = p * endFrag;
       endFrag.xyz /= endFrag.w;
       endFrag.xy   = endFrag.xy * 0.5 + 0.5;
       endFrag.xy  *= texSize;

  vec2 frag  = startFrag.xy;
       uvv.xy = frag / texSize;

  float deltaX    = endFrag.x - startFrag.x;
  float deltaY    = endFrag.y - startFrag.y;
  float useX      = abs(deltaX) >= abs(deltaY) ? 1.0 : 0.0;
  float delta     = mix(abs(deltaY), abs(deltaX), useX) * clamp(resolution, 0.0, 1.0);
  vec2  increment = vec2(deltaX, deltaY) / max(delta, 0.001);

  float search0 = 0;
  float search1 = 0;

  int hit0 = 0;
  int hit1 = 0;

  float viewDistance = startView.y;
  float depth        = thickness;

  float i = 0;

  for (i = 0; i < int(delta); ++i) {
    frag      += increment;
    uvv.xy      = frag / texSize;
    positionTo = texture(position_tex, uvv.xy);

    search1 =
      mix
        ( (frag.y - startFrag.y) / deltaY
        , (frag.x - startFrag.x) / deltaX
        , useX
        );

    search1 = clamp(search1, 0.0, 1.0);

    viewDistance = (startView.y * endView.y) / mix(endView.y, startView.y, search1);
    depth        = viewDistance - positionTo.y;

    if (depth > 0 && depth < thickness) {
      hit0 = 1;
      break;
    } else {
      search0 = search1;
    }
  }

  search1 = search0 + ((search1 - search0) / 2.0);

  steps *= hit0;

  for (i = 0; i < steps; ++i) {
    frag       = mix(startFrag.xy, endFrag.xy, search1);
    uvv.xy      = frag / texSize;
    positionTo = texture(position_tex, uvv.xy);

    viewDistance = (startView.y * endView.y) / mix(endView.y, startView.y, search1);
    depth        = viewDistance - positionTo.y;

    if (depth > 0 && depth < thickness) {
      hit1 = 1;
      search1 = search0 + ((search1 - search0) / 2);
    } else {
      float temp = search1;
      search1 = search1 + ((search1 - search0) / 2);
      search0 = temp;
    }
  }

  float visibility =
      hit1
    * positionTo.w
    * ( 1
      - max
         ( dot(-unitPositionFrom, pivot)
         , 0
         )
      )
    * ( 1
      - clamp
          ( depth / thickness
          , 0
          , 1
          )
      )
    * ( 1
      - clamp
          (   length(positionTo - positionFrom)
            / maxDistance
          , 0
          , 1
          )
      )
    * (uvv.x < 0 || uvv.x > 1 ? 0 : 1)
    * (uvv.y < 0 || uvv.y > 1 ? 0 : 1);

  visibility = clamp(visibility, 0, 1);

  return texture(color_tex, uvv.xy).xyz * visibility;
}

void main()
{
	vec3 tex_col = texture(tex, fs_uv).rgb;
	float specular_factor = pow(clamp(dot(normalize(-fs_position), normalize(reflect(-fs_light_dir, fs_normal))), 0, 1), 10) * 20;
	vec3 color = fs_diffuse + tex_col * (1 + specular_color.xyz * specular_factor);
	float fog_factor = clamp((length(fs_position) - fog_range.x) / (fog_range.y - fog_range.x), 0, 1);
	//vec3 reflected = normalize(reflect(normalize(fs_position), normalize(fs_normal)));
	//float fresnel = fresnel(reflected, fs_normal);
	//vec4 reflection_color = vec4(lettier_reflection(vec4(fs_position, 1), fs_normal), 1);//get_reflection_color();

	//fragcolor = vec4(reflection_color.rgb, 1);
	//fragcolor = vec4(reflection_color.rgb * reflection_color.a, .3 + .7 * fresnel);
	//fragcolor = vec4(reflection_color.rgb * reflection_color.a, 1);
	//fragcolor = vec4(vec3(reflection_color.a), 1);
	//fragcolor = vec4(mix(color, reflection_color.rgb, .1 * reflection_color.a), .8 + fresnel);
	//fragcolor = vec4(mix(mix(color, reflection_color.rgb, .2 * reflection_color.a * fresnel), fog_color.xyz, fog_factor), fs_alpha);
	fragcolor = vec4(mix(color, fog_color.xyz, fog_factor), fs_alpha);
	fragnormal = vec4(0, 1, 0, 0);
	fragposition = vec4(fs_position, 1);
	//float v = (uv.x + uv.y) * 3.1415 + sin(uv.x * 3.1415 * 2);
	//float c = abs(cos(v)) * .2;
	//float s = abs(sin(v)) * .2 + .8;
	//fragcolor = vec4(c, s, 0, 1);
}
