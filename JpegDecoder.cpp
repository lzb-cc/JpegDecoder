
#include "JpegDecoder.h"
#include <math.h>
#include <stdlib.h>

using namespace std;

//---------------------------------------------------------------------------

/* @brief �����������
 */
void JpegDecoder::DecoderTable()
{
	int val = 0; // type of table ( DC(0,1) or AC(0,1) )
	uint8_t buf[16];

	fread(&val, 1, 1, fp); // get type of table
	fread(buf, 1, 16, fp); // get key value
	map<string, uint8_t> &tb = huffman[val];

	string keyStr = "";
	for (int i = 0; i < 16; i++) // length of key (i.e. i = 2 means key = 000 , 001 , 010 , 011 or ...)
	{
		int cnt = buf[i]; // number of key, which length is (i+1)

		/* alignment */
		for (int k = keyStr.length(); k <= i; k++)
		{
			keyStr += "0";
		}

		while (cnt > 0)
		{
			/* value of key */
			fread(&val, 1, 1, fp); // read value
			//printf("%s = %X\n", keyStr.c_str(), val);
			tb.insert(pair<string, uint8_t>(keyStr, val));

			/* increment */
			int carry = 1; //��λ
			for (int k = keyStr.length() - 1; k >= 0; k--)
			{
				int tmpVal = (keyStr[k] + carry - '0'); //�����λ
				carry = tmpVal / 2;
				keyStr[k] = tmpVal % 2 + '0'; //�����ǰλ���
			}
			cnt = cnt - 1;
		}
	}
}

//-------------------------------------------------------------------------------

Mtx JpegDecoder::InveseSample(Mtx& block, int number)
{
	Mtx ret;

	int x = (number / 2) * 4;
	int y = (number % 2) * 4;
	for (int i = 0; i < 8; i += 2)
	{
		for (int j = 0; j < 8; j += 2)
		{
			ret[i][j] = ret[i][j + 1] = ret[i + 1][j] = ret[i + 1][j + 1] = block[x + i / 2][y + j / 2];
		}
	}

	return ret;
}


/* @brief ��ɫ�ռ�ת�� YCbCr -> RGB
 */
void JpegDecoder::ConvertClrSpace(Mtx &Y, Mtx &Cb, Mtx &Cr, Pixel out[8][8])
{
	for (int i = 0; i < 8; i++)
	{
		for (int j = 0; j < 8; j++)
		{
			out[i][j].R = Y[i][j] + 1.402 * Cr[i][j] + 128;
			out[i][j].G = Y[i][j] - 0.34414 * Cb[i][j] - 0.71414 * Cr[i][j] + 128;
			out[i][j].B = Y[i][j] + 1.772 * Cb[i][j] + 128;

			/* �ض� */
			if (out[i][j].R > 255) out[i][j].R = 255;
			if (out[i][j].G > 255) out[i][j].G = 255;
			if (out[i][j].B > 255) out[i][j].B = 255;

			/* �ض� */
			if (out[i][j].R < 0) out[i][j].R = 0;
			if (out[i][j].G < 0) out[i][j].G = 0;
			if (out[i][j].B < 0) out[i][j].B = 0;
		}
	}
}


/* @brief ���� ConvertClrSpace(), �� MCU �е� YCbCr ��ɫת��Ϊ RGB ��ɫ
 */
void JpegDecoder::Convert()
{
	Mtx cb;
	Mtx cr;
	Pixel out[8][8];


	cb = InveseSample(mcu.cbMtx, 0);
	cr = InveseSample(mcu.crMtx, 0);
	ConvertClrSpace(mcu.yMtx[0], cb, cr, out);
	WriteToRGBBuffer(out, 0);

	cb = InveseSample(mcu.cbMtx, 1);
	cr = InveseSample(mcu.crMtx, 1);
	ConvertClrSpace(mcu.yMtx[1], cb, cr, out);
	WriteToRGBBuffer(out, 1);

	cb = InveseSample(mcu.cbMtx, 2);
	cr = InveseSample(mcu.crMtx, 2);
	ConvertClrSpace(mcu.yMtx[2], cb, cr, out);
	WriteToRGBBuffer(out, 2);

	//for (int i = 0; i < 8; i++)
	//{
	//	for (int j = 0; j < 8; j++)
	//	{
	//		cr[i][j] = mcu.crMtx[4 + i % 4][4 + j % 4];
	//		cb[i][j] = mcu.cbMtx[4 + i % 4][4 + i % 4];
	//	}
	//}

	cb = InveseSample(mcu.cbMtx, 3);
	cr = InveseSample(mcu.crMtx, 3);
	ConvertClrSpace(mcu.yMtx[3], cb, cr, out);
	WriteToRGBBuffer(out, 3);
}


