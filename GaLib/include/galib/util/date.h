#ifndef galib_util_date_h__
#define galib_util_date_h__

#include "../base.h"
#include <ctime>

namespace galib {

	namespace util {
		
		// ---- Time interval ----
		class TimeInterval {
		public:
			TimeInterval(i64 i)
				: interval(i) {}
			TimeInterval(int days, int hours, int minutes, int seconds)
				: interval(days*24*60*60*1000 + hours*60*60*1000 + minutes*60*1000 + seconds*1000) {}

			i64 getTotalDays()                         const { return interval/(24*60*60*1000); }
			i64 getTotalHours()                        const { return interval/(60*60*1000); }
			i64 getTotalMinutes()                      const { return interval/(60*1000); }
			i64 getTotalSeconds()                      const { return interval/1000; }
			i64 getTotalMiliseconds()                  const { return interval; }

			i64 compare(TimeInterval otherInterval) const           { return interval - otherInterval.interval; }
			bool equals(TimeInterval otherInterval) const           { return interval == otherInterval.interval; }
			bool operator == (TimeInterval otherInterval) const     { return interval == otherInterval.interval; }
			bool operator != (TimeInterval otherInterval) const     { return interval != otherInterval.interval; }
			bool operator < (TimeInterval otherInterval) const      { return interval < otherInterval.interval; }

		private:
			i64 interval;
		};
		
		// ---- DateTime ---
		class DateTime {
		public:
			DateTime()                 : from1970(0) {}
			DateTime(i64 from1970_UTC) : from1970(from1970_UTC) {}
			DateTime(int year, int month, int day)                                        { construct(year, month, day, 0, 0, 0, true); }
			DateTime(int year, int month, int day, int hour, int minutes, int seconds)    { construct(year, month, day, hour, minutes, seconds, true); }
			DateTime(const DateTime& otherDate) : from1970(otherDate.from1970) {}

			DateTime getDate() const;

			DateTime addInterval(TimeInterval interval) const;
			DateTime subInterval(TimeInterval interval) const;

			TimeInterval sub(DateTime otherDate) const;

			i64 compare(DateTime otherDate) const;
			bool equals(DateTime otherDate) const;
			bool operator == (DateTime otherDate) const;
			bool operator != (DateTime otherDate) const;

			bool isValid() const;
			i64 getTime() const;

			std::time_t toTimeT() const;
			
			std::string toStringUTC() const;
			std::string toString() const;

			std::tm toTmUTC() const;
			std::tm toTm() const;

			static DateTime parseStrUTC(const char* dateString);
			static DateTime parseStrUTC(const std::string& dateString);

			static DateTime parseStr(const char* dateString);
			static DateTime parseStr(const std::string& dateString);

			static DateTime fromTmUTC(const std::tm& timeDate);
			static DateTime fromTm(const std::tm& timeDate);

			static DateTime fromTimeT(std::time_t t);

			static DateTime now();
			static DateTime minValue();
			static DateTime maxValue();
			
		private:
			void construct(int year, int month, int day, int hour, int minutes, int seconds, bool isLocal);

			static void parseGeneric(const char* fmt, const char* str, tm& outTm);
			
			i64 from1970;
		};

		static const char* DATE_DEFAULT_FORMAT = "%Y-%m-%d %H:%M:%S";
		
		inline TimeInterval DateTime::sub(DateTime otherDate) const {
			i64 d = from1970 - otherDate.from1970;
			return TimeInterval(d);
		}

		inline DateTime DateTime::addInterval(TimeInterval interval) const {
			i64 t = from1970 + interval.getTotalMiliseconds();
			return DateTime(t);
		}

		inline DateTime DateTime::getDate() const {
			std::tm sTm = toTmUTC();
			sTm.tm_hour = 0;
			sTm.tm_min = 0;
			sTm.tm_sec = 0;
			return fromTmUTC(sTm);
		}

		inline DateTime DateTime::subInterval(TimeInterval interval) const {
			i64 t = from1970 - interval.getTotalMiliseconds();
			return DateTime(t);
		}

		inline i64 DateTime::compare(DateTime otherDate) const {
			return from1970 - otherDate.from1970;
		}

		inline bool DateTime::equals(DateTime otherDate) const {
			return from1970 == otherDate.from1970;
		}

		inline bool DateTime::operator==(DateTime otherDate ) const {
			return from1970 == otherDate.from1970;
		}

		inline bool DateTime::operator!=(DateTime otherDate) const {
			return from1970 != otherDate.from1970;
		}

		inline i64 DateTime::getTime() const {
			return from1970;
		}

		inline bool DateTime::isValid() const {
			return from1970 > 0;
		}

		inline std::time_t DateTime::toTimeT() const {
			std::time_t t = from1970 / 1000;
			return t;
		}
		
		inline std::tm DateTime::toTmUTC() const {
			// Return a UTC date in the form: yyyy-MM-dd HH:mm:ss
			// ex: 2012-03-16 04:50:01
			std::time_t t = toTimeT();

			std::tm sTm;
			gmtime_s(&sTm, &t);

			return sTm;
		}

