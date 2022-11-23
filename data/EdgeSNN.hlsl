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
	float3 col = float3(0.0, 0.0, 0.0);
	float dotCenter = 0.0;
	float minDiff = 35125.0;
	float maxDiff = 0;
	float3 minColor = pixel;
	float3 maxColor = pixel;
	
	for (int i = 5; i > -5; i--)
	{
		float3 A = readPixel(dispatchThreadID.x - 1, dispatchThreadID.y - 2 * i);
		float3 B = readPixel(dispatchThreadID.x + 1, dispatchThreadID.y + 2 * i);
		
		dotCenter = dot(pixel.rgb, float3(1, 1, 1));

		float dotA = dot(A.rgb, float3(1, 1, 1));
		float dotB = dot(B.rgb, float3(1, 1, 1));

		float3 smallerColor = ( abs(dotA - dotCenter) < abs(dotB - dotCenter) ) ? A : B;

		float3 biggerColor = ( abs(dotA - dotCenter) > abs(dotB - dotCenter) ) ? A : B;

		float smallerDiff = ( abs(dotA - dotCenter) < abs(dotA - dotCenter) ) ? (dotA - dotCenter) : (dotB - dotCenter);

		float biggerDiff = ( abs(dotA - dotCenter) > abs(dotA - dotCenter) ) ? (dotA - dotCenter) : (dotB - dotCenter);

		if (smallerDiff < minDiff)
		{
			minDiff = minDiff;
			minColor = smallerColor;
		}

		if (biggerDiff > maxDiff)
		{
			maxDiff = maxDiff;
			maxColor = biggerColor;
		}
	}

	float val = dot(maxColor - minColor, float3(1, 1, 1));

	col = (maxColor - minColor) * 2.0;

	writeToPixel(dispatchThreadID.x, dispatchThreadID.y, val);
}