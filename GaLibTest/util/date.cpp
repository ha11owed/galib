#include "../test.h"

#include <galib/util/date.h>

using namespace galib::util;

TEST(DateTest, IsValid) {
	DateTime dt;

	EXPECT_EQ(false, dt.isValid());

	dt = DateTime::now();
	EXPECT_EQ(true, dt.isValid());
	EXPECT_EQ(2015, dt.toTm().tm_year + 1900);
}

TEST(DateTest, ParseUTC) {
	std::string sDate = "2014-02-02 15:13:12";
	DateTime dt = DateTime::parseStrUTC(sDate);

	EXPECT_EQ(true, dt.isValid());

	std::tm sTm = dt.toTmUTC();

	EXPECT_EQ(2014, sTm.tm_year + 1900);
	EXPECT_EQ(2,    sTm.tm_mon + 1);
	EXPECT_EQ(2,    sTm.tm_mday);

	EXPECT_EQ(15,   sTm.tm_hour);
	EXPECT_EQ(13,   sTm.tm_min);
	EXPECT_EQ(12,   sTm.tm_sec);

	EXPECT_EQ(sDate, dt.toStringUTC());
}

TEST(DateTest, Parse) {
	std::string sDate = "2014-02-02 15:13:12";
	DateTime dt = DateTime::parseStr(sDate);

	EXPECT_EQ(true, dt.isValid());

	std::tm sTm = dt.toTm();

	EXPECT_EQ(2014, sTm.tm_year + 1900);
	EXPECT_EQ(2,    sTm.tm_mon + 1);
	EXPECT_EQ(2,    sTm.tm_mday);

	EXPECT_EQ(15,   sTm.tm_hour);
	EXPECT_EQ(13,   sTm.tm_min);
	EXPECT_EQ(12,   sTm.tm_sec);

	EXPECT_EQ(sDate, dt.toString());
}
