#include "intrusive_containers.h"
#include "gtest/gtest.h"

using namespace galib;

#define N 100

class DictLink1 {
  public:
    DictLink1(std::string data_)
        : key(data_)
        , data(0) {}

    std::string key;
    int data;

    Link<DictLink1> m_DictLink1;
};

TEST(IntrusivedictionaryTest, Empty) {
    DictLink1 *p1 = new DictLink1("d1");
    DictLink1 *p2 = new DictLink1("d2");

    Dictionary<DictLink1, std::string, &DictLink1::key, &DictLink1::m_DictLink1> dict;
    EXPECT_EQ(true, dict.isEmpty());
    EXPECT_EQ(NULL, dict.get("d1"));
    EXPECT_EQ(true, dict.put(p1));
    EXPECT_EQ(false, dict.isEmpty());
    EXPECT_EQ(p1, dict.get("d1"));
    EXPECT_EQ(NULL, dict.get("d2"));

    delete p1;
    delete p2;
    EXPECT_EQ(true, dict.isEmpty());
}

TEST(IntrusivedictionaryTest, Unlink) {
    DictLink1 *element = new DictLink1("d1");
    Dictionary<DictLink1, std::string, &DictLink1::key, &DictLink1::m_DictLink1> dict1;
    Dictionary<DictLink1, std::string, &DictLink1::key, &DictLink1::m_DictLink1> dict2;

    dict1.put(element);
    EXPECT_EQ(element, dict1.get("d1"));
    EXPECT_EQ(nullptr, dict2.get("d1"));

    element->m_DictLink1.unlink();
    EXPECT_EQ(nullptr, dict1.get("d1"));
    EXPECT_EQ(nullptr, dict2.get("d1"));

    dict2.put(element);
    EXPECT_EQ(nullptr, dict1.get("d1"));
    EXPECT_EQ(element, dict2.get("d1"));

    dict1.put(element);
    EXPECT_EQ(element, dict1.get("d1"));
    EXPECT_EQ(nullptr, dict2.get("d1"));
}

TEST(IntrusivedictionaryTest, NoCollision) {
    DictLink1 *p0 = new DictLink1("generated_id_0");
    DictLink1 *p1 = new DictLink1("generated_id_1");
    DictLink1 *p2 = new DictLink1("generated_id_2");
    DictLink1 *p3 = new DictLink1("generated_id_3");
    DictLink1 *p4 = new DictLink1("generated_id_4");
    DictLink1 *p5 = new DictLink1("generated_id_5");

    Dictionary<DictLink1, std::string, &DictLink1::key, &DictLink1::m_DictLink1> l1(100);
    l1.put(p0);
    l1.put(p1);
    l1.put(p2);
    l1.put(p3);
    l1.put(p4);
    l1.put(p5);

    size_t n = l1.countCollisions();
    EXPECT_EQ(n, 0);

    delete p0;
    delete p1;
    delete p2;
    delete p3;
    delete p4;
    delete p5;
}

TEST(IntrusivedictionaryTest, Collision) {
    Dictionary<DictLink1, std::string, &DictLink1::key, &DictLink1::m_DictLink1> l1(N);

    // Put more than N elements in the dictionary to ensure that a collision will happen
    for (int i = 0; i < 2 * N; i++) {
        l1.put(new DictLink1("generated_id_" + std::to_string(i)));
    }

    size_t n = l1.countCollisions();
    EXPECT_TRUE(n > 0);
}

TEST(IntrusivedictionaryTest, EmptyIterator) {
    DictLink1 *p1 = new DictLink1("d1");
    DictLink1 *p2 = new DictLink1("d2");

    Dictionary<DictLink1, std::string, &DictLink1::key, &DictLink1::m_DictLink1> l1(N);
    l1.put(p1);
    l1.put(p2);

    int i = 0;
    bool d1 = false;
    bool d2 = false;

    Dictionary<DictLink1, std::string, &DictLink1::key, &DictLink1::m_DictLink1>::const_iterator end(l1.end());
    for (Dictionary<DictLink1, std::string, &DictLink1::key, &DictLink1::m_DictLink1>::const_iterator it = l1.begin();
         it != end; it++) {
        d1 |= (it->key == "d1");
        d2 |= (it->key == "d2");
        i++;
    }

    EXPECT_EQ(2, i);
    EXPECT_TRUE(d1);
    EXPECT_TRUE(d2);

    delete p1;
    delete p2;
}