/* @brief �ѽ��������RGB����д�뵽RGB��������
	@buf: ��������
	@blockIndex: ����ţ� ȡֵ��Χ: 00, 01, 10, 11 (������) -> 0��1��2��3 (ʮ����)

 */
void JpegDecoder::WriteToRGBBuffer(Pixel buf[8][8], int blockIndex)
{
	int xOffset = 8 * (blockIndex & 0x02) >> 1; // binary: blockIndex & 10 (i.e. if blockIndex =  01 => blockIndex & 0x02 == 01 & 10 => xOffset = 0 * 8 = 0 )
	int yOffset = 8 * (blockIndex & 0x01);      // binary: blockIndex & 01 (i.e. if blockIndex =  01 => blockIndex & 0x01 == 01 & 01 => yOffset = 1 * 8 = 8 )
	for (int i = 0; i < 8; i++)
	{
		for (int j = 0; j < 8; j++)
		{
			rgbBuf[xOffset + i][yOffset + j].R = buf[i][j].R;
			rgbBuf[xOffset + i][yOffset + j].G = buf[i][j].G;
			rgbBuf[xOffset + i][yOffset + j].B = buf[i][j].B;
		}
	}
}

//-------------------------------------------------------------------------------

/* @brief ����һ�� MCU
 */
void JpegDecoder::DecoderNextMCU()
{
	/* Y ������ ֱ�� 0�ű� */
	DecoderMtx(mcu.yMtx[0], 0, quantY, dcY);
	DecoderMtx(mcu.yMtx[1], 0, quantY, dcY);
	DecoderMtx(mcu.yMtx[2], 0, quantY, dcY);
	DecoderMtx(mcu.yMtx[3], 0, quantY, dcY);

	/* Cb, Cr ������ ֱ�� 1�ű� */
	DecoderMtx(mcu.cbMtx, 1, quantC, dcCr);
	DecoderMtx(mcu.crMtx, 1, quantC, dcCb);
}

//--------------------------------------------------------------------

/* @brief ����һ�� 8 x 8 �ľ����
 */
void JpegDecoder::DecoderMtx(Mtx &block, int table, uint8_t *quant, int &dc)
{
	if (endOfDecoder) return; // ������ֹ

	// reset matrix
	for (int i = 0; i < 64; i++) block[i / 8][i % 8] = 0x0;

	// decoder DC of matrix
	int length = FindKeyValue(table);
	int value = GetRealValue(length);
	dc += value; // DC
	block[0][0] = dc;

	// decoder AC of matrix, table = table + 16 => table = 0x00 (DC-0)-> table = 0x10 (AC-0)
	for (int i = 1; i < 64; i++)
	{
		length = FindKeyValue(table + 16);
		if (length == 0x0) break; // ��������

		value = GetRealValue(length & 0xf); // �ұ� 4λ��ʵ��ֵ����
		i += (length >> 4);          // ��� 4λ���г̳���
		block[i / 8][i % 8] = value; // AC
	}


	// ������
	for (int i = 0; i < 64; i++) block[i / 8][i % 8] *= quant[i];

	// �� Zig-Zag ����
	UnZigZag(block);

	// ����ɢ���ұ任
	IDCT(block);
}

/* @brief ���ҹ���������ȡ��һ����Чֵ
 */
int JpegDecoder::FindKeyValue(int table)
{
	map<string, uint8_t> &huf = huffman[table];

	string keyStr = "";
	while (huf.find(keyStr) == huf.end())
	{
		//printf("%s\n", keyStr.c_str());
		keyStr += (NextBit() + '0'); // char of 0 or 1
	}

	//printf("%s = %d\n", keyStr.c_str(), huf[keyStr]);
	return huf[keyStr];
}


/* ��ȡ��׼�������������ʵֵ */
int JpegDecoder::GetRealValue(int length)
{
	int retVal = 0;
	for (int i = 0; i < length; i++)
	{
		retVal = (retVal << 1) + NextBit();
	}

	return (retVal >= pow(2, length - 1) ? retVal : retVal - pow(2, length) + 1);
}
//---------------------------------------------------------------------

/* @brief ����
 */
BitmapImage &JpegDecoder::Decoder()
{
	/* decoder quant table */
	DecoderQuant();

	/* decoder width and height of image */
	DecoderSize();

	/* decoder huffman table of DC and AC */
	DecoderHuffman();

	/* decoder data */
	ToStartOfData();

	/* decoder MCU */
	int totalBlock = xNumberOfBlock * yNumberOfBlock;
	while (!endOfDecoder && (idxOfBlock < totalBlock))
	{
		DecoderNextMCU();

		Convert();

		WirteBlock(rgbBuf);
	}

	/* end of decoder */
	return img;
}
