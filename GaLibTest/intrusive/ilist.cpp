#include "../test.h"

#include <galib/intrusive/list.h>

using namespace galib::intrusive;

#define N 100

class Link1 {
public:
	std::string data;
	int nData;

	Link<Link1> m_link1;
};

class Link2 : public Link1 {
public:
	std::string data2;
	int nData2;

	Link<Link2> m_link2;
};

TEST(IntrusiveTest, Empty) {
	List<Link1, &Link1::m_link1> l1;
	EXPECT_EQ(true, l1.isEmpty());
	EXPECT_EQ(NULL, l1.head());
	EXPECT_EQ(NULL, l1.tail());
}

TEST(IntrusiveTest, UnlinkAll) {
	List<Link1, &Link1::m_link1> l1;
	List<Link1, &Link1::m_link1> l2;
	
	Link1 * items1[N];
	Link1 * items2[N];
	for(int i=0; i<N; i++) {
		items1[i] = new Link1;
		items2[i] = new Link1;

		l1.insertHead(items1[i]);
		l2.insertTail(items2[i]);

		EXPECT_EQ(items1[i], l1.head());
		EXPECT_EQ(items2[i], l2.tail());
	}

	l1.unlinkAll();
	l2.unlinkAll();
	EXPECT_EQ(true, l1.isEmpty());
	EXPECT_EQ(true, l2.isEmpty());


	for(int i=0; i<N; i++) {
		delete items1[i];
		delete items2[i];
	}
}

TEST(IntrusiveTest, DeleteAll) {
	List<Link1, &Link1::m_link1> l1;
	List<Link1, &Link1::m_link1> l2;
	
	Link1 * items1[N];
	Link1 * items2[N];
	for(int i=0; i<N; i++) {
		items1[i] = new Link1;
		items2[i] = new Link1;

		l1.insertHead(items1[i]);
		l2.insertTail(items2[i]);

		EXPECT_EQ(items1[i], l1.head());
		EXPECT_EQ(items2[i], l2.tail());
	}

	l1.deleteAll();
	l2.deleteAll();
	EXPECT_EQ(true, l1.isEmpty());
	EXPECT_EQ(true, l2.isEmpty());
}


TEST(IntrusiveTest, NodeInsertAfter1) {
	List<Link1, &Link1::m_link1> l1;

	Link1 *n1 = new Link1;
	n1->data = "node1";
	Link1 *n2 = new Link1;
	n2->data = "node2";
	Link1 *n3 = new Link1;
	n3->data = "node3";
	
	l1.insertHead(n3);
	l1.insertHead(n2);
	l1.insertHead(n1);

	EXPECT_EQ(n1, l1.head());
	EXPECT_EQ(n3, l1.tail());

	delete n2;
	delete n3;
	EXPECT_EQ(n1, l1.head());
	EXPECT_EQ(n1, l1.tail());

	delete n1;
	EXPECT_EQ(NULL, l1.head());
	EXPECT_EQ(NULL, l1.tail());
}

TEST(IntrusiveTest, Iterate) {
	List<Link1, &Link1::m_link1> l1;

	Link1 *n1 = new Link1;
	n1->data = "node1";
	Link1 *n2 = new Link1;
	n2->data = "node2";
	Link1 *n3 = new Link1;
	n3->data = "node3";
	
	l1.insertHead(n3);
	l1.insertHead(n2);
	l1.insertHead(n1);

	EXPECT_EQ(n1, l1.head());
	EXPECT_EQ(n3, l1.tail());

	delete n2;
	delete n3;
	EXPECT_EQ(n1, l1.head());
	EXPECT_EQ(n1, l1.tail());

	delete n1;
	EXPECT_EQ(NULL, l1.head());
	EXPECT_EQ(NULL, l1.tail());
}

TEST(IntrusiveTest, STDIterate) {
	List<Link1, &Link1::m_link1> l1;

	Link1 *n1 = new Link1;
	n1->data = "node1";
	Link1 *n2 = new Link1;
	n2->data = "node2";
	Link1 *n3 = new Link1;
	n3->data = "node3";
	
	l1.insertHead(n3);
	l1.insertHead(n2);
	l1.insertHead(n1);
	

	int i = 0;
	for(List<Link1, &Link1::m_link1>::const_iterator it = l1.begin(); it != l1.end(); it++) {
		if(i == 0)
			EXPECT_EQ("node1", it->data);
		if(i == 1)
			EXPECT_EQ("node2", it->data);
		if(i == 2)
			EXPECT_EQ("node3", it->data);

		i++;
	}

	EXPECT_EQ(3, i);


	l1.deleteAll();
}


