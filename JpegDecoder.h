
#define _CRT_SECURE_NO_WARNINGS

#ifndef JPEGDECODER_H
#define JPEGDECODER_H

#include <stdio.h>
#include <map>
#include <string>
#include <math.h>

#ifndef DEFUINT8
#define DEFUNIT8
typedef unsigned char uint8_t;
#endif

/* @brief 反 Zig-Zag 编码表
 */
 /*
 static const int UnZigZagTable[64] =
 {
	  0,  1,  8, 16,  9,  2,  3, 10,
	 17, 24, 32, 25, 18, 11,  4,  5,
	 12, 19, 26, 33, 40, 48, 41, 34,
	 27, 20, 13,  6,  7, 14, 21, 28,
	 35, 42, 49, 56, 57, 50, 43, 36,
	 29, 22, 15, 23, 30, 37, 44, 51,
	 58, 59, 52, 45, 38, 31, 39, 46,
	 53, 60, 61, 54, 47, 55, 62, 63
 };
 */

static int const UnZigZagTable[64] =
{
	 0,  1,  5,  6, 14, 15, 27, 28,
	 2,  4,  7, 13, 16, 26, 29, 42,
	 3,  8, 12, 17, 25, 30, 41, 43,
	 9, 11, 18, 24, 31, 40, 44, 53,
	10, 19, 23, 32, 39, 45, 52, 54,
	20, 22, 33, 38, 46, 41, 55, 60,
	21, 34, 37, 47, 50, 56, 59, 61,
	35, 36, 48, 49, 57, 58, 62, 63
};

/* @brief 离散余弦变换系数矩阵
 */
static const double MtxDCT[8][8] =
{
	{0.3536,    0.3536,    0.3536,    0.3536,    0.3536,    0.3536,    0.3536,    0.3536},
	{0.4904,    0.4157,    0.2778,    0.0975,   -0.0975,   -0.2778,   -0.4157,   -0.4904},
	{0.4619,    0.1913,   -0.1913,   -0.4619,   -0.4619,   -0.1913,    0.1913,    0.4619},
	{0.4157,   -0.0975,   -0.4904,   -0.2778,    0.2778,    0.4904,    0.0975,   -0.4157},
	{0.3536,   -0.3536,   -0.3536,    0.3536,    0.3536,   -0.3536,   -0.3536,    0.3536},
	{0.2778,   -0.4904,    0.0975,    0.4157,   -0.4157,   -0.0975,    0.4904,   -0.2778},
	{0.1913,   -0.4619,    0.4619,   -0.1913,   -0.1913,    0.4619,   -0.4619,    0.1913},
	{0.0975,   -0.2778,    0.4157,   -0.4904,    0.4904,   -0.4157,    0.2778,   -0.0975}
};

/* @brief 反离散余弦变换系数矩阵
 */
static const double MtxIDCT[8][8] =
{
	{0.3536,    0.4904,    0.4619,    0.4157,    0.3536,    0.2778,    0.1913,    0.0975},
	{0.3536,    0.4157,    0.1913,   -0.0975,   -0.3536,   -0.4904,   -0.4619,   -0.2778},
	{0.3536,    0.2778,   -0.1913,   -0.4904,   -0.3536,    0.0975,    0.4619,    0.4157},
	{0.3536,    0.0975,   -0.4619,   -0.2778,    0.3536,    0.4157,   -0.1913,   -0.4904},
	{0.3536,   -0.0975,   -0.4619,    0.2778,    0.3536,   -0.4157,   -0.1913,    0.4904},
	{0.3536,   -0.2778,   -0.1913,    0.4904,   -0.3536,   -0.0975,    0.4619,   -0.4157},
	{0.3536,   -0.4157,    0.1913,    0.0975,   -0.3536,    0.4904,   -0.4619,    0.2778},
	{0.3536,   -0.4904,    0.4619,   -0.4157,    0.3536,   -0.2778,    0.1913,   -0.0975}
};


/* Matrix */
class Mtx
{
public:
	int *operator[] (int row)
	{
		return data[row];
	}
private:
	int data[8][8];
};


class Pixel
{
public:
	int R, G, B;
};

/*  Y : Cr : Cb  =  4 : 1 : 1 */
class MCU
{
public:
	Mtx    yMtx[4];
	Mtx    crMtx;
	Mtx    cbMtx;
};

