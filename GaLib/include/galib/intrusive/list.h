#ifndef galib_intrusive_list_h__
#define galib_intrusive_list_h__

#include "link.h"
#include "detail/list_iterator.h"

namespace galib {

	namespace intrusive {


		// ---- List ----
		template<typename T, Link<T> T::*TLinkField>
		class List {
		public:
			List();
			List(size_t offset);
			virtual ~List();

			bool isEmpty() const;
			void unlinkAll();
			void deleteAll();

			void insertHead(T * node);
			void insertTail(T * node);
			void insertBefore(T * node, T * before);
			void insertAfter(T * node, T * after);

			T* head() const;
			T* tail() const;
			T* next(T * node) const;
			T* prev(T * node) const;

		public:
			// std iterators
			typedef detail::ListIterator<T, T*, T&>                  iterator;
			typedef detail::ListIterator<T, const T*, const T&>      const_iterator;
			typedef ptrdiff_t                                        difference_type;
			typedef size_t                                           size_type;
			typedef T                                                value_type;
			typedef T                                              * pointer;
			typedef T                                              & reference;

			iterator       begin();
			iterator       end();
			const_iterator begin() const;
			const_iterator end() const;

			void clear();

		protected:
			Link<T> m_link;
			size_t  m_offset;

			// Hide copy-constructor and assignment operator
			List (const List &) {};
			List & operator= (const List &) { return *this; };
		};


		// ----
		// ---- List
		// ----
		template<typename T, Link<T> T::*TLinkField>
		List<T, TLinkField>::List() {
			auto m = TLinkField;
			m_offset = *reinterpret_cast<size_t*>(&m);
		}
				
		template<typename T, Link<T> T::*TLinkField>
		List<T, TLinkField>::List(size_t offset) : m_offset(offset) {
		}

		template<typename T, Link<T> T::*TLinkField>
		List<T, TLinkField>::~List() {
			unlinkAll();
		}

		template<typename T, Link<T> T::*TLinkField>
		bool List<T, TLinkField>::isEmpty() const {
			return m_link.next(m_offset) == NULL;
		}

		template<typename T, Link<T> T::*TLinkField>
		void List<T, TLinkField>::unlinkAll() {
			Link<T>* link = m_link.nextLink();
			while(link != &m_link) {
				Link<T>* tmp = link;
				link = link->nextLink();
				tmp->unlink();
			}
		}

		template<typename T, Link<T> T::*TLinkField>
		void List<T, TLinkField>::deleteAll() {
			Link<T>* link = m_link.nextLink();
			while(link != &m_link) {
				Link<T>* tmp = link;
				link = link->nextLink();
				delete tmp->owner(m_offset);
			}
		}
		
		template<typename T, Link<T> T::*TLinkField>
		void List<T, TLinkField>::insertHead(T * node) {
			m_link.insertAfter(node, m_offset);
		}

		template<typename T, Link<T> T::*TLinkField>
		void List<T, TLinkField>::insertTail(T * node) {
			m_link.insertBefore(node, m_offset);
		}

		template<typename T, Link<T> T::*TLinkField>
		void List<T, TLinkField>::insertBefore(T * node, T * before) {
			if(NULL == before)
				m_link.insertBefore(node, m_offset);
			else
				before->insertBefore(node, m_offset);
		}

		template<typename T, Link<T> T::*TLinkField>
		void List<T, TLinkField>::insertAfter(T * node, T * after) {
			if(NULL == after)
				m_link.insertBefore(node, m_offset);
			else
				after->insertAfter(node, m_offset);
		}

		template<typename T, Link<T> T::*TLinkField>
		T* List<T, TLinkField>::head() const {
			Link<T> *next = m_link.nextLink();
			if(next == &m_link) 
				return NULL;

			return Link<T>::getData(next, m_offset);
		}

		template<typename T, Link<T> T::*TLinkField>
		T* List<T, TLinkField>::tail() const {
			Link<T> *prev = m_link.prevLink();
			if(prev == &m_link) 
				return NULL;

			return Link<T>::getData(prev, m_offset);
		}

		template<typename T, Link<T> T::*TLinkField>
		T* List<T, TLinkField>::next(T * node) const {
			if(node == NULL)
				return NULL;

			Link<T> *next = Link<T>::getLink(node, m_offset)->nextLink();
			if(next == &m_link) 
				return NULL;

			return Link<T>::getData(next, m_offset);
		}

		template<typename T, Link<T> T::*TLinkField>
		T* List<T, TLinkField>::prev(T * node) const {
			if(node == null)
				return NULL;

			Link<T> *prev = Link<T>::getLink(node, m_offset)->prevLink();
			if(prev == &m_link) 
				return NULL;

			return Link<T>::getData(prev, m_offset);
		}

		// ----
		// ---- List iterators
		// ----
		template<typename T, Link<T> T::*TLinkField>
		typename List<T, TLinkField>::iterator List<T, TLinkField>::begin() {
			return iterator(&m_link, m_offset, head());
		}

		template<typename T, Link<T> T::*TLinkField>
		typename List<T, TLinkField>::iterator List<T, TLinkField>::end() {
			return iterator(&m_link, m_offset, NULL);
		}

		template<typename T, Link<T> T::*TLinkField>
		typename List<T, TLinkField>::const_iterator List<T, TLinkField>::begin() const {
			return const_iterator(&m_link, m_offset, head());
		}

		template<typename T, Link<T> T::*TLinkField>
		typename List<T, TLinkField>::const_iterator List<T, TLinkField>::end() const {
			return const_iterator(&m_link, m_offset, NULL);
		}

		template<typename T, Link<T> T::*TLinkField>
		void List<T, TLinkField>::clear() {
			unlinkAll();
		}

	}
}

#endif
