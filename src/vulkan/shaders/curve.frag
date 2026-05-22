#version 450

layout(location = 0) in float fragEdgeDist;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 1) uniform CurveParams {
	vec4 curveColor;
};

layout(set = 0, binding = 2) uniform CurveParams2 {
	float lineWidth;
};

void main()
{
	float dist = abs(fragEdgeDist);
	float alpha = 1.0 - smoothstep(lineWidth * 0.5 - 1.0, lineWidth * 0.5 + 1.0, dist);
	outColor = vec4(curveColor.rgb, curveColor.a * alpha);
}
