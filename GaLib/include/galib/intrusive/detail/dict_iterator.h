#ifndef galib_intrusive_detail_dictiterator_h__
#define galib_intrusive_detail_dictiterator_h__

#include "../link.h"
#include <iterator>

namespace galib {

	namespace intrusive {

		namespace detail {
			
			template<typename T, typename TPointer, typename TReference>
			class DictionaryIterator : public std::iterator<std::bidirectional_iterator_tag, T, TPointer, TReference> {
			public:
				DictionaryIterator(Link<T> * begin, Link<T> * end, size_t offset, size_t index) {
					m_begin = begin;
					m_end = end;
					m_offset = offset;

					m_currentBucket = begin + index;
					m_currentItem = NULL;
					updateNextBucket();
				}

				// NOTE: the two constructors and the friend are needed in order to allow conversion from one type to the other
				friend class DictionaryIterator<T, const T*, const T&>;

				DictionaryIterator(const DictionaryIterator<T, T*, T&>& other)
					: m_begin(other.m_begin), m_end(other.m_end), m_offset(other.m_offset), m_currentBucket(other.m_currentBucket), m_currentItem(other.m_currentItem) {
				}

				DictionaryIterator(const DictionaryIterator<T, const T*, const T&>& other)
					: m_begin(other.m_begin), m_end(other.m_end), m_offset(other.m_offset), m_currentBucket(other.m_currentBucket), m_currentItem(other.m_currentItem) {
				}

				TReference operator*() {
					ASSERT(m_currentItem != NULL);
					return *m_currentItem;
				}

				TPointer operator->() {
					ASSERT(m_currentItem != NULL);
					return m_currentItem;
				}

				const DictionaryIterator& operator--() {
					return *this;
				}

				DictionaryIterator operator--(int){
					// Use operator--()
					const DictionaryIterator old(*this);
					--(*this);
					return old;
				}

				const DictionaryIterator& operator++() {
					if (m_currentBucket != m_end) {
						Link<T> * current = Link<T>::getLink(m_currentItem, m_offset);
						Link<T> * next = current->nextLink();
						if (next == m_currentBucket) {
							// There is no next
							m_currentItem = NULL;
							m_currentBucket++;
							updateNextBucket();
						}
						else {
							m_currentItem = Link<T>::getData(next, m_offset);
						}
					}
					return *this;
				}

				DictionaryIterator operator++(int){
					// Use operator++()
					const DictionaryIterator old(*this);
					++(*this);
					return old;
				}
				
				bool operator != (const DictionaryIterator& other) const {
					return !(*this == other);
				}

				bool operator == (const DictionaryIterator& other) const {
					return m_currentBucket == other.m_currentBucket && m_currentItem == other.m_currentItem && m_offset == other.m_offset;
				}

			protected:
				void updatePrevBucket() {
					while (m_currentItem == NULL && m_currentBucket != m_begin) {
						Link<T> * prev = m_currentBucket->prevLink();
						if (prev != m_currentBucket) {
							m_currentItem = Link<T>::getData(prev, m_offset);
						}
						else {
							m_currentBucket--;
						}
					}
				}

				void updateNextBucket() {
					while (m_currentItem == NULL && m_currentBucket != m_end) {
						Link<T> * next = m_currentBucket->nextLink();
						if (next != m_currentBucket) {
							m_currentItem = Link<T>::getData(next, m_offset);
						}
						else {
							m_currentBucket++;
						}
					}
				}

				Link<T> * m_begin;
				Link<T> * m_end;
				size_t    m_offset;

				Link<T> * m_currentBucket;
				T       * m_currentItem;
			};



		}

	}
}

#endif
