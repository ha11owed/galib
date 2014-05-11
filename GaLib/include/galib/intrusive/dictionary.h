#ifndef galib_intrusive_dictionary_h__
#define galib_intrusive_dictionary_h__

#include "link.h"
#include "detail/dict_iterator.h"

namespace galib {

	namespace intrusive {

		// ---- Dictionary ----
		template<typename T, typename K, K T::*TKeyField, Link<T> T::*TLinkField, typename Hash = std::hash<K>, typename Pred = std::equal_to<K>>
		class Dictionary {
		public:
			Dictionary();
			Dictionary(size_t n);
			virtual ~Dictionary();

			bool resize(size_t n);

			T*   get(const K& value) const;
			bool put(T * value);

			size_t countCollisions() const;
			bool isEmpty() const;
			void unlinkAll();
			void deleteAll();

		public:
			// std iterators
			typedef detail::DictionaryIterator<T, T*, T&>                        iterator;
			typedef detail::DictionaryIterator<T, const T*, const T&>            const_iterator;
			typedef ptrdiff_t                                                     difference_type;
			typedef size_t                                                        size_type;
			typedef T                                                             value_type;
			typedef T                                                           * pointer;
			typedef T                                                           & reference;

			iterator       begin();
			iterator       end();
			const_iterator begin() const;
			const_iterator end() const;

			void clear();

		protected:
			Link<T> * m_buckets;
			size_t    m_size;    // MUST always be a power of 2. A minimum of 16 is enforced.
			size_t    m_offset;

			size_t calculateCapacity(size_t initialCapacity);
			size_t indexOf(size_t hash) const { return hash & (m_size - 1); }
			Link<T> * getBucket(const K& key) const;

			// Hide copy-constructor and assignment operator
			Dictionary(const Dictionary &) {};
			Dictionary & operator= (const Dictionary &) { return *this; };
		};


		// ----
		// ---- Dictionary
		// ----
		template<typename T, typename K, K T::*TKeyField, Link<T> T::*TLinkField, typename Hash, typename Pred>
		Dictionary<T, K, TKeyField, TLinkField, Hash, Pred>::Dictionary() {
			m_buckets = new Link<T>[32];
			m_size = 32;

			auto m = TLinkField;
			m_offset = *reinterpret_cast<size_t*>(&m);
		}

		template<typename T, typename K, K T::*TKeyField, Link<T> T::*TLinkField, typename Hash, typename Pred>
		Dictionary<T, K, TKeyField, TLinkField, Hash, Pred>::Dictionary(size_t n) {
			n = calculateCapacity(n);

			m_buckets = new Link<T>[n];
			m_size = n;

			auto m = TLinkField;
			m_offset = *reinterpret_cast<size_t*>(&m);
		}

		template<typename T, typename K, K T::*TKeyField, Link<T> T::*TLinkField, typename Hash, typename Pred>
		bool Dictionary<T, K, TKeyField, TLinkField, Hash, Pred>::resize(size_t n) {
			n = calculateCapacity(n);

			if (!isEmpty())
				return false;

			delete[] m_buckets;
			m_buckets = new Link<T>[n];
			m_size = n;

			return true;
		}

		template<typename T, typename K, K T::*TKeyField, Link<T> T::*TLinkField, typename Hash, typename Pred>
		Dictionary<T, K, TKeyField, TLinkField, Hash, Pred>::~Dictionary() {
			unlinkAll();
			if (NULL != m_buckets)
				delete[] m_buckets;
		}

		template<typename T, typename K, K T::*TKeyField, Link<T> T::*TLinkField, typename Hash, typename Pred>
		size_t Dictionary<T, K, TKeyField, TLinkField, Hash, Pred>::calculateCapacity(size_t initialCapacity) {
			size_t capacity = 16;
			while (capacity < initialCapacity) {
				capacity <<= 1;
			}
			return capacity;
		}

		template<typename T, typename K, K T::*TKeyField, Link<T> T::*TLinkField, typename Hash, typename Pred>
		Link<T>* Dictionary<T, K, TKeyField, TLinkField, Hash, Pred>::getBucket(const K& key) const {
			size_t h = Hash()(key);
			return &(m_buckets[indexOf(h)]);
		}


		template<typename T, typename K, K T::*TKeyField, Link<T> T::*TLinkField, typename Hash, typename Pred>
		bool Dictionary<T, K, TKeyField, TLinkField, Hash, Pred>::isEmpty() const {
			for (size_t i = 0; i < m_size; i++) {
				if (m_buckets[i].next(m_offset) != NULL)
					return false;
			}
			return true;
		}

