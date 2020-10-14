#pragma once

#ifndef assert
#define assert(x) (static_cast<void>(0))
#define _u_needed_to_undefine_assert
#endif

#include <iterator>

namespace galib {

// --------------
// ---- Link ----
// --------------
template <typename T> class Link {
  public:
    Link();
    ~Link();

    bool isLinked() const;
    void unlink();
    void insertAfter(Link<T> *link);
    void insertBefore(Link<T> *link);
    void insertAfter(T *node, std::size_t offset);
    void insertBefore(T *node, std::size_t offset);

    Link<T> *prevLink() const;
    Link<T> *nextLink() const;

    T *prev(std::size_t offset) const;
    T *next(std::size_t offset) const;
    T *owner(std::size_t offset) const;

    static Link<T> *getLink(T *data, std::size_t offset);
    static T *getData(const Link<T> *link, std::size_t offset);

  private:
    Link<T> *m_next;
    Link<T> *m_prev;

    void removeFromList();

    // Hide copy-constructor and assignment operator
    Link(const Link &);
    Link &operator=(const Link &);
};

// --------------
// ---- Link ----
// --------------
template <typename T> Link<T>::Link() { m_next = m_prev = this; }

template <typename T> Link<T>::~Link() { unlink(); }

template <typename T> Link<T> *Link<T>::getLink(T *data, std::size_t offset) {
    assert(data != nullptr);
    return reinterpret_cast<Link<T> *>(reinterpret_cast<std::size_t>(data) + offset);
}

template <typename T> T *Link<T>::getData(const Link<T> *link, std::size_t offset) {
    assert(link != nullptr);
    return reinterpret_cast<T *>(reinterpret_cast<std::size_t>(link) - offset);
}

template <typename T> bool Link<T>::isLinked() const { return m_next != this; }

template <typename T> void Link<T>::removeFromList() {
    m_next->m_prev = m_prev;
    m_prev->m_next = m_next;
}

template <typename T> void Link<T>::unlink() {
    removeFromList();
    m_next = m_prev = this;
}

template <typename T> void Link<T>::insertAfter(Link<T> *link) {
    assert(link != nullptr);
    link->removeFromList();

    link->m_next = m_next;
    link->m_prev = this;
    m_next->m_prev = link;
    m_next = link;
}

template <typename T> void Link<T>::insertBefore(Link<T> *link) {
    assert(link != nullptr);
    link->removeFromList();

    link->m_prev = m_prev;
    link->m_next = this;
    m_prev->m_next = link;
    m_prev = link;
}

template <typename T> void Link<T>::insertAfter(T *node, std::size_t offset) {
    assert(node != nullptr);
    insertAfter(getLink(node, offset));
}

template <typename T> void Link<T>::insertBefore(T *node, std::size_t offset) {
    assert(node != nullptr);
    insertBefore(getLink(node, offset));
}

template <typename T> Link<T> *Link<T>::nextLink() const { return m_next; }

template <typename T> Link<T> *Link<T>::prevLink() const { return m_prev; }

template <typename T> T *Link<T>::next(std::size_t offset) const {
    if (m_next == this) {
        return nullptr;
    }
    return getData(m_next, offset);
}

template <typename T> T *Link<T>::prev(std::size_t offset) const {
    if (m_prev == this) {
        return nullptr;
    }
    return getData(m_prev, offset);
}

template <typename T> T *Link<T>::owner(std::size_t offset) const { return getData(this, offset); }

namespace detail {

template <typename T, typename TPointer, typename TReference>
class ListIterator : public std::iterator<std::bidirectional_iterator_tag, T, TPointer, TReference> {
  public:
    ListIterator(Link<T> *startLink, size_t offset, T *item) {
        m_startLink = startLink;
        m_offset = offset;
        m_currentItem = item;
    }

    // NOTE: the two constructors and the friend are needed in order to allow conversion from one type to the other
    friend class ListIterator<T, const T *, const T &>;

    ListIterator(const ListIterator<T, T *, T &> &other)
        : m_startLink(other.m_startLink)
        , m_offset(other.m_offset)
        , m_currentItem(other.m_currentItem) {}

