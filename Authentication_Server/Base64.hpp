#pragma once

// �ڵ� ��ó : https://www.iamcorean.net/125
// �Ϻ� ������.

/*
* use ex.
	char txt[16] = "hi\0bye";
	char* etxt = nullptr;
	unsigned char* dtxt = nullptr;

	int iret = base64_encode(txt, 6, etxt); // 48bit -> 8bytes + 1bytes
	if (iret != 0)
	{
		std::string str(etxt);
		std::cout << etxt << "\n";
	}

	dtxt = new unsigned char[7];

	iret = base64_decode(etxt, dtxt, 6);

	delete[] etxt;


	for (int i = 0; i < 7; i++)
	{
		if (dtxt[i] == NULL)
		{
			std::cout << "NULL";
		}
		std::cout << dtxt[i];
	}

	delete[] dtxt;
*/


#include <string.h>
#include <stdlib.h>
#include <string>

/*------ Base64 Encoding Table ------*/
static const char MimeBase64[] = {
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
	'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
	'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
	'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
	'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
	'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
	'w', 'x', 'y', 'z', '0', '1', '2', '3',
	'4', '5', '6', '7', '8', '9', '+', '/'
};

/*------ Base64 Decoding Table ------*/
// 1bytes -> 2^8 = 256��.
// �� �� 64���� askii���� ����Ѵ�. -1�� ������.
// decode table�� -1���� �ش��ϴ� input�� ���Դٸ�, �̴� base64������ encoding�� �ƴ϶�� ��.
static int DecodeMimeBase64[256] = {
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,//00-0F
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,//10-1F
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63,//20-2F
	52,53,54,55,56,57,58,59,60,61,-1,-1,-1,-1,-1,-1,//30-3F
	-1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,//40-4F
	15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,//50-5F
	-1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,//60-6F
	41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1,//70-7F
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,//80-8F
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,//90-9F
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,//A0-AF
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,//B0-BF
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,//C0-CF
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,//D0-DF
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,//E0-EF
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1 //F0-FF
};


int base64_decode(char* text, unsigned char* dst, int numBytes)
{
	const char* cp;
	int space_idx = 0, phase;
	int d, prev_d = 0;
	unsigned char c;
	space_idx = 0;
	phase = 0;
	for (cp = text; *cp != '\0'; ++cp) {
		d = DecodeMimeBase64[(int)*cp];
		if (d != -1) {
			switch (phase) {
			case 0:												// ���� 6bits�ۿ� ������ ����. (�� �ѱ�.)
				++phase;
				break;
			case 1:												// 0phase�� ���� 6bits�� ���� ���� 6bits�� 12bits�� 8bits�� decode.
				c = ((prev_d << 2) | ((d & 0x30) >> 4));		// combined bits (8bits)
				if (space_idx < numBytes)
					dst[space_idx++] = c;
				++phase;
				break;
			case 2:												// 1phase�� ���� 4bits�� ���� ���� 6bits�� 10bits�� 8bits�� decode.
				c = (((prev_d & 0xf) << 4) | ((d & 0x3c) >> 2));
				if (space_idx < numBytes)
					dst[space_idx++] = c;
				++phase;
				break;
			case 3:												// 2phase�� ���� 2bits�� ���� ���� 6bits�� bits�� ����� decode.
				c = (((prev_d & 0x03) << 6) | d);
				if (space_idx < numBytes)
					dst[space_idx++] = c;
				phase = 0;
				break;
			}
			prev_d = d;											// �� phase�� 6bits�δ� decode�� �Ұ����ϴ� ����phase�� �ѱ�.
		}
	}

	return space_idx;
}

int base64_encode(char* text, int numBytes, char*& encodedText)
{
	unsigned char input[3] = { 0,0,0 };
	unsigned char output[4] = { 0,0,0,0 };
	int index, i, j, size;
	char* p, * plen;
	plen = text + numBytes - 1;
	size = (4 * (numBytes / 3)) + (numBytes % 3 ? 4 : 0) + 1; // 6bits�� 1bytes�� ȯ��. 3bytes = 24bits => 4bytes, 4bytes = 32bits => 6bytes (4bits padding)
	
	encodedText = new char[size];// (char*)malloc(size);
	ZeroMemory(encodedText, size);

	j = 0;
	for (i = 0, p = text; p <= plen; i++, p++)
	{
		index = i % 3;														// 3bytes
		input[index] = *p;													// 3bytes
		if (index == 2 || p == plen) {										// 3bytes input -> 4bytes output (8bits*3 -> 6bits*4)
			output[0] = ((input[0] & 0xFC) >> 2);							// ���� 6bits�� ���
			output[1] = ((input[0] & 0x3) << 4) | ((input[1] & 0xF0) >> 4); // input0�� ���� 2bits�� input1�� ���� 4bits�� ������ ���
			output[2] = ((input[1] & 0xF) << 2) | ((input[2] & 0xC0) >> 6); // input1�� ���� 4bits�� input2�� ���� 2bits�� ������ ���
			output[3] = (input[2] & 0x3F);									// input2�� ���� 6bits�� ���
			encodedText[j++] = MimeBase64[output[0]];
			encodedText[j++] = MimeBase64[output[1]];
			encodedText[j++] = index == 0 ? '=' : MimeBase64[output[2]];	// padding : output
			encodedText[j++] = index < 2 ? '=' : MimeBase64[output[3]];		// padding : output
			input[0] = input[1] = input[2] = 0;								// init
		}
	}
	encodedText[j] = NULL;
	return size;
}