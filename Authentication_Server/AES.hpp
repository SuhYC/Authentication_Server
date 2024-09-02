/*
* �ϴ� ���� ����Ϸ��� �ϴ� ��/��ȣȭ�� ���� ����
* 1. Ŭ���̾�Ʈ
*  - ��ȣȭ, ��ȣȭ�� ��ü�� �ƴϴ�. key�� �� �ʿ䰡 ����.
* 2. Auth Server
*  - ��ȣȭ�� ��ü.
* 3. Feat Server
*  - ��ȣȭ�� ��ü.
*
* �����ý��� �������� ��/��ȣȭ�� ����ϸ� key�� public���� ������ ������ ���� ������ ��ĪŰ�� ����ص� ����.
*
* ��ϱ�� ��ȣȭ�� ����ص� ����.
* ���� ��/��ȣȭ�� ����� ���� ��ū. �׳� ��ȿ�Ⱓ�� userid�� �����ϸ� �Ǳ� �����̴�.
*
* ��ȣ���� ���̴� ���� 16����Ʈ�� �����Ͽ���. (exptime = 8bytes, userid = 4bytes, pkt_cnt = 4bytes)
*
* AES
* -> 16bytes * n�� ũ�⸦ ���� ���ڿ��� ���� ��ȣȭ�� �� �ִ� �˰���.
* ũ�Ⱑ ���� �ʴ´ٸ� padding�� ���� ���ڿ��� ũ�⸦ �÷� ����Ѵ�.
*
* Key Size : 128, 192, 256 bits
* Key Size�� ũ�� ������ ����������, Process�� Ŀ��
* 
* openssl �� AES�� �������!
*/

#pragma once

extern "C" {
#include <openssl/evp.h>
#include <openssl/aes.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
}

#include <iostream>


// !! key�� �ݵ�� 32����Ʈ(NULL����33����Ʈ)�� ������ �� !!
// AES256�� ����� ���̱� ������ key�� ���̴� 32����Ʈ�̴�.
// !! iv�� �ݵ�� 16����Ʈ�� ������ �� !!

const unsigned char* AES_PRIVATE_KEY = reinterpret_cast<const unsigned char*>("12345678901234567890123456789012");
const unsigned char* AES_PRIVATE_IV = reinterpret_cast<const unsigned char*>("asdfasdfasdfasdf");

void init_openssl()
{
	// Initialize OpenSSL
	SSL_library_init();
	OpenSSL_add_all_algorithms();
	ERR_load_crypto_strings();

	return;
}

// AES encryption function
int encryptAES(const unsigned char* plaintext, int plaintext_len, const unsigned char* key, const unsigned char* iv, unsigned char* ciphertext)
{
	EVP_CIPHER_CTX* ctx;
	int len;
	int ciphertext_len;

	// Create and initialize the context
	if (!(ctx = EVP_CIPHER_CTX_new()))
	{
		return 0;
	}

	// Initialize AES CBC encryption
	if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key, iv) != 1)
	{
		EVP_CIPHER_CTX_free(ctx);
		return 0;
	}

	// Perform AES CBC encryption
	if (EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len) != 1)
	{
		EVP_CIPHER_CTX_free(ctx);
		return 0;
	}
	ciphertext_len = len;

	// Finalize AES CBC encryption (padding if necessary)
	if (EVP_EncryptFinal_ex(ctx, ciphertext + len, &len) != 1)
	{
		EVP_CIPHER_CTX_free(ctx);
		return 0;
	}
	ciphertext_len += len;

	// Clean up
	EVP_CIPHER_CTX_free(ctx);

	return ciphertext_len;
}

// AES decryption function
int decryptAES(const unsigned char* ciphertext, int ciphertext_len, const unsigned char* key, const unsigned char* iv, unsigned char* plaintext)
{
	EVP_CIPHER_CTX* ctx;
	int len;
	int plaintext_len;

	// Create and initialize the context
	if (!(ctx = EVP_CIPHER_CTX_new()))
	{
		return 0;
	}

	// Initialize AES CBC decryption
	if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key, iv) != 1)
	{
		EVP_CIPHER_CTX_free(ctx);
		return 0;
	}

	// Perform AES CBC decryption
	if (EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len) != 1)
	{
		EVP_CIPHER_CTX_free(ctx);
		return 0;
	}
	plaintext_len = len;

	// Finalize AES CBC decryption (padding if necessary)
	if (EVP_DecryptFinal_ex(ctx, plaintext + len, &len) != 1)
	{
		EVP_CIPHER_CTX_free(ctx);
		return plaintext_len;
	}
	plaintext_len += len;

	// Clean up
	EVP_CIPHER_CTX_free(ctx);

	return plaintext_len;
}