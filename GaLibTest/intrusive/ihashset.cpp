#include "../test.h"

#include <galib/intrusive/hashset.h>

using namespace galib::intrusive;

#define N 100

class SetLink1 {
public:
	std::string data;
	int nData;

	Link<SetLink1> m_SetLink1;
};

class Link2 : public SetLink1 {
public:
	std::string data2;
	int nData2;

	Link<Link2> m_link2;
};

TEST(IntrusiveHashSetTest, Empty) {
	SetLink1 * p1 = new SetLink1();
	SetLink1 * p2 = new SetLink1();

	HashSet<SetLink1, &SetLink1::m_SetLink1> l1(100);
	EXPECT_EQ(true, l1.isEmpty());
	EXPECT_EQ(NULL, l1.contains(NULL));
	EXPECT_EQ(true, l1.put(p1));
	EXPECT_EQ(false, l1.isEmpty());
	EXPECT_EQ(true, l1.contains(p1));
	EXPECT_EQ(false, l1.contains(p2));

	delete p1;
	delete p2;
	EXPECT_EQ(true, l1.isEmpty());

}
