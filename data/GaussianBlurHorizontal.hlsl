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
	
	float3 pixel;
	float stDevSquared = 0.3;
	float weight;
	float sum = 0;

	for (int i = 0; i < 30; i += 1) {
		float offset = (i / (30 - 1) - 0.5) * 0.3;
		weight = (1 / sqrt( 2 * 3.14159265359 * stDevSquared)) * pow(2.71828182846, -((offset * offset) / (2 * stDevSquared)));
		pixel += readPixel(dispatchThreadID.x + i, dispatchThreadID.y) * weight;
		sum += weight;
	}

	pixel /= sum;

	writeToPixel(dispatchThreadID.x, dispatchThreadID.y, pixel);
}