    ListIterator(const ListIterator<T, const T *, const T &> &other)
        : m_startLink(other.m_startLink)
        , m_offset(other.m_offset)
        , m_currentItem(other.m_currentItem) {}

    TReference operator*() {
        assert(m_currentItem != nullptr);
        return *m_currentItem;
    }

    TPointer operator->() {
        assert(m_currentItem != nullptr);
        return m_currentItem;
    }

    const ListIterator &operator--() {
        Link<T> *prev;
        if (m_currentItem != nullptr) {
            Link<T> *current = Link<T>::getLink(m_currentItem, m_offset);
            prev = current->prevLink();
        } else {
            prev = m_startLink->prevLink();
        }

        if (prev != m_startLink) {
            m_currentItem = Link<T>::getData(prev, m_offset);
        } else {
            m_currentItem = nullptr;
        }

        return *this;
    }

    ListIterator operator--(int) {
        // Use operator--()
        const ListIterator old(*this);
        --(*this);
        return old;
    }

    const ListIterator &operator++() {
        if (m_currentItem != nullptr) {
            Link<T> *current = Link<T>::getLink(m_currentItem, m_offset);
            Link<T> *next = current->nextLink();
            if (next != m_startLink) {
                m_currentItem = Link<T>::getData(next, m_offset);
            } else {
                m_currentItem = nullptr;
            }
        }
        return *this;
    }

    ListIterator operator++(int) {
        // Use operator++()
        const ListIterator old(*this);
        ++(*this);
        return old;
    }

    bool operator!=(const ListIterator &other) const { return !(*this == other); }

    bool operator==(const ListIterator &other) const {
        return (m_currentItem == other.m_currentItem) && (m_offset == other.m_offset);
    }

  protected:
    Link<T> *m_startLink;
    size_t m_offset;
    T *m_currentItem;
};

template <typename T, typename TPointer, typename TReference>
class DictionaryIterator : public std::iterator<std::bidirectional_iterator_tag, T, TPointer, TReference> {
  public:
    DictionaryIterator(Link<T> *begin, Link<T> *end, size_t offset, size_t index) {
        m_begin = begin;
        m_end = end;
        m_offset = offset;

        m_currentBucket = begin + index;
        m_currentItem = nullptr;
        updateNextBucket();
    }

    // NOTE: the two constructors and the friend are needed in order to allow conversion from one type to the other
    friend class DictionaryIterator<T, const T *, const T &>;

    DictionaryIterator(const DictionaryIterator<T, T *, T &> &other)
        : m_begin(other.m_begin)
        , m_end(other.m_end)
        , m_offset(other.m_offset)
        , m_currentBucket(other.m_currentBucket)
        , m_currentItem(other.m_currentItem) {}

    DictionaryIterator(const DictionaryIterator<T, const T *, const T &> &other)
        : m_begin(other.m_begin)
        , m_end(other.m_end)
        , m_offset(other.m_offset)
        , m_currentBucket(other.m_currentBucket)
        , m_currentItem(other.m_currentItem) {}

    TReference operator*() {
        assert(m_currentItem != nullptr);
        return *m_currentItem;
    }

    TPointer operator->() {
        assert(m_currentItem != nullptr);
        return m_currentItem;
    }

    const DictionaryIterator &operator--() { return *this; }

    DictionaryIterator operator--(int) {
        // Use operator--()
        const DictionaryIterator old(*this);
        --(*this);
        return old;
    }

    const DictionaryIterator &operator++() {
        if (m_currentBucket != m_end) {
            Link<T> *current = Link<T>::getLink(m_currentItem, m_offset);
            Link<T> *next = current->nextLink();
            if (next == m_currentBucket) {
                // There is no next
                m_currentItem = nullptr;
                m_currentBucket++;
                updateNextBucket();
            } else {
                m_currentItem = Link<T>::getData(next, m_offset);
            }
        }
        return *this;
    }

    DictionaryIterator operator++(int) {
        // Use operator++()
        const DictionaryIterator old(*this);
        ++(*this);
        return old;
    }

