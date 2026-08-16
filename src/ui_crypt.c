#include "BridgeEngine.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// .uix format: "BUIX" magic + uint32 LE checksum + uint32 LE length + payload.
// The payload is plain XML XORed with a keyed stream (obfuscation, not strong
// encryption). Byte-level primitives are shared by the editor, the build-time
// encryptor, and the runtime loader.

#define UIX_MAGIC "BUIX"
#define UIX_MAGIC_LEN 4

// FNV-1a 32-bit, used both as a checksum and to seed the key stream.
static uint32_t uix_fnv1a(const unsigned char *data, size_t len)
{
	uint32_t hash = 2166136261u;
	for (size_t i = 0; i < len; i++) {
		hash ^= data[i];
		hash *= 16777619u;
	}
	return hash;
}

// Expand a key into a 256-byte S-box (RC4-style KSA).
static void uix_key_schedule(const char *key, uint8_t sbox[256])
{
	size_t key_len = key ? strlen(key) : 0;
	if (key_len == 0) key_len = 1;
	for (int i = 0; i < 256; i++) sbox[i] = (uint8_t)i;
	uint8_t j = 0;
	for (int i = 0; i < 256; i++) {
		j = (uint8_t)(j + sbox[i] + (uint8_t)key[i % key_len]);
		uint8_t t = sbox[i];
		sbox[i]	 = sbox[j];
		sbox[j]	 = t;
	}
}

// XOR the payload in place with the key stream derived from the key.
static void uix_crypt(uint8_t *data, size_t len, const char *key)
{
	uint8_t sbox[256];
	uix_key_schedule(key, sbox);
	uint8_t i = 0, j = 0;
	for (size_t n = 0; n < len; n++) {
		i = (uint8_t)(i + 1);
		j = (uint8_t)(j + sbox[i]);
		uint8_t t = sbox[i];
		sbox[i]	 = sbox[j];
		sbox[j]	 = t;
		uint8_t k = sbox[(uint8_t)(sbox[i] + sbox[j])];
		data[n] ^= k;
	}
}

// Encrypt `plain` (len bytes) into a malloc'd .uix payload (magic + checksum +
// length + ciphertext). Returns 0 on success.
static int uix_encrypt_bytes(const char *key, const unsigned char *plain, size_t len,
							 unsigned char **out, size_t *out_len)
{
	uint32_t checksum = uix_fnv1a(plain, len);
	size_t	total	 = UIX_MAGIC_LEN + 4 + 4 + len;
	unsigned char *buf = malloc(total);
	if (!buf) return -1;
	memcpy(buf, UIX_MAGIC, UIX_MAGIC_LEN);
	buf[UIX_MAGIC_LEN + 0] = (unsigned char)(checksum & 0xFF);
	buf[UIX_MAGIC_LEN + 1] = (unsigned char)((checksum >> 8) & 0xFF);
	buf[UIX_MAGIC_LEN + 2] = (unsigned char)((checksum >> 16) & 0xFF);
	buf[UIX_MAGIC_LEN + 3] = (unsigned char)((checksum >> 24) & 0xFF);
	buf[UIX_MAGIC_LEN + 4] = (unsigned char)(len & 0xFF);
	buf[UIX_MAGIC_LEN + 5] = (unsigned char)((len >> 8) & 0xFF);
	buf[UIX_MAGIC_LEN + 6] = (unsigned char)((len >> 16) & 0xFF);
	buf[UIX_MAGIC_LEN + 7] = (unsigned char)((len >> 24) & 0xFF);
	memcpy(buf + UIX_MAGIC_LEN + 8, plain, len);
	uix_crypt(buf + UIX_MAGIC_LEN + 8, len, key);
	*out	 = buf;
	*out_len = total;
	return 0;
}

