#ifndef galib_util_hash_h__
#define galib_util_hash_h__

#include "../base.h"

namespace galib {

	namespace util {

		//
		// fnv_32a_buf - perform a 32 bit Fowler/Noll/Vo FNV-1a hash on a buffer
		//
		// input:
		//	buf	- start of buffer to hash
		//	len	- length of buffer in octets
		//
		// returns:
		//	32 bit/64bit hash as a static hash type
		//
		inline size_t hash_string(const char *buf, size_t len) {
#if defined(_WIN64) || defined(__x86_64__) || defined(__ppc64__)
			size_t hval = 0xcbf29ce484222325ULL; // FNV1A_64_INIT
#else
			size_t hval = 0x811c9dc5; // FNV1_32A_INIT
#endif
			const char *bp = (const char *)buf;	/* start of buffer */
			const char *be = bp + len;		/* beyond end of buffer */

			/*
			* FNV-1a hash each octet in the buffer
			*/
			while (bp < be) {
				hval ^= (size_t)*bp++;

#if defined(_WIN64) || defined(__x86_64__) || defined(__ppc64__)
				hval += (hval << 1) + (hval << 4) + (hval << 5) + (hval << 7) + (hval << 8) + (hval << 40);
#else
				hval += (hval<<1) + (hval<<4) + (hval<<7) + (hval<<8) + (hval<<24);
#endif
			}

			return hval;
		}

		struct gahash {
			std::size_t operator()(const std::string& str) const {
				return hash_string(str.c_str(), str.size());
			}
		};

	}

}

#endif