    bool operator!=(const DictionaryIterator &other) const { return !(*this == other); }

    bool operator==(const DictionaryIterator &other) const {
        return (m_currentBucket == other.m_currentBucket) && (m_currentItem == other.m_currentItem) &&
               (m_offset == other.m_offset);
    }

  protected:
    void updatePrevBucket() {
        while (m_currentItem == nullptr && m_currentBucket != m_begin) {
            Link<T> *prev = m_currentBucket->prevLink();
            if (prev != m_currentBucket) {
                m_currentItem = Link<T>::getData(prev, m_offset);
            } else {
                m_currentBucket--;
            }
        }
    }

    void updateNextBucket() {
        while (m_currentItem == nullptr && m_currentBucket != m_end) {
            Link<T> *next = m_currentBucket->nextLink();
            if (next != m_currentBucket) {
                m_currentItem = Link<T>::getData(next, m_offset);
            } else {
                m_currentBucket++;
            }
        }
    }

    Link<T> *m_begin;
    Link<T> *m_end;
    size_t m_offset;

    Link<T> *m_currentBucket;
    T *m_currentItem;
};

} // namespace detail

/// @brief Intrusive linked list.
/// @example Item will be contained in (up to) two linked lists:
/// struct Item {
///   Link<Item> _all;
///   Link<Item> _used;
///   ...
/// };
/// List<Item, &Item::_all> allItems;
/// List<Item, &Item::_used> usedItems;
template <typename T, Link<T> T::*TLinkField> class List {
  public:
    List();
    List(size_t offset);
    virtual ~List();

    bool isEmpty() const;
    void unlinkAll();
    void deleteAll();

    void insertHead(T *node);
    void insertTail(T *node);
    void insertBefore(T *node, T *before);
    void insertAfter(T *node, T *after);

    T *head() const;
    T *tail() const;
    T *next(T *node) const;
    T *prev(T *node) const;

  public:
    // std iterators
    typedef detail::ListIterator<T, T *, T &> iterator;
    typedef detail::ListIterator<T, const T *, const T &> const_iterator;
    typedef ptrdiff_t difference_type;
    typedef size_t size_type;
    typedef T value_type;
    typedef T *pointer;
    typedef T &reference;

    iterator begin();
    iterator end();
    const_iterator begin() const;
    const_iterator end() const;

    void clear();

  protected:
    Link<T> m_link;
    size_t m_offset;

    // Hide copy-constructor and assignment operator
    List(const List &) {}
    List &operator=(const List &) { return *this; }
};

// --------------
// ---- List ----
// --------------
template <typename T, Link<T> T::*TLinkField> List<T, TLinkField>::List() {
    auto m = TLinkField;
    m_offset = *reinterpret_cast<size_t *>(&m);
}

template <typename T, Link<T> T::*TLinkField>
List<T, TLinkField>::List(size_t offset)
    : m_offset(offset) {}

template <typename T, Link<T> T::*TLinkField> List<T, TLinkField>::~List() { unlinkAll(); }

template <typename T, Link<T> T::*TLinkField> bool List<T, TLinkField>::isEmpty() const {
    return m_link.next(m_offset) == nullptr;
}

template <typename T, Link<T> T::*TLinkField> void List<T, TLinkField>::unlinkAll() {
    Link<T> *link = m_link.nextLink();
    while (link != &m_link) {
        Link<T> *tmp = link;
        link = link->nextLink();
        tmp->unlink();
    }
}

template <typename T, Link<T> T::*TLinkField> void List<T, TLinkField>::deleteAll() {
    Link<T> *link = m_link.nextLink();
    while (link != &m_link) {
        Link<T> *tmp = link;
        link = link->nextLink();
        delete tmp->owner(m_offset);
    }
}

template <typename T, Link<T> T::*TLinkField> void List<T, TLinkField>::insertHead(T *node) {
    m_link.insertAfter(node, m_offset);
}

template <typename T, Link<T> T::*TLinkField> void List<T, TLinkField>::insertTail(T *node) {
    m_link.insertBefore(node, m_offset);
}

template <typename T, Link<T> T::*TLinkField> void List<T, TLinkField>::insertBefore(T *node, T *before) {
    if (nullptr == before) {
        m_link.insertBefore(node, m_offset);
    } else {
        Link<T> *link = Link<T>::getLink(before, m_offset);
        link->insertBefore(node, m_offset);
    }
}

template <typename T, Link<T> T::*TLinkField> void List<T, TLinkField>::insertAfter(T *node, T *after) {
    if (nullptr == after) {
        m_link.insertBefore(node, m_offset);
    } else {
        Link<T> *link = Link<T>::getLink(after, m_offset);
        link->insertAfter(node, m_offset);
    }
}

template <typename T, Link<T> T::*TLinkField> T *List<T, TLinkField>::head() const {
    Link<T> *next = m_link.nextLink();
    if (next == &m_link) {
        return nullptr;
    }

    return Link<T>::getData(next, m_offset);
}

template <typename T, Link<T> T::*TLinkField> T *List<T, TLinkField>::tail() const {
    Link<T> *prev = m_link.prevLink();
    if (prev == &m_link) {
        return nullptr;
    }

    return Link<T>::getData(prev, m_offset);
}

template <typename T, Link<T> T::*TLinkField> T *List<T, TLinkField>::next(T *node) const {
    if (node == nullptr) {
        return nullptr;
    }

    Link<T> *next = Link<T>::getLink(node, m_offset)->nextLink();
    if (next == &m_link) {
        return nullptr;
    }

    return Link<T>::getData(next, m_offset);
}

template <typename T, Link<T> T::*TLinkField> T *List<T, TLinkField>::prev(T *node) const {
    if (node == nullptr) {
        return nullptr;
    }

    Link<T> *prev = Link<T>::getLink(node, m_offset)->prevLink();
    if (prev == &m_link) {
        return nullptr;
    }

    return Link<T>::getData(prev, m_offset);
}

// ------------------------
// ---- List iterators ----
// ------------------------
template <typename T, Link<T> T::*TLinkField> typename List<T, TLinkField>::iterator List<T, TLinkField>::begin() {
    return iterator(&m_link, m_offset, head());
}

template <typename T, Link<T> T::*TLinkField> typename List<T, TLinkField>::iterator List<T, TLinkField>::end() {
    return iterator(&m_link, m_offset, nullptr);
}

template <typename T, Link<T> T::*TLinkField>
typename List<T, TLinkField>::const_iterator List<T, TLinkField>::begin() const {
    return const_iterator(&m_link, m_offset, head());
}

template <typename T, Link<T> T::*TLinkField>
typename List<T, TLinkField>::const_iterator List<T, TLinkField>::end() const {
    return const_iterator(&m_link, m_offset, nullptr);
}

template <typename T, Link<T> T::*TLinkField> void List<T, TLinkField>::clear() { unlinkAll(); }

/// @brief Intrusive HashSet.
/// @example Item will be contained in (up to) two hashsets:
/// struct Item {
///   Link<Item> _all;
///   Link<Item> _used;
///   ...
/// };
/// HashSet<Item, &Item::_all> allItems;
/// HashSet<Item, &Item::_used> usedItems;
template <typename T, Link<T> T::*TLinkField, typename Hash = std::hash<T *>, typename Pred = std::equal_to<T *>>
class HashSet {
  public:
    HashSet(size_t n);
    virtual ~HashSet();

