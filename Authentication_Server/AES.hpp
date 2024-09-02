/*
* 일단 현재 사용하려고 하는 암/복호화에 대한 정리
* 1. 클라이언트
*  - 암호화, 복호화의 주체가 아니다. key를 알 필요가 없다.
* 2. Auth Server
*  - 암호화의 주체.
* 3. Feat Server
*  - 복호화의 주체.
*
* 서버시스템 내에서만 암/복호화를 사용하며 key를 public으로 공개할 이유도 없기 때문에 대칭키를 사용해도 좋다.
*
* 블록기반 암호화를 사용해도 좋다.
* 현재 암/복호화를 사용할 것은 토큰. 그냥 유효기간과 userid만 전송하면 되기 때문이다.
*
* 암호문의 길이는 현재 16바이트로 설정하였다. (exptime = 8bytes, userid = 4bytes, pkt_cnt = 4bytes)
*
* AES
* -> 16bytes * n의 크기를 갖는 문자열에 대해 암호화할 수 있는 알고리즘.
* 크기가 맞지 않는다면 padding을 통해 문자열의 크기를 늘려 사용한다.
*
* Key Size : 128, 192, 256 bits
* Key Size가 크면 보안이 강해지지만, Process가 커짐
* 
* openssl 의 AES를 사용하자!
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


// !! key는 반드시 32바이트(NULL포함33바이트)로 설정할 것 !!
// AES256을 사용할 것이기 때문에 key의 길이는 32바이트이다.
// !! iv는 반드시 16바이트로 설정할 것 !!

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