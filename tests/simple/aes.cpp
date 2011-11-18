#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <cstdio>

typedef unsigned char uint8;

uint8 mul(uint8 a, uint8 b)
{
	uint8 p = 0;

	for (int i = 0; i < 8; i++) {
		if (b & 1)
			p ^= a;

		unsigned int aa = a << 1;
		if (a & 0x80)
			a = aa^0x1b;
		else
			a = aa&0xff;

		b >>= 1;
	}
	
	return p;
}

uint8 inv(uint8 a)
{
	uint8 r = 1;
	for (int i = 1; i < 8; i++) {
		a = mul(a, a);
		r = mul(r, a);
	}
	return r;
}

uint8 sbox(uint8 a)
{
	uint8 ret = inv(a);
	bool bprime[8], b[8];

	for (int i = 0; i < 8; i++) {
		b[i] = ret & 0x01;
		ret >>= 1;
	}

	for (int j = 0; j < 8; j++)
		bprime[j] = b[j] ^ b[ (j+4)%8 ] ^ b[ (j+5)%8 ] ^ b[ (j+6)%8 ] ^ b[ (j+7)%8 ];

	ret = bprime[7];
	for (int i = 6; i >= 0; i--) {
		ret <<= 1;
		ret += bprime[i];
	}

	return ret ^ 0x63;
}

void subBytes(uint8 state[16])
{
	for (int i = 0; i < 16; i++)
		state[i] = sbox(state[i]);
}

void addRoundKey(uint8 state[16], uint8 key[16])
{
	for (int i = 0; i < 16; i++)
		state[i] ^= key[i];
}

void shiftRows(uint8 state[16])
{
	uint8 buf[16];

	buf[ 0] = state[ 0]; // 1st row no change
	buf[ 1] = state[ 5];
	buf[ 2] = state[10];
	buf[ 3] = state[15];

	buf[ 4] = state[ 4]; // 2nd row shift 1
	buf[ 5] = state[ 9];
	buf[ 6] = state[14];
	buf[ 7] = state[ 3];

	buf[ 8] = state[ 8]; // 3rd row shift 2
	buf[ 9] = state[13];
	buf[10] = state[ 2];
	buf[11] = state[ 7];

	buf[12] = state[12]; // 4th row shift 3
	buf[13] = state[ 1];
	buf[14] = state[ 6];
	buf[15] = state[11];

	for (int i = 0; i < 16; i++)
		state[i] = buf[i];
}

void mixColumns(uint8 state[16])
{
	uint8 a[16], b[16];

	for (int i = 0; i < 16; i++) {
		a[i] = state[i];
		b[i] = mul(2, a[i]);
	}

	state[ 0] = b[ 0] ^ a[ 3] ^ a[ 2] ^ b[ 1] ^ a[ 1];
	state[ 1] = b[ 1] ^ a[ 0] ^ a[ 3] ^ b[ 2] ^ a[ 2];
	state[ 2] = b[ 2] ^ a[ 1] ^ a[ 0] ^ b[ 3] ^ a[ 3];
	state[ 3] = b[ 3] ^ a[ 2] ^ a[ 1] ^ b[ 0] ^ a[ 0];

	state[ 4] = b[ 4] ^ a[ 7] ^ a[ 6] ^ b[ 5] ^ a[ 5];
	state[ 5] = b[ 5] ^ a[ 4] ^ a[ 7] ^ b[ 6] ^ a[ 6];
	state[ 6] = b[ 6] ^ a[ 5] ^ a[ 4] ^ b[ 7] ^ a[ 7];
	state[ 7] = b[ 7] ^ a[ 6] ^ a[ 5] ^ b[ 4] ^ a[ 4];

	state[ 8] = b[ 8] ^ a[11] ^ a[10] ^ b[ 9] ^ a[ 9];
	state[ 9] = b[ 9] ^ a[ 8] ^ a[11] ^ b[10] ^ a[10];
	state[10] = b[10] ^ a[ 9] ^ a[ 8] ^ b[11] ^ a[11];
	state[11] = b[11] ^ a[10] ^ a[ 9] ^ b[ 8] ^ a[ 8];

	state[12] = b[12] ^ a[15] ^ a[14] ^ b[13] ^ a[13];
	state[13] = b[13] ^ a[12] ^ a[15] ^ b[14] ^ a[14];
	state[14] = b[14] ^ a[13] ^ a[12] ^ b[15] ^ a[15];
	state[15] = b[15] ^ a[14] ^ a[13] ^ b[12] ^ a[12];
}

void roundKey(uint8 key[16], uint8 prevKey[16], uint8 rcon)
{
	key[ 0] = prevKey[ 0] ^ sbox(prevKey[13]) ^ rcon;
	key[ 1] = prevKey[ 1] ^ sbox(prevKey[14]);
	key[ 2] = prevKey[ 2] ^ sbox(prevKey[15]);
	key[ 3] = prevKey[ 3] ^ sbox(prevKey[12]);

	for (int i = 4; i < 16; i++)
		key[i] = prevKey[i] ^ key[ i-4 ];
}

inline void print(uint8 m[16])
{
	for (int i = 0; i < 16; i++)
		std::cout << std::setw(4) << std::dec << static_cast<int>(m[i]);
	std::cout << std::endl;
}

inline void printHex(uint8 m[16])
{
	for (int i = 0; i < 16; i++)
		std::cout << std::setw(4) << std::hex << static_cast<int>(m[i]);
	std::cout << std::endl;
}

void aes(uint8 state[16], uint8 key[16])
{
	uint8 prevKey[16];
	uint8 rcon = 141;

	for (int i = 0; i < 10; i++) {
		std::cout << "Round: " << (i+1) << std::endl;

		std::cout << "start:  ";
		addRoundKey(state, key);
		print(state);

		std::cout << "s_box:  ";
		subBytes(state);
		print(state);

		std::cout << "s_row:  ";
		shiftRows(state);
		print(state);

		std::cout << "m_col:  ";
		if (i < 9)
			mixColumns(state);
		print(state);

		// key expansion
		for (int j = 0; j < 16; j++)
			prevKey[j] = key[j];
		rcon = mul(2, rcon);
		roundKey(key, prevKey, rcon);
		std::cout << "k_sch:  ";
		print(key);

	}

	std::cout << "output: ";
	addRoundKey(state, key);
	print(state);
	printHex(state);
}

int main(int argc, char *argv[])
{

	uint8 key[16] = 
	{
		0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6, 
        0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c
	};

	uint8 txt[16] = 
	{
		0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6, 
        0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c
	};

	uint8 rcon = 141;
	uint8 prevKey[16];

	// key expansion
	for (int j = 0; j < 10; j++) {
	rcon = mul(2, rcon);
	for (int i = 0; i < 16; i++)
		prevKey[i] = key[i];
	roundKey(key, prevKey, rcon);
	print(key);
	}


//	uint8 key[16];
	//uint8 txt[16];
	//char s[16][32];
/*
	std::cout << "plaintext:" << std::endl;
	for (int i = 0; i < 16; i++) {
		std::cin >> s[i];
		sscanf(s[i], "%x", txt+i);
	}
*/
/*
	std::cout << "      key:" << std::endl;
	for (int i = 0; i < 16; i++) {
		std::cin >> s[i];
		sscanf(s[i], "%x", key+i);
	}
*/

	std::cout << "plaintext: ";
	print(txt);
	std::cout << "      key: ";
	print(key);

	aes(txt, key);
	return 0;
}
