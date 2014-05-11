#ifndef galib_util_nullable_h__
#define galib_util_nullable_h__

#include "../base.h"
#include <algorithm>

namespace galib {

	namespace util {

		// ---- Nullable ----
		template<typename T>
		class Nullable {
		private:
			T m_value;
			bool m_hasValue;
		public:
			Nullable()                      : m_value(), m_hasValue(false) {}
			Nullable(T value)               : m_value(value), m_hasValue(true) {} 
			Nullable(const Nullable& other) : m_value(other.m_value), m_hasValue(other.m_hasValue) {}

			friend void swap(Nullable& a, Nullable& b) {
				std::swap(a.m_hasValue, b.m_hasValue);
				std::swap(a.m_value, b.m_value);
			}

			Nullable& operator =(Nullable<T> other) {
				swap(*this, other);
				return *this;
			}

			T& operator =(const T& val) {
				m_value = val;
				m_hasValue = true;
				return m_value; 
			}

			bool operator ==(const Nullable<T>& other) const {
				return m_hasValue == other.m_hasValue && m_value == other.m_value;
			}

			bool operator !=(const Nullable<T>& other) const {
				return m_hasValue != other.m_hasValue || (m_hasValue && m_value != other.m_value);
			}

			bool operator ==(const T& val) const {
				return m_hasValue && m_value == val;
			}

			bool operator !=(const T& val) const {
				return !m_hasValue || (m_hasValue && m_value != val);
			}
			
			const T& value() const {
				if (m_hasValue) { return m_value; }
				else { throw std::logic_error("Value of Nullable is not set."); }
			}
			
			void removeValue() {
				m_value = T();
				m_hasValue = false;
			}

			bool hasValue() const {
				return m_hasValue;
			}
		};

	}
	
}

#endif
