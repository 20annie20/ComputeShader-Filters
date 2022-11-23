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
	float3 LuminanceConv = ( 0.2125f, 0.7154f, 0.0721f );

	float3 x = readPixel(dispatchThreadID.x - 1, dispatchThreadID.y - 2);
	x += 2 * (readPixel(dispatchThreadID.x - 1, dispatchThreadID.y));
	x += readPixel(dispatchThreadID.x - 1, dispatchThreadID.y + 2);
	x -= readPixel(dispatchThreadID.x + 1, dispatchThreadID.y - 2);
	x -= 2 * (readPixel(dispatchThreadID.x + 1, dispatchThreadID.y));
	x -= readPixel(dispatchThreadID.x + 1, dispatchThreadID.y + 2);

	float3 y = readPixel(dispatchThreadID.x - 1, dispatchThreadID.y - 2);
	y += 2 * (readPixel(dispatchThreadID.x, dispatchThreadID.y - 2));
	y += readPixel(dispatchThreadID.x + 1, dispatchThreadID.y - 2);
	y -= readPixel(dispatchThreadID.x - 1, dispatchThreadID.y + 2);
	y -= 2 * (readPixel(dispatchThreadID.x, dispatchThreadID.y + 2));
	y -= readPixel(dispatchThreadID.x + 1, dispatchThreadID.y + 2);

	float lumX = dot(x, LuminanceConv); //omit this & go sqrt(x*x + y * y) for an interesting effect :D
	float lumY = dot(y, LuminanceConv);

	float3 pixel = sqrt(lumX * lumX + lumY * lumY) * 3.0;

	writeToPixel(dispatchThreadID.x, dispatchThreadID.y, pixel);
}