		inline std::tm DateTime::toTm() const {
			// Return a local date in the form: yyyy-MM-dd HH:mm:ss
			// ex: 2012-03-16 04:50:01
			std::time_t t = toTimeT();

			std::tm sTm;
#ifdef GALIB_WINDOWS
			localtime_s(&sTm, &t);
#else
#error "compiler not supported"
#endif

			return sTm;
		}

		inline std::string DateTime::toStringUTC() const {
			// Return a UTC date in the form: yyyy-MM-dd HH:mm:ss
			// ex: 2012-03-16 04:50:01
			std::time_t t = toTimeT();

			std::tm sTm;
			gmtime_s(&sTm, &t);

			char buff[64];
			strftime(buff, 64, DATE_DEFAULT_FORMAT, &sTm);
			
			return std::string(buff);
		}

		inline std::string DateTime::toString() const {
			// Return a local date in the form: yyyy-MM-dd HH:mm:ss
			// ex: 2012-03-16 04:50:01
			std::time_t t = toTimeT();

			std::tm sTm;
#ifdef GALIB_WINDOWS
			localtime_s(&sTm, &t);
#else
#error "compiler not supported"
#endif

			char buff[64];
			strftime(buff, 64, DATE_DEFAULT_FORMAT, &sTm);
			
			return std::string(buff);
		}

		inline DateTime DateTime::fromTm(const std::tm& sTm) {
			std::time_t t = mktime((std::tm*) &sTm);
			return fromTimeT(t);
		}

		inline DateTime DateTime::fromTmUTC(const std::tm& sTm) {
#ifdef GALIB_WINDOWS
			std::time_t t = _mkgmtime((std::tm*) &sTm);
#else
#error "compiler not supported"
#endif
			return fromTimeT(t);
		}

		inline DateTime DateTime::fromTimeT(std::time_t t) {
			return DateTime(t * 1000);
		}

		inline void DateTime::construct(int year, int month, int day, int hour, int minutes, int seconds, bool isLocal) {
			std::tm sTm;
			sTm.tm_year = year - 1900;
			sTm.tm_mon = month - 1;
			sTm.tm_mday = day;
			sTm.tm_hour = hour;
			sTm.tm_min = minutes;
			sTm.tm_sec = seconds;
			if (isLocal) {
				from1970 = DateTime::fromTm(sTm).from1970;
			}
			else {
				from1970 = DateTime::fromTmUTC(sTm).from1970;
			}
		}

		inline DateTime DateTime::parseStr(const std::string& dateString) {
			return parseStr(dateString.c_str());
		}

		inline DateTime DateTime::parseStr(const char* dateString) {
			std::tm sTm;
			parseGeneric("yyyy-MM-dd HH:mm:ss", dateString, sTm);
			return fromTm(sTm);
		}

		inline DateTime DateTime::parseStrUTC(const std::string& dateString) {
			return parseStrUTC(dateString.c_str());
		}

		inline DateTime DateTime::parseStrUTC(const char* dateString) {
			std::tm sTm;
			parseGeneric("yyyy-MM-dd HH:mm:ss", dateString, sTm);
			return fromTmUTC(sTm);
		}

		inline DateTime DateTime::now() {
			std::time_t t;
			time(&t);
			return fromTimeT(t);
		}

		inline DateTime DateTime::minValue() {
			return DateTime(1);
		}

		inline DateTime DateTime::maxValue() {
			return DateTime((i64)1 << 62);
		}
		
		inline void DateTime::parseGeneric(const char* fmt, const char* str, tm& outTm) {
			int year = 0;
			int month = 0;
			int day = 0;
			int hour = 0;
			int minute = 0;
			int second = 0;
			bool ok = true;

			char c, f, lastF = 0;
			for(size_t i = 0; (c = str[i]) != '\0' && (f = fmt[i]) != '\0' && ok; i++) {
				int d = -1;
				if(c >= '0' && c <= '9') {
					d = c - '0';
				}

				switch(f) {
				case 'y':
					if(d == -1) { ok = false; }
					else { year = 10 * year + d; }
					break;
				case 'M':
					if(d == -1) { ok = false; }
					else { month = 10 * month + d; }
					break;
				case 'd':
					if(d == -1) { ok = false; }
					else { day = 10 * day + d; }
					break;
				case 'H':
					if(d == -1) { ok = false; }
					else { hour = 10 * hour + d; }
					break;
				case 'm':
					if(d == -1) { ok = false; }
					else { minute = 10 * minute + d; }
					break;
				case 's':
					if(d == -1) { ok = false; }
					else { second = 10 * second + d; }
					break;
				default:
					if(d != -1) { ok = false; }
					break;
				};
			}

			outTm.tm_year = year - 1900;
			outTm.tm_mon = month - 1;
			outTm.tm_mday = day;

			outTm.tm_hour = hour;
			outTm.tm_min = minute;
			outTm.tm_sec = second;
		}
		
	}

}


namespace std {

	template <>
	struct hash<galib::util::TimeInterval> {
		size_t operator()(const galib::util::TimeInterval& timeInterval) const {
			return (size_t) timeInterval.getTotalMiliseconds();
		}
	};

	template <>
	struct hash<galib::util::DateTime> {
		size_t operator()(const galib::util::DateTime& dateTime) const {
			return (size_t) dateTime.getTime();
		}
	};
}

#endif