// Decrypt a .uix payload (data, len) into a malloc'd plaintext buffer.
// Returns 0 on success; -1 on bad magic, missing key, or checksum mismatch.
static int uix_decrypt_bytes(const char *key, const unsigned char *data, size_t len,
							 unsigned char **out, size_t *out_len)
{
	if (!key || !key[0]) return -1;
	if (len < UIX_MAGIC_LEN + 8) return -1;
	if (memcmp(data, UIX_MAGIC, UIX_MAGIC_LEN) != 0) return -1;
	uint32_t checksum = (uint32_t)data[UIX_MAGIC_LEN + 0] |
						((uint32_t)data[UIX_MAGIC_LEN + 1] << 8) |
						((uint32_t)data[UIX_MAGIC_LEN + 2] << 16) |
						((uint32_t)data[UIX_MAGIC_LEN + 3] << 24);
	uint32_t length = (uint32_t)data[UIX_MAGIC_LEN + 4] |
					  ((uint32_t)data[UIX_MAGIC_LEN + 5] << 8) |
					  ((uint32_t)data[UIX_MAGIC_LEN + 6] << 16) |
					  ((uint32_t)data[UIX_MAGIC_LEN + 7] << 24);
	if (UIX_MAGIC_LEN + 8 + (size_t)length > len) return -1;
	unsigned char *plain = malloc(length ? length : 1);
	if (!plain) return -1;
	memcpy(plain, data + UIX_MAGIC_LEN + 8, length);
	uix_crypt(plain, length, key);
	if (uix_fnv1a(plain, length) != checksum) {
		free(plain);
		return -1; // wrong key or tampered file
	}
	*out	 = plain;
	*out_len = length;
	return 0;
}

// Read an entire file into a malloc'd buffer. Returns 0 on success.
static int read_whole_file(const char *path, unsigned char **out, size_t *out_len)
{
	FILE *file = fopen(path, "rb");
	if (!file) return -1;
	fseek(file, 0, SEEK_END);
	long size = ftell(file);
	fseek(file, 0, SEEK_SET);
	if (size < 0) {
		fclose(file);
		return -1;
	}
	unsigned char *buf = malloc((size_t)size ? (size_t)size : 1);
	if (!buf) {
		fclose(file);
		return -1;
	}
	if (size > 0 && fread(buf, 1, (size_t)size, file) != (size_t)size) {
		free(buf);
		fclose(file);
		return -1;
	}
	fclose(file);
	*out	 = buf;
	*out_len = (size_t)size;
	return 0;
}

// Encrypt a plain XML file into a .uix file. Returns 0 on success.
int bapi_uix_encrypt_file(const char *src_xml_path, const char *dst_uix_path, const char *key)
{
	if (!src_xml_path || !dst_uix_path || !key || !key[0]) return -1;
	unsigned char *plain;
	size_t		   plain_len;
	if (read_whole_file(src_xml_path, &plain, &plain_len) != 0) return -1;
	unsigned char *payload;
	size_t		   payload_len;
	int			   rc = uix_encrypt_bytes(key, plain, plain_len, &payload, &payload_len);
	free(plain);
	if (rc != 0) return -1;
	FILE *f = fopen(dst_uix_path, "wb");
	if (!f) {
		free(payload);
		return -1;
	}
	int ok = fwrite(payload, 1, payload_len, f) == payload_len;
	fclose(f);
	free(payload);
	return ok ? 0 : -1;
}

// Decrypt a .uix file back into plain XML. Returns 0 on success.
int bapi_uix_decrypt_file(const char *src_uix_path, const char *dst_xml_path, const char *key)
{
	if (!src_uix_path || !dst_xml_path || !key || !key[0]) return -1;
	unsigned char *data;
	size_t		   data_len;
	if (read_whole_file(src_uix_path, &data, &data_len) != 0) return -1;
	unsigned char *plain;
	size_t		   plain_len;
	int			   rc = uix_decrypt_bytes(key, data, data_len, &plain, &plain_len);
	free(data);
	if (rc != 0) return -1;
	FILE *f = fopen(dst_xml_path, "wb");
	if (!f) {
		free(plain);
		return -1;
	}
	int ok = fwrite(plain, 1, plain_len, f) == plain_len;
	fclose(f);
	free(plain);
	return ok ? 0 : -1;
}

// Byte-level helpers shared with src/ui_xml.c (bapi_ui_save_to_file / load).
int bapi_uix_encrypt_mem(const char *key, const unsigned char *plain, size_t len,
						 unsigned char **out, size_t *out_len)
{
	return uix_encrypt_bytes(key, plain, len, out, out_len);
}

int bapi_uix_decrypt_mem(const char *key, const unsigned char *data, size_t len,
						 unsigned char **out, size_t *out_len)
{
	return uix_decrypt_bytes(key, data, len, out, out_len);
}

int bapi_uix_is_encrypted_mem(const unsigned char *data, size_t len)
{
	return len >= UIX_MAGIC_LEN && memcmp(data, UIX_MAGIC, UIX_MAGIC_LEN) == 0;
}
