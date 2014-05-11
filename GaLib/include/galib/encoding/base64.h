#ifndef galib_encoding_base64_h__
#define galib_encoding_base64_h__

#include "../base.h"

namespace galib {

	namespace encoding {

		// ---- Base64 methods ----
		size_t base64encode(const unsigned char* data, size_t dataLength, char* result, size_t resultSize);

		size_t base64decode (char *in, size_t inLen, unsigned char *out, size_t *inOutLen);

		// ---- Inline implementation ----
		inline size_t base64encode(const unsigned char* data, size_t dataLength, char* result, size_t resultSize) {
			const char base64chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
			size_t resultIndex = 0, n = 0;
			size_t padCount = dataLength % 3;
			unsigned char n0, n1, n2, n3;

			// increment over the length of the string, three characters at a time
			for (size_t i = 0; i < dataLength; i += 3)  {
				// these three 8-bit (ASCII) characters become one 24-bit number
				n = data[i] << 16;

				if((i+1) < dataLength) {
					n += data[i+1] << 8;
				}

				if((i+2) < dataLength) {
					n += data[i+2];
				}

				// this 24-bit number gets separated into four 6-bit numbers
				n0 = (unsigned char)(n >> 18) & 63;
				n1 = (unsigned char)(n >> 12) & 63;
				n2 = (unsigned char)(n >> 6) & 63;
				n3 = (unsigned char) n & 63;

				// if we have one byte available, then its encoding is spread out over two characters
				if(resultIndex >= resultSize) {
					// indicate failure: buffer too small
					return 0;
				}
				result[resultIndex++] = base64chars[n0];
				if(resultIndex >= resultSize) {
					// indicate failure: buffer too small
					return 0;
				}
				result[resultIndex++] = base64chars[n1];

				// if we have only two bytes available, then their encoding is spread out over three chars
				if((i+1) < dataLength) {
					if(resultIndex >= resultSize) {
						// indicate failure: buffer too small
						return 0;
					}
					result[resultIndex++] = base64chars[n2];
				}

				// if we have all three bytes available, then their encoding is spread out over four characters
				if((i+2) < dataLength) {
					if(resultIndex >= resultSize) {
						// indicate failure: buffer too small
						return 0;
					}
					result[resultIndex++] = base64chars[n3];
				}
			}  

			// create and add padding that is required if we did not have a multiple of 3 number of characters available
			if (padCount > 0) { 
				for (; padCount < 3; padCount++) { 
					if(resultIndex >= resultSize) {
						// indicate failure: buffer too small
						return 0;
					}
					result[resultIndex++] = '=';
				}
			}
			if(resultIndex >= resultSize) {
				// indicate failure: buffer too small
				return 0;
			}
			result[resultIndex] = 0;
			return resultIndex;   // indicate success
		}

		inline size_t base64decode (char *in, size_t inLen, unsigned char *out, size_t *inOutLen) { 
			static const unsigned char d[] = {
				66,66,66,66,66,66,66,66,66,64,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,
				66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,62,66,66,66,63,52,53,
				54,55,56,57,58,59,60,61,66,66,66,65,66,66,66, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
				10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,66,66,66,66,66,66,26,27,28,
				29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,66,66,
				66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,
				66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,
				66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,
				66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,
				66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,
				66,66,66,66,66,66
			};

			static const char WHITESPACE = 64;
			static const char EQUALS     = 65;
			static const char INVALID    = 66;

			char *end = in + inLen;
			size_t buf = 1, len = 0;

			while (in < end) {
				unsigned char c = d[*in++];

				switch (c) {
				case WHITESPACE: continue;   // skip whitespace
				case INVALID:    return 0;   // invalid input, return error
				case EQUALS:                 // pad character, end of data
					in = end;
					continue;
				default:
					buf = buf << 6 | c;

					// If the buffer is full, split it into bytes
					if (buf & 0x1000000) {
						if ((len += 3) > *inOutLen) return 0; // buffer overflow
						*out++ = buf >> 16;
						*out++ = buf >> 8;
						*out++ = buf;
						buf = 1;
					}   
				}
			}

			if (buf & 0x40000) {
				if ((len += 2) > *inOutLen) {
					// buffer overflow
					return 0;
				}
				*out++ = buf >> 10;
				*out++ = buf >> 2;
			}
			else if (buf & 0x1000) {
				if (++len > *inOutLen) {
					// buffer overflow
					return 0;
				}
				*out++ = buf >> 4;
			}

			*inOutLen = len; // modify to reflect the actual output size
			return len;
		}

	}
}

#endif
