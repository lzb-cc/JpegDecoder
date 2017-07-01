
#include <stdio.h>
#include "JpegDecoder.h"

#pragma comment(lib, "ImageShow.lib")

extern "C" __declspec(dllimport) void ShowImage(unsigned char *data, int rows, int cols);

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("Usage: main <parameters> \n");
        return 0;
    }

	JpegDecoder decoder(argv[1]);
    BitmapImage &img = decoder.Decoder();

    ShowImage(img.Data, img.Height, img.Width);
	return 0;
}