class BitmapImage
{
public:
	BitmapImage() : Data(NULL), Height(0), Width(0) {}
	~BitmapImage()
	{
		if (Data != NULL)
			delete[]Data;
	}

	void CreateImage()
	{
		Data = new uint8_t[Height * Width * 3];
	}

	uint8_t *Data;  // image data
	int		Height; // rows of image
	int		Width;  // cols of image
};

class HuffmanTable
{
public:
	std::map<std::string, uint8_t> &operator[] (int index)
	{
		switch (index)
		{
		case 0:  return DC[0]; break; /* 00 */
		case 1:  return DC[1]; break; /* 01 */
		case 16: return AC[0]; break; /* 10 */
		case 17: return AC[1]; break; /* 11 */
		default: return DC[0]; break;
		}
	}
private:
	std::map<std::string, uint8_t> DC[2];
	std::map<std::string, uint8_t> AC[2];
};




class JpegDecoder
{
public:
	JpegDecoder(const char *fileName) : endOfDecoder(false), readCount(0x80), dcY(0), dcCr(0), dcCb(0), idxOfBlock(0)
	{
		fp = fopen(fileName, "rb+");
	}

	~JpegDecoder()
	{
		fclose(fp);
	}


	BitmapImage &Decoder();


protected:
	/* @brief Decoder a matrix of mcu
	   @params:
			block: 返回值
			table: huffman table type ( 00, 01, 10, 11 )
			quant: 量化表
	*/
	void DecoderMtx(Mtx &block, int table, uint8_t *quant, int &dc);

	/*  计算下一个MCU */
	void DecoderNextMCU();

	/* 读取哈夫曼表 */
	void DecoderHuffman()
	{
		int offset = 181;
		fseek(fp, offset, SEEK_SET);
		DecoderTable(); 		/* DC-0 */

		offset = 4;
		fseek(fp, offset, SEEK_CUR);
		DecoderTable();			/* DC-1 */

		fseek(fp, offset, SEEK_CUR);
		DecoderTable();			/* AC-0 */

		fseek(fp, offset, SEEK_CUR);
		DecoderTable();			/* AC-1 */
	}

	/* 解码 一个哈夫曼表 */
	void DecoderTable();

	/* 读取量化表 */
	void DecoderQuant()
	{
		/* read Quant Table of Y */
		int offset = 25;
		fseek(fp, offset, SEEK_SET);
		fread(quantY, 1, 64, fp);

		/* read Quant Table of C */
		offset = 5;
		fseek(fp, offset, SEEK_CUR);
		fread(quantC, 1, 64, fp);
	}

	/* 读取图像 宽高 */
	void DecoderSize()
	{
		uint8_t buf[2];
		/* read height */
		int offset = 163;
		fseek(fp, offset, SEEK_SET);
		fread(buf, 1, 2, fp);
		img.Height = (buf[0] << 8) + buf[1];

		/* read width */
		fread(buf, 1, 2, fp);
		img.Width = (buf[0] << 8) + buf[1];

		/* create image */
		img.CreateImage();

		/* compute number of MCU block */
		xNumberOfBlock = (img.Height + 15) / 16;
		yNumberOfBlock = (img.Width + 15) / 16;
	}


	/* 计算矩阵乘积 */
	Mtx MtxMulI2D(Mtx &left, const double right[8][8])
	{
		Mtx dctBuf;
		for (int i = 0; i < 8; i++)
		{
			for (int j = 0; j < 8; j++)
			{
				double tempVal = 0.0;
				for (int k = 0; k < 8; k++)
				{
					tempVal += left[i][k] * right[k][j];
				}
				dctBuf[i][j] = round(tempVal);
			}
		}
		return dctBuf;
	}

	/* 计算矩阵乘积 */
	Mtx MtxMulD2I(const double left[8][8], Mtx &right)
	{
		Mtx dctBuf;
		for (int i = 0; i < 8; i++)
		{
			for (int j = 0; j < 8; j++)
			{
				double tempVal = 0.0;
				for (int k = 0; k < 8; k++)
				{
					tempVal += left[i][k] * right[k][j];
				}
				dctBuf[i][j] = round(tempVal);
			}
		}
		return dctBuf;
	}

