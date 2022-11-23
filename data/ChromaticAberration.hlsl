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
	float3 pixel = readPixel(dispatchThreadID.x, dispatchThreadID.y);

	float3 pixelR = readPixel(dispatchThreadID.x + 7, dispatchThreadID.y);
	float3 pixelG = readPixel(dispatchThreadID.x - 7, dispatchThreadID.y);
	float3 pixelB = readPixel(dispatchThreadID.x, dispatchThreadID.y + 2 * 7);

	float3 col = (0, 0, 0);

	col.r = pixelR.r;
	col.g = pixelG.g;
	col.b = pixelB.b;

	col *= 0.5;

	writeToPixel(dispatchThreadID.x, dispatchThreadID.y, 0.5*pixel+col);
}