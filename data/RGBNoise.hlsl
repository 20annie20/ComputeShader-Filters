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
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
	float3 pixel = readPixel(DTid.x, DTid.y);

	float2 n = frac(tan(dot(float2(DTid.x + 1, DTid.y - 4), float2(DTid.x - 1, DTid.y + 4)) / float2(DTid.x + 4, DTid.y)) * float2(DTid.x, DTid.y));

	pixel = float3(pixel.rg + 0.3 * n, pixel.b + 0.3 * n.x);

	writeToPixel(DTid.x, DTid.y, pixel);

	pixel = readPixel(DTid.x * 2, DTid.y);
	pixel = float3(pixel.rg + 0.2 * n, pixel.b + 0.2 * n.x);
	writeToPixel(DTid.x * 2, DTid.y, pixel);
}