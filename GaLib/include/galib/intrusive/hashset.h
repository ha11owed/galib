#ifndef galib_intrusive_hashset_h__
#define galib_intrusive_hashset_h__

#include "link.h"

namespace galib {

	namespace intrusive {

		// ---- HashSet ----
		template<typename T, Link<T> T::*TLinkField, typename Hash = std::hash<T*>, typename Pred = std::equal_to<T*>>
		class HashSet {
		public:
			HashSet(size_t n);
			virtual ~HashSet();

			bool contains(T * value) const;
			bool put(T * value);

			bool isEmpty() const;
			void unlinkAll();
			void deleteAll();

		protected:
			Link<T> * m_buckets;
			size_t    m_size;
			size_t    m_offset;

			Link<T> * getBucket(T * val) const;

			HashSet();

			// Hide copy-constructor and assignment operator
			HashSet (const HashSet &) {};
			HashSet & operator= (const HashSet &) { return *this; };
		};



		// ----
		// ---- HashSet
		// ----
		template<typename T, Link<T> T::*TLinkField, typename Hash, typename Pred>
		HashSet<T, TLinkField, Hash, Pred>::HashSet(size_t n) {
			ASSERT(n > 0);

			m_buckets = new Link<T>[n];
			m_size = n;

			auto m = TLinkField;
			m_offset = *reinterpret_cast<size_t*>(&m);
		}
		
		template<typename T, Link<T> T::*TLinkField, typename Hash, typename Pred>
		HashSet<T, TLinkField, Hash, Pred>::~HashSet() {
			unlinkAll();
			if(NULL != m_buckets)
				delete[] m_buckets;
		}


		template<typename T, Link<T> T::*TLinkField, typename Hash, typename Pred>
		Link<T>* HashSet<T, TLinkField, Hash, Pred>::getBucket(T * val) const {
			if(NULL == val)
				return NULL;

			size_t h = Hash()(val);
			return &(m_buckets[h % m_size]);
		}


		template<typename T, Link<T> T::*TLinkField, typename Hash, typename Pred>
		bool HashSet<T, TLinkField, Hash, Pred>::isEmpty() const {
			for(size_t i = 0; i<m_size; i++) {
				if(m_buckets[i].next(m_offset) != NULL)
					return false;
			}
			return true;
		}

		template<typename T, Link<T> T::*TLinkField, typename Hash, typename Pred>
		void HashSet<T, TLinkField, Hash, Pred>::unlinkAll() {
			for(size_t i = 0; i<m_size; i++) {
				Link<T>* link = &(m_buckets[i]);
				Link<T>* next = link->nextLink();
				while(link != next) {
					Link<T>* tmp = next;
					next = next->nextLink();
					tmp->unlink();
				}
			}
		}

		template<typename T, Link<T> T::*TLinkField, typename Hash, typename Pred>
		void HashSet<T, TLinkField, Hash, Pred>::deleteAll() {
			for(size_t i = 0; i<m_size; i++) {
				Link<T>* link = &(m_buckets[i]);
				Link<T>* next = link->nextLink();
				while(link != next) {
					Link<T>* tmp = next;
					next = next->nextLink();
					delete tmp->owner(m_offset);
				}
			}
		}
		
		template<typename T, Link<T> T::*TLinkField, typename Hash, typename Pred>
		bool HashSet<T, TLinkField, Hash, Pred>::contains(T * value) const {
			Link<T> * _link = getBucket(value);
			if(NULL != _link) {
				Link<T> *next = _link->nextLink();
				while(next != _link) {
					T * v = Link<T>::getData(next, m_offset);
					if(Pred()(value, v)) {
						return true;
					}
				}
			}
			return false;
		}

		template<typename T, Link<T> T::*TLinkField, typename Hash, typename Pred>
		bool HashSet<T, TLinkField, Hash, Pred>::put(T * value) {
			Link<T> * _link = getBucket(value);
			if(NULL == _link)
				return false;

			Link<T> *next = _link->nextLink();
			while(next != _link) {
				T * v = Link<T>::getData(next, m_offset);
				if(Pred()(value, v)) {
					return false;
				}
			}
			_link->insertBefore(value, m_offset);
			return true;
		}


	}
}

#endif
