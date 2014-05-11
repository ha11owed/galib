#ifndef galib_io_reader_h__
#define galib_io_reader_h__

#include "../base.h"

namespace galib {

	namespace io {

		// ---- IReader ----
		class IReader {
		public:
			virtual ~IReader() {}

			virtual size_t read(char *outData, size_t n) = 0;
		};

		// ---- StringReader ----
		class StringReader : public IReader {
		public:
			StringReader(const std::string& data) : IReader(), buffer(data), idx(0) {}
			virtual ~StringReader() {}

			size_t read(char *outData, size_t n);

		private:
			std::string buffer;
			size_t idx;
		};

		// ---- BufferedReader ----
		class BufferedReader : public IReader {
		public:
			BufferedReader(IReader* _reader) : IReader(), iStart(0), iEnd(0), reader(_reader) {}
			virtual ~BufferedReader() {}

			virtual size_t read(char *outData, size_t n);

		private:
			static const size_t BufferSize = 4 * 1024;

			IReader* reader;
			char buffer[BufferSize];
			size_t iStart, iEnd;
		};



		// ---- Inline implementation ----
		inline size_t StringReader::read(char *outData, size_t n) {
			size_t s = 0;
			size_t len = buffer.size();
			if(idx < len) {
				s = len - idx;
				if(s > n) {
					s = n;
				}
				strncpy(outData, buffer.c_str() + idx, s);
				idx += s;
			}
			return s;
		}

		inline size_t BufferedReader::read(char *outData, size_t n) {
			size_t ret = 0;
			size_t s = iEnd - iStart;
			if(s > 0) {
				if(s <= n) {
					strncpy(outData, buffer + iStart, s);
					iStart = iEnd = 0;
					ret = s;
				}
				else {
					strncpy(outData, buffer + iStart, n);
					iStart += n;
					ret = n;
				}
			}
			while(ret < n) {
				s = reader->read(buffer, BufferSize);
				if(s == 0) { break; }
				size_t d = n - ret;
				if(s < d) {
					strncpy(outData + ret, buffer, s);
					ret += s;
				}
				else {
					strncpy(outData + ret, buffer, d);
					ret += d;
					iStart += d;
					iEnd += s;
				}
			}
			return ret;
		}

	}
}

#endif
