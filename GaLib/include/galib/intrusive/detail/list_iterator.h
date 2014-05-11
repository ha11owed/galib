#ifndef galib_intrusive_detail_listiterator_h__
#define galib_intrusive_detail_listiterator_h__

#include "../link.h"
#include <iterator>

namespace galib {

	namespace intrusive {

		namespace detail {

			template<typename T, typename TPointer, typename TReference>
			class ListIterator : public std::iterator<std::bidirectional_iterator_tag, T, TPointer, TReference> {
			public:
				ListIterator(Link<T> * startLink, size_t offset, T * item) {
					m_startLink = startLink;
					m_offset = offset;
					m_currentItem = item;
				}

				// NOTE: the two constructors and the friend are needed in order to allow conversion from one type to the other
				friend class ListIterator<T, const T*, const T&>;

				ListIterator(const ListIterator<T, T*, T&>& other)
					: m_startLink(other.m_startLink), m_offset(other.m_offset), m_currentItem(other.m_currentItem) {
				}

				ListIterator(const ListIterator<T, const T*, const T&>& other)
					: m_startLink(other.m_startLink), m_offset(other.m_offset), m_currentItem(other.m_currentItem) {
				}

				TReference operator*() {
					ASSERT(m_currentItem != NULL);
					return *m_currentItem;
				}

				TPointer operator->() {
					ASSERT(m_currentItem != NULL);
					return m_currentItem;
				}

				const ListIterator& operator--() {
					Link<T> * prev;
					if (m_currentItem != NULL) {
						Link<T> * current = Link<T>::getLink(m_currentItem, m_offset);
						prev = current->prevLink();
					}
					else {
						prev = m_startLink->prevLink();
					}

					if(prev != m_startLink) {
						m_currentItem = Link<T>::getData(prev, m_offset);
					}
					else {
						m_currentItem = NULL;
					}

					return *this;
				}

				ListIterator operator--(int){
					// Use operator--()
					const ListIterator old(*this);
					--(*this);
					return old;
				}

				const ListIterator& operator++() {
					if (m_currentItem != NULL) {
						Link<T> * current = Link<T>::getLink(m_currentItem, m_offset);
						Link<T> * next = current->nextLink();
						if(next != m_startLink) {
							m_currentItem = Link<T>::getData(next, m_offset);
						}
						else {
							m_currentItem = NULL;
						}
					}
					return *this;
				}

				ListIterator operator++(int){
					// Use operator++()
					const ListIterator old(*this);
					++(*this);
					return old;
				}

				bool operator != (const ListIterator& other) const {
					return !(*this == other);
				}

				bool operator == (const ListIterator& other) const {
					return m_currentItem == other.m_currentItem && m_offset == other.m_offset;
				}

			protected:
				Link<T> * m_startLink;
				size_t    m_offset;
				T       * m_currentItem;
			};

		}

	}
}

#endif
