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

[numthreads(2, 2, 1)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{	
	int blockSize = 32;
	float3 pixel = readPixel(dispatchThreadID.x * blockSize, dispatchThreadID.y * blockSize / 2);
	int numPixels = blockSize * blockSize;
	
	
	for (int i = 0; i < blockSize; i += 1)
	{
		for (int j = 0; j < blockSize; j += 1)
		{
			pixel += readPixel(dispatchThreadID.x * blockSize + i, dispatchThreadID.y * blockSize + j * 2);
		}
	}

	pixel /= numPixels;

	for (int k = 0; k < blockSize; k += 1)
	{
		for (int z = 0; z < blockSize; z += 1)
		{
			writeToPixel(dispatchThreadID.x * blockSize + k, dispatchThreadID.y * blockSize + z * 2, pixel);
		}
	}
}
