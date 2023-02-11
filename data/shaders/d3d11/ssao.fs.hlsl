#define SAMPLES 6

cbuffer model_block : register(b1)
{
	float4x4 mvp;
	float4x4 p;
	float2 window_size;
	float1 radius;
};

struct pixel_input
{
	float4 out_position : SV_POSITION;
	float2 uv : FS_INPUT0;
};

struct pixel_output
{
	float4 color : SV_TARGET0;
};

Texture2D color_tex : register(t0);
SamplerState color_sampler : register(s0);
Texture2D normal_tex : register(t1);
SamplerState normal_sampler : register(s1);
Texture2D position_tex : register(t2);
SamplerState position_sampler : register(s2);

static float3 samples[128] =
{
	float3(+0.051314, -0.015931, +0.059061), float3(+0.058856, -0.043243, +0.023964), float3(-0.042369, +0.010290, +0.045510), float3(-0.025144, +0.002492, +0.088537),
	float3(+0.031263, +0.050057, +0.016310), float3(-0.071426, -0.037967, +0.010132), float3(-0.010490, -0.003026, +0.001983), float3(+0.068485, -0.038674, +0.035204),
	float3(+0.015482, -0.028035, +0.043815), float3(-0.001045, +0.076962, +0.023809), float3(+0.007463, +0.075313, +0.055837), float3(-0.044109, -0.030034, +0.082211),
	float3(-0.005916, +0.006178, +0.003616), float3(-0.020761, +0.011010, +0.030025), float3(-0.004430, -0.004879, +0.002326), float3(-0.035620, +0.064054, +0.061396),
	float3(-0.066993, +0.011416, +0.053863), float3(+0.000181, +0.002427, +0.003846), float3(-0.008302, +0.057488, +0.061957), float3(-0.020172, +0.022307, +0.029924),
	float3(+0.045633, -0.081138, +0.053453), float3(+0.094248, -0.048573, +0.032776), float3(-0.020852, +0.026012, +0.066640), float3(+0.041767, +0.095236, +0.058356),
	float3(-0.025595, +0.079326, +0.086217), float3(-0.003907, -0.063414, +0.106024), float3(-0.034215, +0.037004, +0.031127), float3(+0.011063, -0.020252, +0.036352),
	float3(-0.007112, -0.036779, +0.012591), float3(+0.056810, -0.084041, +0.085354), float3(-0.075303, -0.070956, +0.047008), float3(+0.038574, +0.034615, +0.027232),
	float3(+0.028484, -0.014978, +0.016773), float3(+0.035337, -0.106900, +0.031869), float3(-0.094494, +0.074043, +0.020702), float3(-0.059884, +0.063351, +0.003697),
	float3(-0.070532, -0.056623, +0.086893), float3(+0.035469, +0.105890, +0.072434), float3(-0.009424, -0.008464, +0.006030), float3(-0.113430, -0.077893, +0.060846),
	float3(+0.008137, +0.028366, +0.002882), float3(+0.016490, -0.009751, +0.014676), float3(+0.008889, -0.007963, +0.007772), float3(-0.025169, +0.021475, +0.015073),
	float3(-0.098648, -0.031717, +0.133685), float3(-0.041269, +0.015372, +0.084853), float3(+0.059514, -0.127154, +0.084292), float3(-0.063028, +0.158525, +0.092909),
	float3(+0.150002, +0.078031, +0.010720), float3(+0.092437, +0.117995, +0.121118), float3(+0.159343, +0.081032, +0.150120), float3(+0.180916, -0.002972, +0.088879),
	float3(+0.036324, -0.039505, +0.030338), float3(+0.022615, -0.047309, +0.061185), float3(-0.087952, -0.137957, +0.019047), float3(-0.176227, +0.096379, +0.162835),
	float3(-0.015997, +0.003353, +0.026155), float3(-0.007429, +0.024109, +0.123008), float3(-0.000341, +0.000725, +0.000453), float3(-0.023196, +0.014732, +0.062324),
	float3(+0.053365, -0.005185, +0.073282), float3(+0.005358, -0.008020, +0.007855), float3(-0.042752, +0.097407, +0.167479), float3(-0.065673, -0.135473, +0.130823),
	float3(+0.032346, -0.024061, +0.051902), float3(+0.063775, +0.027993, +0.049479), float3(+0.022269, -0.283062, +0.089409), float3(+0.051800, -0.113065, +0.015484),
	float3(+0.022118, +0.026883, +0.096119), float3(+0.049175, -0.037736, +0.029391), float3(+0.073394, -0.075221, +0.062686), float3(-0.187708, -0.038230, +0.288558),
	float3(+0.207494, +0.155951, +0.047450), float3(-0.074953, +0.176963, +0.304232), float3(+0.129588, -0.106148, +0.142089), float3(-0.112897, +0.225521, +0.253306),
	float3(-0.001903, +0.078112, +0.083421), float3(+0.304906, -0.006357, +0.193886), float3(-0.005250, -0.272697, +0.117734), float3(+0.094967, -0.150638, +0.125853),
	float3(+0.081586, +0.054109, +0.021499), float3(-0.000636, -0.163253, +0.029793), float3(-0.078600, +0.193769, +0.203794), float3(+0.102715, +0.097012, +0.075638),
	float3(-0.037131, +0.055810, +0.106975), float3(-0.202338, -0.247987, +0.031284), float3(-0.020878, -0.032409, +0.025362), float3(-0.018831, +0.112586, +0.075608),
	float3(+0.033521, +0.055277, +0.051463), float3(+0.088923, +0.029174, +0.060095), float3(+0.029081, -0.037286, +0.209654), float3(-0.189491, -0.128949, +0.270281),
	float3(+0.054797, -0.018282, +0.050721), float3(-0.062357, -0.345707, +0.273747), float3(-0.141573, -0.264405, +0.296954), float3(-0.283716, -0.032029, +0.310397),
	float3(-0.031599, -0.002853, +0.047280), float3(-0.365861, -0.106992, +0.137852), float3(+0.021521, -0.025644, +0.001793), float3(-0.026685, +0.177252, +0.218379),
	float3(-0.269304, +0.063377, +0.204559), float3(+0.246398, -0.357971, +0.394420), float3(-0.018370, -0.259034, +0.239173), float3(+0.350072, +0.283166, +0.343898),
	float3(-0.067709, -0.063181, +0.072741), float3(-0.007549, -0.113446, +0.170839), float3(-0.189932, +0.503106, +0.228925), float3(+0.178123, -0.518961, +0.144085),
	float3(+0.052793, +0.140864, +0.064576), float3(-0.469071, +0.174659, +0.085225), float3(+0.233206, +0.181320, +0.082766), float3(-0.242995, -0.086988, +0.152247),
	float3(+0.000850, -0.008940, +0.007190), float3(-0.311782, -0.306700, +0.397464), float3(-0.453877, +0.283554, +0.523227), float3(+0.095462, +0.159005, +0.179094),
	float3(+0.178149, -0.603230, +0.325405), float3(+0.126857, +0.152824, +0.277665), float3(-0.169358, +0.268901, +0.492607), float3(-0.358531, -0.482457, +0.032825),
	float3(-0.326490, +0.124330, +0.365962), float3(-0.164187, -0.162678, +0.050229), float3(+0.121592, +0.279825, +0.070889), float3(+0.092500, +0.209504, +0.180563),
	float3(+0.364151, -0.296735, +0.074312), float3(+0.102802, +0.054893, +0.141280), float3(-0.114014, +0.222741, +0.469618), float3(+0.430027, +0.778291, +0.209758)
};

