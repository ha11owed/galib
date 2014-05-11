#ifndef galib_util_object_h__
#define galib_util_object_h__

namespace galib {

	namespace util {

		class GaObject {
			GaObject * m_firstChild;
			GaObject * m_parent;
			GaObject * m_next;
			GaObject * m_prev;
			
			void insertInList(GaObject * obj);
			void removeFromList();

			GaObject() : m_firstChild(NULL), m_parent(NULL), m_next(NULL), m_prev(NULL) {}

		public:
			explicit GaObject(GaObject * parent) 
				: m_firstChild(NULL), m_parent(NULL), m_next(NULL), m_prev(NULL)   { setParent(parent); }
			virtual ~GaObject()                                                    { setParent(NULL); deleteAllChildren(); }
			
			void setParent(GaObject * parent);
			bool removeChild(GaObject * child);
			
			void deleteAllChildren();
			
			bool hasChildren() const;
			GaObject* parent() const;
			GaObject* next() const;
			GaObject* firstChild() const;

		};


		// ---- inline implementation ----

		inline GaObject* GaObject::parent() const {
			return m_parent;
		}

		inline GaObject* GaObject::next() const {
			return m_next;
		}

		inline GaObject* GaObject::firstChild() const {
			return m_firstChild;
		}

		inline bool GaObject::hasChildren() const {
			return m_firstChild != NULL;
		}

		inline void GaObject::removeFromList() {
			if(m_next != NULL) {
				m_next->m_prev = m_prev;
			}
			if(m_prev != NULL) {
				m_prev->m_next = m_next;
			}
		}


		inline void GaObject::insertInList(GaObject * obj) {
			obj->removeFromList();
			if(m_next == NULL) {
				obj->m_prev = this;
				m_next = obj;
			}
			else {
				obj->m_prev = this;
				obj->m_next = m_next;
				m_next->m_prev = obj;
				m_next = obj;
			}
		}

		inline bool GaObject::removeChild(GaObject * child) {
			if(NULL != child && child->m_parent == this) {
				GaObject* c = child->m_next;
				child->removeFromList();
				child->m_parent = NULL;
				m_firstChild = c;
				return true;
			}
			return false;
		}

		inline void GaObject::setParent(GaObject * parent) {
			if(NULL != m_parent) {
				m_parent->removeChild(this);
			}
			// Set the parent
			m_parent = parent;
			if(NULL != parent) {
				if(parent->m_firstChild != NULL) {
					parent->m_firstChild->insertInList(this);
				}
				else {
					parent->m_firstChild = this;
				}
			}
		}

		inline void GaObject::deleteAllChildren() {
			while(NULL != m_firstChild) {
				delete m_firstChild;
			}
		}
		
	}
}

#endif