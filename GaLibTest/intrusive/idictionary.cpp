#include "../test.h"

#include <galib/intrusive/dictionary.h>
#include <galib/util/hash.h>

using namespace galib::intrusive;

#define N 100

class DictLink1 {
public:
	std::string data;
	int nData;

	Link<DictLink1> m_DictLink1;
};

class Link2 : public DictLink1 {
public:
	std::string data2;
	int nData2;

	Link<Link2> m_link2;
};

TEST(IntrusivedictionaryTest, Empty) {
	DictLink1 * p1 = new DictLink1();
	p1->data = "d1";
	DictLink1 * p2 = new DictLink1();
	p2->data = "d2";

	Dictionary<DictLink1, std::string, &DictLink1::data, &DictLink1::m_DictLink1> l1(100);
	EXPECT_EQ(true, l1.isEmpty());
	EXPECT_EQ(NULL, l1.get("d1"));
	EXPECT_EQ(true, l1.put(p1));
	EXPECT_EQ(false, l1.isEmpty());
	EXPECT_EQ(p1, l1.get("d1"));
	EXPECT_EQ(NULL, l1.get("d2"));

	delete p1;
	delete p2;
	EXPECT_EQ(true, l1.isEmpty());

}

TEST(IntrusivedictionaryTest, Collision) {
	DictLink1 * p1 = new DictLink1();
	p1->data = "generated_id_0";
	DictLink1 * p2 = new DictLink1();
	p2->data = "generated_id_1";

	Dictionary<DictLink1, std::string, &DictLink1::data, &DictLink1::m_DictLink1, galib::util::gahash> l1(100);
	l1.put(p1);
	l1.put(p2);

	size_t n = l1.countCollisions();

	delete p1;
	delete p2;

}

TEST(IntrusivedictionaryTest, EmptyIterator) {
	DictLink1 * p1 = new DictLink1();
	p1->data = "d1";
	DictLink1 * p2 = new DictLink1();
	p2->data = "d2";

	Dictionary<DictLink1, std::string, &DictLink1::data, &DictLink1::m_DictLink1> l1(100);
	l1.put(p1);
	l1.put(p2);

	int i = 0;
	bool d1 = false;
	bool d2 = false;
	
	Dictionary<DictLink1, std::string, &DictLink1::data, &DictLink1::m_DictLink1>::const_iterator end(l1.end());
	for(Dictionary<DictLink1, std::string, &DictLink1::data, &DictLink1::m_DictLink1>::const_iterator it = l1.begin(); it != end; it++) {
		d1 |= (it->data == "d1");
		d2 |= (it->data == "d2");
		i++;
	}

	EXPECT_EQ(2, i);
	EXPECT_TRUE(d1);
	EXPECT_TRUE(d2);


	delete p1;
	delete p2;
}