static float2 noise[16] =
{
	float2( 0.685977, -0.903419), float2( 0.189102, -0.887366), float2( 0.845739, -0.637804), float2(-0.664273,  0.233258),
	float2(-0.049572,  0.967945), float2( 0.213393,  0.349980), float2(-0.544501,  0.067557), float2( 0.565815,  0.254290),
	float2(-0.251042, -0.618374), float2(-0.491521, -0.761909), float2(-0.822749,  0.752513), float2( 0.650445, -0.330653),
	float2( 0.349434,  0.612735), float2(-0.668184,  0.984954), float2( 0.860216, -0.708853), float2(-0.805433, -0.410620)
};

float get_occlusion(float3 frag_pos, row_major float3x3 tbn, int sample_id)
{
	float3 sample_pos = mul(samples[sample_id], tbn);
	float3 sample = frag_pos + sample_pos * radius;
	float4 offset = mul(float4(sample, 1), p);
	offset.xy = offset.xy / offset.w;
	offset.xy = offset.xy * .5 + .5;
	float4 pick = position_tex.Sample(position_sampler, offset.xy);
	if (pick.z < sample.z + .05)
		return 0;
	return smoothstep(0, 1, radius / abs(frag_pos.z - pick.z));
}

pixel_output main(pixel_input input)
{
	pixel_output output;
	float4 pos = color_tex.Sample(color_sampler, input.uv);
	if (pos.w <= 0)
	{
		output.color = float4(1, 1, 1, 1);
		return output;
	}
	float3 frag_pos = pos.xyz;
	float3 normal = normal_tex.Sample(normal_sampler, input.uv).xyz;
#if 1
	float2 noise_pos = (input.uv * window_size) % float2(4, 4);
	float3 random_vec = float3(noise[int(noise_pos.x + noise_pos.y * 4)].xy, 0);
#else
	float2 noise_pos = mod(input.uv * window_size, float2(100)) / float2(100);
	float3 random_vec = texture(tex_noise, noise_pos).xyz;
#endif
	float3 tangent = normalize(random_vec - normal * dot(random_vec, normal));
	float3 bitangent = cross(normal, tangent);
	row_major float3x3 tbn = {tangent, bitangent, normal};
	float1 occlusion = 0;
#if SAMPLES > 0
	occlusion += get_occlusion(frag_pos, tbn, 96);
#endif
#if SAMPLES > 1
	occlusion += get_occlusion(frag_pos, tbn, 0);
#endif
#if SAMPLES > 2
	occlusion += get_occlusion(frag_pos, tbn, 32);
	occlusion += get_occlusion(frag_pos, tbn, 64);
#endif
#if SAMPLES > 3
	occlusion += get_occlusion(frag_pos, tbn, 16);
	occlusion += get_occlusion(frag_pos, tbn, 48);
	occlusion += get_occlusion(frag_pos, tbn, 80);
	occlusion += get_occlusion(frag_pos, tbn, 112);
#endif
#if SAMPLES > 4
	occlusion += get_occlusion(frag_pos, tbn, 8);
	occlusion += get_occlusion(frag_pos, tbn, 24);
	occlusion += get_occlusion(frag_pos, tbn, 40);
	occlusion += get_occlusion(frag_pos, tbn, 56);
	occlusion += get_occlusion(frag_pos, tbn, 72);
	occlusion += get_occlusion(frag_pos, tbn, 88);
	occlusion += get_occlusion(frag_pos, tbn, 104);
	occlusion += get_occlusion(frag_pos, tbn, 120);
#endif
#if SAMPLES > 5
	occlusion += get_occlusion(frag_pos, tbn, 4);
	occlusion += get_occlusion(frag_pos, tbn, 12);
	occlusion += get_occlusion(frag_pos, tbn, 20);
	occlusion += get_occlusion(frag_pos, tbn, 28);
	occlusion += get_occlusion(frag_pos, tbn, 36);
	occlusion += get_occlusion(frag_pos, tbn, 44);
	occlusion += get_occlusion(frag_pos, tbn, 52);
	occlusion += get_occlusion(frag_pos, tbn, 60);
	occlusion += get_occlusion(frag_pos, tbn, 68);
	occlusion += get_occlusion(frag_pos, tbn, 76);
	occlusion += get_occlusion(frag_pos, tbn, 84);
	occlusion += get_occlusion(frag_pos, tbn, 92);
	occlusion += get_occlusion(frag_pos, tbn, 100);
	occlusion += get_occlusion(frag_pos, tbn, 108);
	occlusion += get_occlusion(frag_pos, tbn, 116);
	occlusion += get_occlusion(frag_pos, tbn, 124);
#endif
#if SAMPLES > 6
	occlusion += get_occlusion(frag_pos, tbn, 2);
	occlusion += get_occlusion(frag_pos, tbn, 6);
	occlusion += get_occlusion(frag_pos, tbn, 10);
	occlusion += get_occlusion(frag_pos, tbn, 14);
	occlusion += get_occlusion(frag_pos, tbn, 18);
	occlusion += get_occlusion(frag_pos, tbn, 22);
	occlusion += get_occlusion(frag_pos, tbn, 26);
	occlusion += get_occlusion(frag_pos, tbn, 30);
	occlusion += get_occlusion(frag_pos, tbn, 34);
	occlusion += get_occlusion(frag_pos, tbn, 38);
	occlusion += get_occlusion(frag_pos, tbn, 42);
	occlusion += get_occlusion(frag_pos, tbn, 46);
	occlusion += get_occlusion(frag_pos, tbn, 50);
	occlusion += get_occlusion(frag_pos, tbn, 54);
	occlusion += get_occlusion(frag_pos, tbn, 58);
	occlusion += get_occlusion(frag_pos, tbn, 62);
	occlusion += get_occlusion(frag_pos, tbn, 66);
	occlusion += get_occlusion(frag_pos, tbn, 70);
	occlusion += get_occlusion(frag_pos, tbn, 74);
	occlusion += get_occlusion(frag_pos, tbn, 78);
	occlusion += get_occlusion(frag_pos, tbn, 82);
	occlusion += get_occlusion(frag_pos, tbn, 86);
	occlusion += get_occlusion(frag_pos, tbn, 90);
	occlusion += get_occlusion(frag_pos, tbn, 94);
	occlusion += get_occlusion(frag_pos, tbn, 98);
	occlusion += get_occlusion(frag_pos, tbn, 102);
	occlusion += get_occlusion(frag_pos, tbn, 106);
	occlusion += get_occlusion(frag_pos, tbn, 110);
	occlusion += get_occlusion(frag_pos, tbn, 114);
	occlusion += get_occlusion(frag_pos, tbn, 118);
	occlusion += get_occlusion(frag_pos, tbn, 122);
	occlusion += get_occlusion(frag_pos, tbn, 126);
#endif
#if SAMPLES > 7
	occlusion += get_occlusion(frag_pos, tbn, 1);
	occlusion += get_occlusion(frag_pos, tbn, 3);
	occlusion += get_occlusion(frag_pos, tbn, 5);
	occlusion += get_occlusion(frag_pos, tbn, 7);
	occlusion += get_occlusion(frag_pos, tbn, 9);
	occlusion += get_occlusion(frag_pos, tbn, 11);
	occlusion += get_occlusion(frag_pos, tbn, 13);
	occlusion += get_occlusion(frag_pos, tbn, 15);
	occlusion += get_occlusion(frag_pos, tbn, 17);
	occlusion += get_occlusion(frag_pos, tbn, 19);
	occlusion += get_occlusion(frag_pos, tbn, 21);
	occlusion += get_occlusion(frag_pos, tbn, 23);
	occlusion += get_occlusion(frag_pos, tbn, 25);
	occlusion += get_occlusion(frag_pos, tbn, 27);
	occlusion += get_occlusion(frag_pos, tbn, 29);
	occlusion += get_occlusion(frag_pos, tbn, 31);
	occlusion += get_occlusion(frag_pos, tbn, 33);
	occlusion += get_occlusion(frag_pos, tbn, 35);
	occlusion += get_occlusion(frag_pos, tbn, 37);
	occlusion += get_occlusion(frag_pos, tbn, 39);
	occlusion += get_occlusion(frag_pos, tbn, 41);
	occlusion += get_occlusion(frag_pos, tbn, 43);
	occlusion += get_occlusion(frag_pos, tbn, 45);
	occlusion += get_occlusion(frag_pos, tbn, 47);
	occlusion += get_occlusion(frag_pos, tbn, 49);
	occlusion += get_occlusion(frag_pos, tbn, 51);
	occlusion += get_occlusion(frag_pos, tbn, 53);
	occlusion += get_occlusion(frag_pos, tbn, 55);
	occlusion += get_occlusion(frag_pos, tbn, 57);
	occlusion += get_occlusion(frag_pos, tbn, 59);
	occlusion += get_occlusion(frag_pos, tbn, 61);
	occlusion += get_occlusion(frag_pos, tbn, 63);
	occlusion += get_occlusion(frag_pos, tbn, 65);
	occlusion += get_occlusion(frag_pos, tbn, 67);
	occlusion += get_occlusion(frag_pos, tbn, 69);
	occlusion += get_occlusion(frag_pos, tbn, 71);
	occlusion += get_occlusion(frag_pos, tbn, 73);
	occlusion += get_occlusion(frag_pos, tbn, 75);
	occlusion += get_occlusion(frag_pos, tbn, 77);
	occlusion += get_occlusion(frag_pos, tbn, 79);
	occlusion += get_occlusion(frag_pos, tbn, 81);
	occlusion += get_occlusion(frag_pos, tbn, 83);
	occlusion += get_occlusion(frag_pos, tbn, 85);
	occlusion += get_occlusion(frag_pos, tbn, 87);
	occlusion += get_occlusion(frag_pos, tbn, 89);
	occlusion += get_occlusion(frag_pos, tbn, 91);
	occlusion += get_occlusion(frag_pos, tbn, 93);
	occlusion += get_occlusion(frag_pos, tbn, 95);
	occlusion += get_occlusion(frag_pos, tbn, 97);
	occlusion += get_occlusion(frag_pos, tbn, 99);
	occlusion += get_occlusion(frag_pos, tbn, 101);
	occlusion += get_occlusion(frag_pos, tbn, 103);
	occlusion += get_occlusion(frag_pos, tbn, 105);
	occlusion += get_occlusion(frag_pos, tbn, 107);
	occlusion += get_occlusion(frag_pos, tbn, 109);
	occlusion += get_occlusion(frag_pos, tbn, 111);
	occlusion += get_occlusion(frag_pos, tbn, 113);
	occlusion += get_occlusion(frag_pos, tbn, 115);
	occlusion += get_occlusion(frag_pos, tbn, 117);
	occlusion += get_occlusion(frag_pos, tbn, 119);
	occlusion += get_occlusion(frag_pos, tbn, 121);
	occlusion += get_occlusion(frag_pos, tbn, 123);
	occlusion += get_occlusion(frag_pos, tbn, 125);
	occlusion += get_occlusion(frag_pos, tbn, 127);
#endif
#if SAMPLES > 7
	occlusion /= 128;
#elif SAMPLES > 6
	occlusion /= 64;
#elif SAMPLES > 5
	occlusion /= 32;
#elif SAMPLES > 4
	occlusion /= 16;
#elif SAMPLES > 3
	occlusion /= 8;
#elif SAMPLES > 2
	occlusion /= 4;
#elif SAMPLES > 1
	occlusion /= 2;
#endif
	//occlusion = pow(occlusion, 1.1);
	//occlusion = 1.1 * (occlusion - .5) + .5;
	output.color = float4(1 - occlusion, 0, 0, 1);
	return output;
}
