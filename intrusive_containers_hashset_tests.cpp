#include "intrusive_containers.h"
#include "gtest/gtest.h"

using namespace galib;

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
    SetLink1 *p1 = new SetLink1();
    SetLink1 *p2 = new SetLink1();

    HashSet<SetLink1, &SetLink1::m_SetLink1> l1(100);
    EXPECT_TRUE(l1.isEmpty());
    EXPECT_FALSE(l1.contains(NULL));

    EXPECT_TRUE(l1.put(p1));
    EXPECT_FALSE(l1.isEmpty());
    EXPECT_TRUE(l1.contains(p1));
    EXPECT_FALSE(l1.contains(p2));

    delete p1;
    delete p2;
    EXPECT_EQ(true, l1.isEmpty());
}
