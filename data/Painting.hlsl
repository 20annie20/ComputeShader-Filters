struct Pixel
{
	int colour;
};

StructuredBuffer<Pixel> Buffer0 : register(t0);
RWStructuredBuffer<Pixel> BufferOut : register(u0);

float3 readPixel(int x, int y)
{
	float3 output;
	uint index = (x + y * 512);

	output.x = (float)(((Buffer0[index].colour) & 0x000000ff)) / 255.0f;
	output.y = (float)(((Buffer0[index].colour) & 0x0000ff00) >> 8) / 255.0f;
	output.z = (float)(((Buffer0[index].colour) & 0x00ff0000) >> 16) / 255.0f;

	return output;
}

void writeToPixel(int x, int y, float3 colour)
{
	uint index = (x + y * 512);

	int ired = (int)(clamp(colour.r, 0, 1) * 255);
	int igreen = (int)(clamp(colour.g, 0, 1) * 255) << 8;
	int iblue = (int)(clamp(colour.b, 0, 1) * 255) << 16;

	BufferOut[index].colour = ired + igreen + iblue;
}

[numthreads(16, 16, 1)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
	int _Radius = 10;
	float3 pixel = readPixel(dispatchThreadID.x, dispatchThreadID.y);

	float3 mean[4] = {
	 { 0, 0, 0 },
	 { 0, 0, 0 },
	 { 0, 0, 0 },
	 { 0, 0, 0 }
	};

	float3 sigma[4] = {
	{ 0, 0, 0 },
	{ 0, 0, 0 },
	{ 0, 0, 0 },
	{ 0, 0, 0 }
	};

	float2 start[4] = { { -_Radius * .5, -_Radius * .5},{ -_Radius * .5, 0 },{ 0, -_Radius * .5},{ 0, 0 } };

	float2 pos;
	float3 col;

	for (int k = 0; k < 4; k++) {
		for (int i = 0; i <= _Radius; i++) {
			for (int j = 0; j <= _Radius; j++) {
				pos = float2(i, j) + start[k];
				col = readPixel(dispatchThreadID.x + pos.x, dispatchThreadID.y + 2 * pos.y);
				mean[k] += col;
				sigma[k] += col * col;
			}
		}
	}

	float sigma2;

	float n = pow(_Radius + 1, 2);
	float min = 1;

	for (int l = 0; l < 4; l++) {
		mean[l] /= n;
		sigma[l] = abs(sigma[l] / n - mean[l] * mean[l]);
		sigma2 = sigma[l].r + sigma[l].g + sigma[l].b;

		if (sigma2 < min) {
			min = sigma2;
			pixel.rgb = mean[l].rgb;
		}
	}

	writeToPixel(dispatchThreadID.x, dispatchThreadID.y, pixel);
}