#ifndef galib_intrusive_entity_h__
#define galib_intrusive_entity_h__

#include "list.h"

namespace galib {

	namespace intrusive {

		class GaObject {
		public:
			virtual ~GaObject() {}

			virtual int  hashCode() const                 = 0;
			virtual bool equals(GaObject* other) const    = 0;
		};

	}
}

#endif