	void UnZigZag(Mtx &block)
	{
		int tempBuf[64];
		for (int i = 0; i < 64; i++) tempBuf[i] = block[i / 8][i % 8];

		for (int i = 0; i < 64; i++) block[i / 8][i % 8] = tempBuf[UnZigZagTable[i]];
	}

	/* 获取查哈夫曼表后得到真实数据的有效长度， 读取指定的位数 */
	int GetRealValue(int length);

	void DCT(Mtx &block)
	{
		block = MtxMulD2I(MtxDCT, block);
		block = MtxMulI2D(block, MtxIDCT);
	}

	/* 反离散余弦变换 */
	void IDCT(Mtx &block)
	{
		block = MtxMulD2I(MtxIDCT, block);
		block = MtxMulI2D(block, MtxDCT);
	}


	/* 反采样 */
	Mtx InveseSample(Mtx &block, int number);

	/* 颜色空间转换 */
	void ConvertClrSpace(Mtx &Y, Mtx &Cr, Mtx &Cb, Pixel out[8][8]);

	/* 将 MCU 单元 YCrCb 颜色转换为 RGB 颜色 */
	void Convert();

	void WriteToRGBBuffer(Pixel buf[8][8], int blockIndex);

	/* 重置 DC 值到 0 */
	void ResetDC()
	{
		dcY = dcCr = dcCb = 0x0;
	}


	void WirteBlock(Pixel block[16][16])
	{
		int x = (idxOfBlock / yNumberOfBlock) * 16; //
		int y = (idxOfBlock % yNumberOfBlock) * 16; //
		idxOfBlock++;

		for (int i = 0; i < 16; i++)
		{
			if ((x + i) >= img.Height) break;
			for (int j = 0; j < 16; j++)
			{
				if (y + j >= img.Width) continue;
				int offset = ((x + i) * img.Width + (y + j)) * 3;
				img.Data[offset + 0] = block[i][j].B;
				img.Data[offset + 1] = block[i][j].G;
				img.Data[offset + 2] = block[i][j].R;
			}
		}
	}



	/* 定位到数据流的第一个字节 */
	void ToStartOfData()
	{
		int offset = 623; // 文件头的长度
		fseek(fp, offset, SEEK_SET);  // 头文件头开始
		fread(&currentVal, 1, 1, fp); // 读取第一个数据
	}

	/* @brief 获取下一个 Key 的 value
		@params:
				table: type of huffman ( 00, 01, 10, 11 )
	*/
	int FindKeyValue(int table);

	int NextBit()
	{
		if (readCount == 0x0)
		{
			// reset
			readCount = 0x80;
			fread(&currentVal, 1, 1, fp);

			// check
			if (currentVal == 0xFF) //标记值
			{
				fread(&currentVal, 1, 1, fp); //读取下一个字节
				switch (currentVal)
				{
				case 0x00: currentVal = 0xFF; break;
				case 0xD9: endOfDecoder = true; break;
				case 0xD0:
				case 0xD1:
				case 0xD2:
				case 0xD3:
				case 0xD4:
				case 0xD5:
				case 0xD6:
				case 0xD7: ResetDC(); break;
				default:break;
				};
			}
		}

		if (endOfDecoder) return 0; // end of decoder

		int retVal = currentVal & readCount; // 获取当前位的值 (1 or 0)
		readCount >>= 1;
		return (retVal > 0 ? 1 : 0);
	}
private:
	BitmapImage	 img; // image data
	FILE         *fp; // 文件指针
	uint8_t		 quantY[64]; // Y 量化表
	uint8_t		 quantC[64]; // Cr, Cb 共用量化表
	HuffmanTable huffman;    // 哈夫曼表
	MCU			 mcu;        // 最小编码（解码）单元
	int          dcY;        // Y 分量的 DC 差分值
	int          dcCr;       // Cr 分量的 DC 差分值
	int          dcCb;       // Cb 分量的 DC 差分值

	Pixel        rgbBuf[16][16]; // 解码后的RGB值
	int          idxOfBlock;  // index of block number
	int          xNumberOfBlock;
	int          yNumberOfBlock;

	bool		endOfDecoder;
	uint8_t		currentVal; // 当前解码数据
	uint8_t		readCount;  // 已读取 bit 数, >=8 时从文件读取下一个字节数据到 currentVal
};


#endif
