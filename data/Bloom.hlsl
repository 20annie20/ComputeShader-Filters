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
	float4 origPixel = pixel;

	
	if (dot(pixel.rgb, float3(1, 1, 1)) > 1.7)
	{
		float stDevSquared = 0.01;
		float weight = 0.0;
		float sum = 0.1;

		for (int i = -7; i < 7; i += 1)
		{
			for (int j = -7; j < 7; j += 1)
			{
				float offsetI = (i / (225 - 1) - 0.5) * stDevSquared;
				float offsetJ = (j / (225 - 1) - 0.5) * stDevSquared;

				weight = (1 / (2 * 3.14159265359 * stDevSquared)) * pow(2.71828182846, -((offsetI * offsetI + offsetJ * offsetJ) / (2 * stDevSquared)));

				pixel += readPixel(dispatchThreadID.x + i, dispatchThreadID.y + 2 * j) * weight;
				sum += weight;
			}
		}

		pixel /= (sum - 3.0);
		writeToPixel(dispatchThreadID.x, dispatchThreadID.y, lerp(origPixel, pixel, 1.0));
	}	

	else
	{
		writeToPixel(dispatchThreadID.x, dispatchThreadID.y, pixel);
	}
}