    bool contains(T *value) const;
    bool put(T *value);

    bool isEmpty() const;
    void unlinkAll();
    void deleteAll();

  protected:
    Link<T> *m_buckets;
    size_t m_size;
    size_t m_offset;

    Link<T> *getBucket(T *val) const;

    HashSet();

    // Hide copy-constructor and assignment operator
    HashSet(const HashSet &) {}
    HashSet &operator=(const HashSet &) { return *this; }
};

// -----------------
// ---- HashSet ----
// -----------------
template <typename T, Link<T> T::*TLinkField, typename Hash, typename Pred>
HashSet<T, TLinkField, Hash, Pred>::HashSet(size_t n) {
    assert(n > 0);

    m_buckets = new Link<T>[n];
    m_size = n;

    auto m = TLinkField;
    m_offset = *reinterpret_cast<size_t *>(&m);
}

template <typename T, Link<T> T::*TLinkField, typename Hash, typename Pred>
HashSet<T, TLinkField, Hash, Pred>::~HashSet() {
    unlinkAll();
    if (nullptr != m_buckets) {
        delete[] m_buckets;
        m_buckets = nullptr;
    }
}

template <typename T, Link<T> T::*TLinkField, typename Hash, typename Pred>
Link<T> *HashSet<T, TLinkField, Hash, Pred>::getBucket(T *val) const {
    if (nullptr == val) {
        return nullptr;
    }

    size_t h = Hash()(val);
    return &(m_buckets[h % m_size]);
}

template <typename T, Link<T> T::*TLinkField, typename Hash, typename Pred>
bool HashSet<T, TLinkField, Hash, Pred>::isEmpty() const {
    for (size_t i = 0; i < m_size; i++) {
        if (m_buckets[i].next(m_offset) != nullptr) {
            return false;
        }
    }
    return true;
}

template <typename T, Link<T> T::*TLinkField, typename Hash, typename Pred>
void HashSet<T, TLinkField, Hash, Pred>::unlinkAll() {
    for (size_t i = 0; i < m_size; i++) {
        Link<T> *link = &(m_buckets[i]);
        Link<T> *next = link->nextLink();
        while (link != next) {
            Link<T> *tmp = next;
            next = next->nextLink();
            tmp->unlink();
        }
    }
}

template <typename T, Link<T> T::*TLinkField, typename Hash, typename Pred>
void HashSet<T, TLinkField, Hash, Pred>::deleteAll() {
    for (size_t i = 0; i < m_size; i++) {
        Link<T> *link = &(m_buckets[i]);
        Link<T> *next = link->nextLink();
        while (link != next) {
            Link<T> *tmp = next;
            next = next->nextLink();
            delete tmp->owner(m_offset);
        }
    }
}

template <typename T, Link<T> T::*TLinkField, typename Hash, typename Pred>
bool HashSet<T, TLinkField, Hash, Pred>::contains(T *value) const {
    Link<T> *link = getBucket(value);
    if (nullptr != link) {
        Link<T> *next = link->nextLink();
        while (next != link) {
            T *v = Link<T>::getData(next, m_offset);
            if (Pred()(value, v)) {
                return true;
            }
        }
    }
    return false;
}

template <typename T, Link<T> T::*TLinkField, typename Hash, typename Pred>
bool HashSet<T, TLinkField, Hash, Pred>::put(T *value) {
    Link<T> *link = getBucket(value);
    if (nullptr == link) {
        return false;
    }

    Link<T> *next = link->nextLink();
    while (next != link) {
        T *v = Link<T>::getData(next, m_offset);
        if (Pred()(value, v)) {
            return false;
        }
    }
    link->insertBefore(value, m_offset);
    return true;
}

/// @brief Intrusive Dictionary.
/// @example Item can be searched by key in the dictionary:
/// struct Item {
///   int key;
///   Link<Item> _link;
///   ...
/// };
/// Dictionary<Item, int, &Item::key, &Item::_link> dict;
template <typename T, typename K, K T::*TKeyField, Link<T> T::*TLinkField, typename Hash = std::hash<K>,
          typename Pred = std::equal_to<K>>
class Dictionary {
  public:
    Dictionary();
    Dictionary(size_t n);
    virtual ~Dictionary();

