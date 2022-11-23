struct Pixel
{
	int colour;
};

StructuredBuffer<Pixel> Buffer0 : register(t0);
RWStructuredBuffer<Pixel> BufferOut : register(u0);

float4 readPixel(int x, int y)
{
	float4 output;
	uint index = (x + y * 512);

	output.x = (float)(((Buffer0[index].colour) & 0x000000ff)) / 255.0f;
	output.y = (float)(((Buffer0[index].colour) & 0x0000ff00) >> 8) / 255.0f;
	output.z = (float)(((Buffer0[index].colour) & 0x00ff0000) >> 16) / 255.0f;
	output.w = (float)(((Buffer0[index].colour) & 0xff000000) >> 24) / 255.0f;

	return output;
}

float4 readOutputPixel(int x, int y)
{
	float4 output;
	uint index = (x + y * 512);

	output.x = (float)(((BufferOut[index].colour) & 0x000000ff)) / 255.0f;
	output.y = (float)(((BufferOut[index].colour) & 0x0000ff00) >> 8) / 255.0f;
	output.z = (float)(((BufferOut[index].colour) & 0x00ff0000) >> 16) / 255.0f;
	output.w = (float)(((BufferOut[index].colour) & 0xff000000) >> 24) / 255.0f;

	return output;
}

void writeToPixel(int x, int y, float4 colour)
{
	uint index = (x + y * 512);

	int ired = (int)(clamp(colour.r, 0, 1) * 255);
	int igreen = (int)(clamp(colour.g, 0, 1) * 255) << 8;
	int iblue = (int)(clamp(colour.b, 0, 1) * 255) << 16;
	int ialpha = (int)(clamp(colour.a, 0, 1) * 255) << 24;

	BufferOut[index].colour = ired + igreen + iblue + ialpha;
}


[numthreads(16, 16, 1)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
	int x = dispatchThreadID.x;
	int y = dispatchThreadID.y;

	float4 pixel = readPixel(dispatchThreadID.x, dispatchThreadID.y);

	if (dot(pixel.rgb, float3(1, 1, 1)) > 2.5)
	{
		for (int k = 0; k < 15; k += 1)
		{
			int clampedX = clamp(x, 0, 512);
			int clampedY = clamp(y, 0, 1024);

			writeToPixel(clampedX, clampedY, float4(1.0, 1.0, 1.0, 1.0));
			writeToPixel(clampedX + k, clampedY + k, float4(1.0, 1.0, 1.0, 1.0));
			writeToPixel(clampedX - k, clampedY - k, float4(1.0, 1.0, 1.0, 1.0));
			writeToPixel(clampedX + k, clampedY, float4(1.0, 1.0, 1.0, 1.0));
			writeToPixel(clampedX - k, clampedY, float4(1.0, 1.0, 1.0, 1.0));
			writeToPixel(clampedX, clampedY - k, float4(1.0, 1.0, 1.0, 1.0));
			writeToPixel(clampedX, clampedY + k, float4(1.0, 1.0, 1.0, 1.0));
		}
	}
}