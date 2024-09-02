#pragma once

// 코드 출처 : https://www.iamcorean.net/125
// 일부 수정함.

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
// 1bytes -> 2^8 = 256개.
// 그 중 64개의 askii값만 사용한다. -1은 사용안함.
// decode table의 -1값에 해당하는 input이 들어왔다면, 이는 base64형태의 encoding이 아니라는 것.
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
			case 0:												// 아직 6bits밖에 정보가 없음. (턴 넘김.)
				++phase;
				break;
			case 1:												// 0phase의 남은 6bits와 새로 들어온 6bits로 12bits중 8bits를 decode.
				c = ((prev_d << 2) | ((d & 0x30) >> 4));		// combined bits (8bits)
				if (space_idx < numBytes)
					dst[space_idx++] = c;
				++phase;
				break;
			case 2:												// 1phase의 남은 4bits와 새로 들어온 6bits로 10bits중 8bits를 decode.
				c = (((prev_d & 0xf) << 4) | ((d & 0x3c) >> 2));
				if (space_idx < numBytes)
					dst[space_idx++] = c;
				++phase;
				break;
			case 3:												// 2phase의 남은 2bits와 새로 들어온 6bits로 bits를 만들어 decode.
				c = (((prev_d & 0x03) << 6) | d);
				if (space_idx < numBytes)
					dst[space_idx++] = c;
				phase = 0;
				break;
			}
			prev_d = d;											// 각 phase의 6bits로는 decode가 불가능하니 다음phase로 넘김.
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
	size = (4 * (numBytes / 3)) + (numBytes % 3 ? 4 : 0) + 1; // 6bits당 1bytes로 환산. 3bytes = 24bits => 4bytes, 4bytes = 32bits => 6bytes (4bits padding)
	
	encodedText = new char[size];// (char*)malloc(size);
	ZeroMemory(encodedText, size);

	j = 0;
	for (i = 0, p = text; p <= plen; i++, p++)
	{
		index = i % 3;														// 3bytes
		input[index] = *p;													// 3bytes
		if (index == 2 || p == plen) {										// 3bytes input -> 4bytes output (8bits*3 -> 6bits*4)
			output[0] = ((input[0] & 0xFC) >> 2);							// 상위 6bits만 사용
			output[1] = ((input[0] & 0x3) << 4) | ((input[1] & 0xF0) >> 4); // input0의 하위 2bits와 input1의 상위 4bits를 연결해 사용
			output[2] = ((input[1] & 0xF) << 2) | ((input[2] & 0xC0) >> 6); // input1의 하위 4bits와 input2의 상위 2bits를 연결해 사용
			output[3] = (input[2] & 0x3F);									// input2의 하위 6bits를 사용
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