    bool resize(size_t n);

    T *get(const K &value) const;
    bool put(T *value);

    size_t countCollisions() const;
    bool isEmpty() const;
    void unlinkAll();
    void deleteAll();

  public:
    // std iterators
    typedef detail::DictionaryIterator<T, T *, T &> iterator;
    typedef detail::DictionaryIterator<T, const T *, const T &> const_iterator;
    typedef ptrdiff_t difference_type;
    typedef size_t size_type;
    typedef T value_type;
    typedef T *pointer;
    typedef T &reference;

    iterator begin();
    iterator end();
    const_iterator begin() const;
    const_iterator end() const;

    void clear();

  protected:
    Link<T> *m_buckets;
    size_t m_size; // MUST always be a power of 2. A minimum of 16 is enforced.
    size_t m_offset;

    size_t calculateCapacity(size_t initialCapacity);
    Link<T> *getBucket(const K &key) const;

    // Hide copy-constructor and assignment operator
    Dictionary(const Dictionary &) {}
    Dictionary &operator=(const Dictionary &) { return *this; }
};

// --------------------
// ---- Dictionary ----
// --------------------
template <typename T, typename K, K T::*TKeyField, Link<T> T::*TLinkField, typename Hash, typename Pred>
Dictionary<T, K, TKeyField, TLinkField, Hash, Pred>::Dictionary() {
    m_buckets = new Link<T>[32];
    m_size = 32;

    auto m = TLinkField;
    m_offset = *reinterpret_cast<size_t *>(&m);
}

template <typename T, typename K, K T::*TKeyField, Link<T> T::*TLinkField, typename Hash, typename Pred>
Dictionary<T, K, TKeyField, TLinkField, Hash, Pred>::Dictionary(size_t n) {
    n = calculateCapacity(n);

    m_buckets = new Link<T>[n];
    m_size = n;

    auto m = TLinkField;
    m_offset = *reinterpret_cast<size_t *>(&m);
}

template <typename T, typename K, K T::*TKeyField, Link<T> T::*TLinkField, typename Hash, typename Pred>
bool Dictionary<T, K, TKeyField, TLinkField, Hash, Pred>::resize(size_t n) {
    n = calculateCapacity(n);

    if (!isEmpty()) {
        return false;
    }

    delete[] m_buckets;
    m_buckets = new Link<T>[n];
    m_size = n;

    return true;
}

template <typename T, typename K, K T::*TKeyField, Link<T> T::*TLinkField, typename Hash, typename Pred>
Dictionary<T, K, TKeyField, TLinkField, Hash, Pred>::~Dictionary() {
    unlinkAll();
    if (nullptr != m_buckets) {
        delete[] m_buckets;
        m_buckets = nullptr;
    }
}

template <typename T, typename K, K T::*TKeyField, Link<T> T::*TLinkField, typename Hash, typename Pred>
size_t Dictionary<T, K, TKeyField, TLinkField, Hash, Pred>::calculateCapacity(size_t initialCapacity) {
    size_t capacity = 16;
    while (capacity < initialCapacity) {
        capacity <<= 1;
    }
    return capacity;
}

template <typename T, typename K, K T::*TKeyField, Link<T> T::*TLinkField, typename Hash, typename Pred>
Link<T> *Dictionary<T, K, TKeyField, TLinkField, Hash, Pred>::getBucket(const K &key) const {
    size_t h = Hash()(key);
    return &(m_buckets[h & (m_size - 1)]);
}

template <typename T, typename K, K T::*TKeyField, Link<T> T::*TLinkField, typename Hash, typename Pred>
bool Dictionary<T, K, TKeyField, TLinkField, Hash, Pred>::isEmpty() const {
    for (size_t i = 0; i < m_size; i++) {
        if (m_buckets[i].next(m_offset) != nullptr) {
            return false;
        }
    }
    return true;
}

template <typename T, typename K, K T::*TKeyField, Link<T> T::*TLinkField, typename Hash, typename Pred>
size_t Dictionary<T, K, TKeyField, TLinkField, Hash, Pred>::countCollisions() const {
    size_t n = 0;
    for (size_t i = 0; i < m_size; i++) {
        Link<T> *link = &(m_buckets[i]);
        Link<T> *next = link->nextLink();
        if (link != next) {
            next = next->nextLink();
            while (link != next) {
                n++;
                next = next->nextLink();
            }
        }
    }
    return n;
}

template <typename T, typename K, K T::*TKeyField, Link<T> T::*TLinkField, typename Hash, typename Pred>
void Dictionary<T, K, TKeyField, TLinkField, Hash, Pred>::unlinkAll() {
    for (size_t i = 0; i < m_size; i++) {
        Link<T> *link = &(m_buckets[i]);
        Link<T> *next = link->nextLink();
        while (link != next) {
            Link<T> *tmp = next;
            next = next->nextLink();
            tmp->unlink();
        }
    }
}

template <typename T, typename K, K T::*TKeyField, Link<T> T::*TLinkField, typename Hash, typename Pred>
void Dictionary<T, K, TKeyField, TLinkField, Hash, Pred>::deleteAll() {
    for (size_t i = 0; i < m_size; i++) {
        Link<T> *link = &(m_buckets[i]);
        Link<T> *next = link->nextLink();
        while (link != next) {
            Link<T> *tmp = next;
            next = next->nextLink();
            delete tmp->owner(m_offset);
        }
    }
}

template <typename T, typename K, K T::*TKeyField, Link<T> T::*TLinkField, typename Hash, typename Pred>
T *Dictionary<T, K, TKeyField, TLinkField, Hash, Pred>::get(const K &key) const {
    Link<T> *link = getBucket(key);
    Link<T> *next = link->nextLink();
    while (next != link) {
        T *v = Link<T>::getData(next, m_offset);
        if (Pred()(key, v->*TKeyField)) {
            return v;
        }
        next = next->nextLink();
    }
    return nullptr;
}

template <typename T, typename K, K T::*TKeyField, Link<T> T::*TLinkField, typename Hash, typename Pred>
bool Dictionary<T, K, TKeyField, TLinkField, Hash, Pred>::put(T *val) {
    Link<T> *link = getBucket(val->*TKeyField);
    Link<T> *next = link->nextLink();
    while (next != link) {
        T *v = Link<T>::getData(next, m_offset);
        if (Pred()(val->*TKeyField, v->*TKeyField)) {
            return false;
        }
        next = next->nextLink();
    }
    link->insertBefore(val, m_offset);
    return true;
}

// ----------------------------------------------
// ---- Dictionary iterators and std methods ----
// ----------------------------------------------
template <typename T, typename K, K T::*TKeyField, Link<T> T::*TLinkField, typename Hash, typename Pred>
typename Dictionary<T, K, TKeyField, TLinkField, Hash, Pred>::iterator
Dictionary<T, K, TKeyField, TLinkField, Hash, Pred>::begin() {
    return iterator(m_buckets, m_buckets + m_size, m_offset, 0);
}

template <typename T, typename K, K T::*TKeyField, Link<T> T::*TLinkField, typename Hash, typename Pred>
typename Dictionary<T, K, TKeyField, TLinkField, Hash, Pred>::iterator
Dictionary<T, K, TKeyField, TLinkField, Hash, Pred>::end() {
    return iterator(m_buckets, m_buckets + m_size, m_offset, m_size);
}

template <typename T, typename K, K T::*TKeyField, Link<T> T::*TLinkField, typename Hash, typename Pred>
typename Dictionary<T, K, TKeyField, TLinkField, Hash, Pred>::const_iterator
Dictionary<T, K, TKeyField, TLinkField, Hash, Pred>::begin() const {
    return const_iterator(m_buckets, m_buckets + m_size, m_offset, 0);
}

template <typename T, typename K, K T::*TKeyField, Link<T> T::*TLinkField, typename Hash, typename Pred>
typename Dictionary<T, K, TKeyField, TLinkField, Hash, Pred>::const_iterator
Dictionary<T, K, TKeyField, TLinkField, Hash, Pred>::end() const {
    return const_iterator(m_buckets, m_buckets + m_size, m_offset, m_size);
}

template <typename T, typename K, K T::*TKeyField, Link<T> T::*TLinkField, typename Hash, typename Pred>
void Dictionary<T, K, TKeyField, TLinkField, Hash, Pred>::clear() {
    unlinkAll();
}

} // namespace galib

#ifdef _u_needed_to_undefine_assert
#undef assert
#undef _u_needed_to_undefine_assert
#endif
