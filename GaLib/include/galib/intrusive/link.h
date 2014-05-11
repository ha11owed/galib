#ifndef galib_intrusive_link_h__
#define galib_intrusive_link_h__

#include "../base.h"

namespace galib {

	namespace intrusive {
		
		// ---- Link -------------------------------------------------------------------
		template<typename T>
		class Link {
		public:
			Link();
			virtual ~Link();

			bool isLinked() const;
			void unlink();
			void insertAfter(Link<T>* link);
			void insertBefore(Link<T>* link);
			void insertAfter(T* node, size_t offset);
			void insertBefore(T* node, size_t offset);
			
			Link<T> * prevLink() const;
			Link<T> * nextLink() const;

			T       * prev(size_t offset) const;
			T       * next(size_t offset) const;
			T       * owner(size_t offset) const;

			static Link<T> * getLink(T * data, size_t offset);
			static T       * getData(Link<T> * link, size_t offset);

		private:
			Link<T> * m_next;
			Link<T> * m_prev;

			void removeFromList();

			// Hide copy-constructor and assignment operator
			Link (const Link &);
			Link & operator= (const Link &);
		};


		// -----------------------------------------------------------------------------
		// ---- Link -------------------------------------------------------------------
		// -----------------------------------------------------------------------------
		template<typename T>
		Link<T>::Link() {
			m_next = m_prev = this;
		}

		template<typename T>
		Link<T>::~Link() {
			unlink();
		}
		
		template<typename T>
		Link<T> * Link<T>::getLink(T * data, size_t offset) {
			ASSERT(data != NULL && offset >= 0);
			return (Link<T>*) ( (size_t) data + offset );
		}

		template<typename T>
		T* Link<T>::getData(Link<T> * link, size_t offset) {
			ASSERT(link != NULL && offset >= 0);
			return (T*) ( (size_t)link - offset );
		}

		template<typename T>
		bool Link<T>::isLinked() const {
			return m_next != this;
		}

		template<typename T>
		void Link<T>::removeFromList() {
			m_next->m_prev = m_prev;
			m_prev->m_next = m_next;
		}

		template<typename T>
		void Link<T>::unlink() {
			removeFromList();
			m_next = m_prev = this;
		}

		template<typename T>
		void Link<T>::insertAfter(Link<T>* link) {
			ASSERT(link != NULL);
			link->removeFromList();

			link->m_next = m_next;
			link->m_prev = this;
			m_next->m_prev = link;
			m_next = link;
		}

		template<typename T>
		void Link<T>::insertBefore(Link<T>* link) {
			ASSERT(link != NULL);
			link->removeFromList();

			link->m_prev = m_prev;
			link->m_next = this;
			m_prev->m_next = link;
			m_prev = link;
		}

		template<typename T>
		void Link<T>::insertAfter(T* node, size_t offset) {
			ASSERT(node != NULL);
			insertAfter(getLink(node, offset));
		}

		template<typename T>
		void Link<T>::insertBefore(T* node, size_t offset) {
			ASSERT(node != NULL);
			insertBefore(getLink(node, offset));
		}

		template<typename T>
		Link<T>* Link<T>::nextLink() const {
			return m_next;
		}

		template<typename T>
		Link<T>* Link<T>::prevLink() const {
			return m_prev;
		}

		template<typename T>
		T* Link<T>::next(size_t offset) const {
			if(m_next == this)
				return NULL;
			return getData(m_next, offset);
		}

		template<typename T>
		T* Link<T>::prev(size_t offset) const {
			if(m_prev == this)
				return NULL;
			return getData(m_prev, offset);
		}

		template<typename T>
		T* Link<T>::owner(size_t offset) const {
			return getData((Link<T>*) this, offset);
		}
		

	}
}

#endif
