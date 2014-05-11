#include "../test.h"

#include <galib/encoding/base64.h>

using namespace galib::encoding;

TEST(Base64Test, Str) {
	std::string data = "aaaaaaaBBBBBB111111111";
	char out[1000];
	char data2[1000];

	size_t n = base64encode((const unsigned char*) data.c_str(), data.size()+1, out, 1000);
	EXPECT_TRUE(n > 0);
	size_t n2 = 1000;
	base64decode(out, n, (unsigned char*) data2, &n2);

	EXPECT_EQ(data.size()+1, n2);
	EXPECT_STREQ(data.c_str(), data2);
}

TEST(Base64Test, BinaryData) {
	unsigned char in[2000];
	char out[2000];
	char data[2000];

	size_t inSize = 1000;
	for(size_t i=0; i<inSize; i++) {
		in[i] = (char) i;
	}

	size_t m = base64encode(in, inSize, out, 2000);
	EXPECT_TRUE(m > 0);
	size_t n = 100000;
	base64decode(out, m, (unsigned char*) data, &n);

	EXPECT_EQ(inSize, n);
	for(size_t i=0; i<inSize; i++) {
		char c1 = *(in + i);
		char c2 = *(data + i);
		EXPECT_EQ(c1, c2);
	}
}


