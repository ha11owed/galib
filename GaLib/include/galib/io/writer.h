#ifndef galib_io_writer_h__
#define galib_io_writer_h__

#include "../base.h"

namespace galib {

	namespace io {

		// ---- IWriter ----
		class IWriter {
		public:
			virtual ~IWriter() {}

			virtual void write(const char* inData, size_t n) = 0;
		};

		// ---- StringWriter ----
		class StringWriter : public IWriter {
		public:
			StringWriter() : IWriter() {}
			virtual ~StringWriter() {}

			std::string buffer;

			virtual void write(const char* inData, size_t n) {
				buffer.append(inData, n);
			}
		};


	}
}

#endif
