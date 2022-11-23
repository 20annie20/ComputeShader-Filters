struct Pixel
{
	int colour;
};

StructuredBuffer<Pixel> Buffer0 : register(t0);
RWStructuredBuffer<Pixel> BufferOut : register(u0);

#define SIGMA 10.0
#define BSIGMA 0.1
#define MSIZE 15

float normpdf(in float x, in float sigma)
{
	return 0.39894 * exp(-0.5 * x * x / (sigma * sigma)) / sigma;
}

float normpdf3(in float3 v, in float sigma)
{
	return 0.39894 * exp(-0.5 * dot(v, v) / (sigma * sigma)) / sigma;
}

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
	 
	const int kSize = (MSIZE - 1) / 2;
	float kernel[MSIZE];
	float3 final_colour = float3(0.0, 0.0, 0.0);

	float Z = 0.0;
	
	for (int j = 0; j <= kSize; ++j)
	{
		kernel[kSize + j] = kernel[kSize - j] = normpdf(float(j), SIGMA);
	}
	
	float3 cc;
	float factor;
	float bZ = 1.0 / normpdf(0.0, BSIGMA);
	
	for (int i = -kSize; i <= kSize; ++i)
	{
		for (int j = -kSize; j <= kSize; ++j)
		{
			cc = readPixel(dispatchThreadID.x + i, dispatchThreadID.y + 2 * j );

			factor = normpdf3(cc - pixel, BSIGMA) * bZ * kernel[kSize + j] * kernel[kSize + i];
			Z += factor;
			final_colour += factor * cc;

		}
	}

	pixel = float3(final_colour / Z);

	writeToPixel(dispatchThreadID.x, dispatchThreadID.y, pixel);
}