		template<typename T, typename K, K T::*TKeyField, Link<T> T::*TLinkField, typename Hash, typename Pred>
		size_t Dictionary<T, K, TKeyField, TLinkField, Hash, Pred>::countCollisions() const {
			size_t n = 0;
			for (size_t i = 0; i < m_size; i++) {
				Link<T>* link = &(m_buckets[i]);
				Link<T>* next = link->nextLink();
				if(link != next) {
					next = next->nextLink();
					while (link != next) {
						n++;
						next = next->nextLink();
					}
				}
			}
			return n;
		}

		template<typename T, typename K, K T::*TKeyField, Link<T> T::*TLinkField, typename Hash, typename Pred>
		void Dictionary<T, K, TKeyField, TLinkField, Hash, Pred>::unlinkAll() {
			for (size_t i = 0; i < m_size; i++) {
				Link<T>* link = &(m_buckets[i]);
				Link<T>* next = link->nextLink();
				while (link != next) {
					Link<T>* tmp = next;
					next = next->nextLink();
					tmp->unlink();
				}
			}
		}

		template<typename T, typename K, K T::*TKeyField, Link<T> T::*TLinkField, typename Hash, typename Pred>
		void Dictionary<T, K, TKeyField, TLinkField, Hash, Pred>::deleteAll() {
			for (size_t i = 0; i < m_size; i++) {
				Link<T>* link = &(m_buckets[i]);
				Link<T>* next = link->nextLink();
				while (link != next) {
					Link<T>* tmp = next;
					next = next->nextLink();
					delete tmp->owner(m_offset);
				}
			}
		}

		template<typename T, typename K, K T::*TKeyField, Link<T> T::*TLinkField, typename Hash, typename Pred>
		T* Dictionary<T, K, TKeyField, TLinkField, Hash, Pred>::get(const K& key) const {
			Link<T> * _link = getBucket(key);
			if (NULL != _link) {
				Link<T> *next = _link->nextLink();
				while (next != _link) {
					T * v = Link<T>::getData(next, m_offset);
					if (Pred()(key, v->*TKeyField)) {
						return v;
					}
					next = next->nextLink();
				}
			}
			return NULL;
		}

		template<typename T, typename K, K T::*TKeyField, Link<T> T::*TLinkField, typename Hash, typename Pred>
		bool Dictionary<T, K, TKeyField, TLinkField, Hash, Pred>::put(T * val) {
			Link<T> * _link = getBucket(val->*TKeyField);
			if (NULL == _link)
				return false;

			Link<T> * next = _link->nextLink();
			while (next != _link) {
				T * v = Link<T>::getData(next, m_offset);
				if (Pred()(val->*TKeyField, v->*TKeyField)) {
					return false;
				}
				next = next->nextLink();
			}
			_link->insertBefore(val, m_offset);
			return true;
		}


		// ----
		// ---- Dictionary iterators and std methods
		// ----
		template<typename T, typename K, K T::*TKeyField, Link<T> T::*TLinkField, typename Hash, typename Pred>
		typename Dictionary<T, K, TKeyField, TLinkField, Hash, Pred>::iterator Dictionary<T, K, TKeyField, TLinkField, Hash, Pred>::begin() {
			return iterator(m_buckets, m_buckets + m_size, m_offset, 0);
		}

		template<typename T, typename K, K T::*TKeyField, Link<T> T::*TLinkField, typename Hash, typename Pred>
		typename Dictionary<T, K, TKeyField, TLinkField, Hash, Pred>::iterator Dictionary<T, K, TKeyField, TLinkField, Hash, Pred>::end() {
			return iterator(m_buckets, m_buckets + m_size, m_offset, m_size);
		}

		template<typename T, typename K, K T::*TKeyField, Link<T> T::*TLinkField, typename Hash, typename Pred>
		typename Dictionary<T, K, TKeyField, TLinkField, Hash, Pred>::const_iterator Dictionary<T, K, TKeyField, TLinkField, Hash, Pred>::begin() const {
			return const_iterator(m_buckets, m_buckets + m_size, m_offset, 0);
		}

		template<typename T, typename K, K T::*TKeyField, Link<T> T::*TLinkField, typename Hash, typename Pred>
		typename Dictionary<T, K, TKeyField, TLinkField, Hash, Pred>::const_iterator Dictionary<T, K, TKeyField, TLinkField, Hash, Pred>::end() const {
			return const_iterator(m_buckets, m_buckets + m_size, m_offset, m_size);
		}

		template<typename T, typename K, K T::*TKeyField, Link<T> T::*TLinkField, typename Hash, typename Pred>
		void Dictionary<T, K, TKeyField, TLinkField, Hash, Pred>::clear() {
			unlinkAll();
		}
		
	}
}

#endif
