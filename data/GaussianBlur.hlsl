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
	
	float stDevSquared = 0.01;
	float weight = 0.0;
	float sum = 0.1;
	
	for (int i = -5; i < 5; i += 1) 
	{
		for (int j = -5; j < 5; j += 1)
		{
			float offsetI = (i / (121 - 1) - 0.5) * stDevSquared;
			float offsetJ = (j / (121 - 1) - 0.5) * stDevSquared;

			float PI = 3.14159265359;
			float e = 2.71828182846;

			weight = (1 /( 2 * PI * stDevSquared)) * pow(e, -((offsetI * offsetI + offsetJ * offsetJ ) / (2 * stDevSquared)));

			pixel += readPixel(dispatchThreadID.x + i, dispatchThreadID.y + 2 * j) * weight;
			sum += weight;
		}
	}

	pixel /= sum;

	writeToPixel(dispatchThreadID.x, dispatchThreadID.y, pixel);
}
