/*
 Copyright (C) 2017-2022 Fredrik Öhrström (gpl-3.0-or-later)

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "util.h"

#include <algorithm>
#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <functional>
#include <grp.h>
#include <math.h>
#include <pwd.h>
#include <set>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>
#include <string>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>

int char2int(char input) {
  if (input >= '0' && input <= '9')
    return input - '0';
  if (input >= 'A' && input <= 'F')
    return input - 'A' + 10;
  if (input >= 'a' && input <= 'f')
    return input - 'a' + 10;
  return -1;
}

bool isHexChar(uchar c) { return char2int(c) != -1; }

// The byte 0x13 i converted into the integer value 13.
uchar bcd2bin(uchar c) { return (c & 15) + (c >> 4) * 10; }

// The byte 0x13 is converted into the integer value 31.
uchar revbcd2bin(uchar c) { return (c & 15) * 10 + (c >> 4); }

uchar reverse(uchar c) { return ((c & 15) << 4) | (c >> 4); }

bool isHexString(const char *txt, bool *invalid, bool strict) {
  *invalid = false;
  // An empty string is not an hex string.
  if (*txt == 0)
    return false;

  const char *i = txt;
  int n = 0;
  for (;;) {
    char c = *i++;
    if (!strict && c == '#')
      continue; // Ignore hashes if not strict
    if (!strict && c == ' ')
      continue; // Ignore hashes if not strict
    if (!strict && c == '|')
      continue; // Ignore hashes if not strict
    if (!strict && c == '_')
      continue; // Ignore underlines if not strict
    if (c == 0)
      break;
    n++;
    if (char2int(c) == -1)
      return false;
  }
  // An empty string is not an hex string.
  if (n == 0)
    return false;
  if (n % 2 == 1)
    *invalid = true;

  return true;
}

bool isHexStringFlex(const char *txt, bool *invalid) {
  return isHexString(txt, invalid, false);
}

bool isHexStringFlex(const std::string &txt, bool *invalid) {
  return isHexString(txt.c_str(), invalid, false);
}

bool isHexStringStrict(const char *txt, bool *invalid) {
  return isHexString(txt, invalid, true);
}

bool isHexStringStrict(const std::string &txt, bool *invalid) {
  return isHexString(txt.c_str(), invalid, true);
}

bool hex2bin(const char *src, std::vector<uchar> *target) {
  if (!src)
    return false;
  while (*src && src[1]) {
    if (*src == ' ' || *src == '#' || *src == '|' || *src == '_') {
      // Ignore space and hashes and pipes and underlines.
      src++;
    } else {
      int hi = char2int(*src);
      int lo = char2int(src[1]);
      if (hi < 0 || lo < 0)
        return false;
      target->push_back(hi * 16 + lo);
      src += 2;
    }
  }
  return true;
}

bool hex2bin(const std::string &src, std::vector<uchar> *target) {
  return hex2bin(src.c_str(), target);
}

bool hex2bin(std::vector<uchar> &src, std::vector<uchar> *target) {
  if (src.size() % 2 == 1)
    return false;
  for (size_t i = 0; i < src.size(); i += 2) {
    if (src[i] != ' ') {
      int hi = char2int(src[i]);
      int lo = char2int(src[i + 1]);
      if (hi < 0 || lo < 0)
        return false;
      target->push_back(hi * 16 + lo);
    }
  }
  return true;
}

char const hexChar[16] = {'0', '1', '2', '3', '4', '5', '6', '7',
                          '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

std::string bin2hex(const std::vector<uchar> &target) {
  std::string str;
  for (size_t i = 0; i < target.size(); ++i) {
    const char ch = target[i];
    str.append(&hexChar[(ch & 0xF0) >> 4], 1);
    str.append(&hexChar[ch & 0xF], 1);
  }
  return str;
}

std::string bin2hex(std::vector<uchar>::iterator data,
                    std::vector<uchar>::iterator end, int len) {
  std::string str;
  while (data != end && len-- > 0) {
    const char ch = *data;
    data++;
    str.append(&hexChar[(ch & 0xF0) >> 4], 1);
    str.append(&hexChar[ch & 0xF], 1);
  }
  return str;
}

std::string bin2hex(std::vector<uchar> &data, int offset, int len) {
  std::string str;
  std::vector<uchar>::iterator i = data.begin();
  i += offset;
  while (i != data.end() && len-- > 0) {
    const char ch = *i;
    i++;
    str.append(&hexChar[(ch & 0xF0) >> 4], 1);
    str.append(&hexChar[ch & 0xF], 1);
  }
  return str;
}

std::string safeString(std::vector<uchar> &target) {
  std::string str;
  for (size_t i = 0; i < target.size(); ++i) {
    const char ch = target[i];
    if (ch >= 32 && ch < 127 && ch != '<' && ch != '>') {
      str += ch;
    } else {
      str += '<';
      str.append(&hexChar[(ch & 0xF0) >> 4], 1);
      str.append(&hexChar[ch & 0xF], 1);
      str += '>';
    }
  }
  return str;
}

std::string tostrprintf(const char *fmt, ...) {
  std::string s;
  char buf[4096];
  va_list args;
  va_start(args, fmt);
  size_t n = vsnprintf(buf, 4096, fmt, args);
  assert(n < 4096);
  va_end(args);
  s = buf;
  return s;
}

// Why a pointer here? To avoid the compiler warning:
// warning: passing an object of reference type to 'va_start' has undefined
// behavior [-Wvarargs]
std::string tostrprintf(const std::string *fmt, ...) {
  std::string s;
  char buf[4096];
  va_list args;
  va_start(args, fmt); // <<<<< here fmt must be a native type.
  size_t n = vsnprintf(buf, 4096, fmt->c_str(), args);
  assert(n < 4096);
  va_end(args);
  s = buf;
  return s;
}

void strprintf(std::string *s, const char *fmt, ...) {
  char buf[4096];
  va_list args;
  va_start(args, fmt);
  size_t n = vsnprintf(buf, 4096, fmt, args);
  assert(n < 4096);
  va_end(args);
  *s = buf;
}

void xorit(uchar *srca, uchar *srcb, uchar *dest, int len) {
  for (int i = 0; i < len; ++i) {
    dest[i] = srca[i] ^ srcb[i];
  }
}

void shiftLeft(uchar *srca, uchar *srcb, int len) {
  uchar overflow = 0;

  for (int i = len - 1; i >= 0; i--) {
    srcb[i] = srca[i] << 1;
    srcb[i] |= overflow;
    overflow = (srca[i] & 0x80) >> 7;
  }
  return;
}

std::string format3fdot3f(double v) {
  std::string r;
  strprintf(&r, "%3.3f", v);
  return r;
}

bool logging_silenced_ = false;
bool verbose_enabled_ = false;
bool debug_enabled_ = false;
bool trace_enabled_ = false;

void silentLogging(bool b) { logging_silenced_ = b; }

const char *version_;

void setVersion(const char *v) { version_ = v; }

const char *getVersion() { return version_; }

time_t telegrams_start_time_;

bool is_ascii_alnum(char c) {
  if (c >= 'A' && c <= 'Z')
    return true;
  if (c >= 'a' && c <= 'z')
    return true;
  if (c >= '0' && c <= '9')
    return true;
  if (c == '_')
    return true;
  return false;
}

bool is_ascii(char c) {
  if (c >= 'A' && c <= 'Z')
    return true;
  if (c >= 'a' && c <= 'z')
    return true;
  return false;
}

bool isValidAlias(const std::string &alias) {
  if (alias.length() == 0)
    return false;

  if (!is_ascii(alias[0]))
    return false;

  for (char c : alias) {
    if (!is_ascii_alnum(c))
      return false;
  }

  return true;
}

bool isFrequency(const std::string &fq) {
  int len = fq.length();
  if (len == 0)
    return false;
  if (fq[len - 1] != 'M')
    return false;
  len--;
  for (int i = 0; i < len; ++i) {
    if (!isdigit(fq[i]) && fq[i] != '.')
      return false;
  }
  return true;
}

bool isNumber(const std::string &fq) {
  int len = fq.length();
  if (len == 0)
    return false;
  for (int i = 0; i < len; ++i) {
    if (!isdigit(fq[i]))
      return false;
  }
  return true;
}

void incrementIV(uchar *iv, size_t len) {
  uchar *p = iv + len - 1;
  while (p >= iv) {
    int pp = *p;
    (*p)++;
    if (pp + 1 <= 255) {
      // Nice, no overflow. We are done here!
      break;
    }
    // Move left add add one.
    p--;
  }
}

void debugPayload(const std::string &intro, std::vector<uchar> &payload) {
  std::string msg = bin2hex(payload);
  verbose("%s \"%s\"\n", intro.c_str(), msg.c_str());
}

void debugPayload(const std::string &intro, std::vector<uchar> &payload,
                  std::vector<uchar>::iterator &pos) {
  std::string msg = bin2hex(pos, payload.end(), 1024);
  verbose("%s \"%s\"\n", intro.c_str(), msg.c_str());
}

void logTelegram(std::vector<uchar> &original, std::vector<uchar> &parsed,
                 int header_size, int suffix_size) {
  std::vector<uchar> logged = parsed;
  if (!original.empty()) {
    logged = std::vector<uchar>(parsed);
    for (unsigned int i = 0; i < original.size(); i++) {
      logged[i] = original[i];
    }
  }
  time_t diff = time(NULL) - telegrams_start_time_;
  std::string parsed_hex = bin2hex(logged);
  std::string header = parsed_hex.substr(0, header_size * 2);
  std::string content = parsed_hex.substr(header_size * 2);
  if (suffix_size == 0) {
    notice("telegram=|%s_%s|+%ld\n", header.c_str(), content.c_str(), diff);
  } else {
    assert((suffix_size * 2) < (int)content.size());
    std::string content2 = content.substr(0, content.size() - suffix_size * 2);
    std::string suffix = content.substr(content.size() - suffix_size * 2);
    notice("telegram=|%s_%s_%s|+%ld\n", header.c_str(), content2.c_str(),
           suffix.c_str(), diff);
  }
}

std::string eatTo(std::vector<uchar> &v, std::vector<uchar>::iterator &i, int c,
                  size_t max, bool *eof, bool *err) {
  std::string s;

  *eof = false;
  *err = false;
  while (max > 0 && i != v.end() && (c == -1 || *i != c)) {
    s += *i;
    i++;
    max--;
  }
  if (c != -1 && i != v.end() && *i != c) {
    *err = true;
  }
  if (i != v.end()) {
    i++;
  }
  if (i == v.end()) {
    *eof = true;
  }
  return s;
}

void padWithZeroesTo(std::vector<uchar> *content, size_t len,
                     std::vector<uchar> *full_content) {
  if (content->size() < len) {
    warning("Padded with zeroes.", (int)len);
    size_t old_size = content->size();
    content->resize(len);
    for (size_t i = old_size; i < len; ++i) {
      (*content)[i] = 0;
    }
    full_content->insert(full_content->end(), content->begin() + old_size,
                         content->end());
  }
}

static std::string space =
    "                                                                          "
    "                                                                          "
    "           ";
std::string padLeft(const std::string &input, int width) {
  int w = width - input.size();
  if (w < 0)
    return input;
  assert(w < (int)space.length());
  return space.substr(0, w) + input;
}

int parseTime(const std::string &s) {
  std::string time = s;
  int mul = 1;
  if (time.back() == 'h') {
    time.pop_back();
    mul = 3600;
  }
  if (time.back() == 'm') {
    time.pop_back();
    mul = 60;
  }
  if (time.back() == 's') {
    time.pop_back();
    mul = 1;
  }
  int n = atoi(time.c_str());
  return n * mul;
}

#define CRC16_EN_13757 0x3D65

uint16_t crc16_EN13757_per_byte(uint16_t crc, uchar b) {
  unsigned char i;

  for (i = 0; i < 8; i++) {

    if (((crc & 0x8000) >> 8) ^ (b & 0x80)) {
      crc = (crc << 1) ^ CRC16_EN_13757;
    } else {
      crc = (crc << 1);
    }

    b <<= 1;
  }

  return crc;
}

uint16_t crc16_EN13757(uchar *data, size_t len) {
  uint16_t crc = 0x0000;

  assert(len == 0 || data != NULL);

  for (size_t i = 0; i < len; ++i) {
    crc = crc16_EN13757_per_byte(crc, data[i]);
  }

  return (~crc);
}

#define CRC16_INIT_VALUE 0xFFFF
#define CRC16_GOOD_VALUE 0x0F47
#define CRC16_POLYNOM 0x8408

uint16_t crc16_CCITT(uchar *data, uint16_t length) {
  uint16_t initVal = CRC16_INIT_VALUE;
  uint16_t crc = initVal;
  while (length--) {
    int bits = 8;
    uchar byte = *data++;
    while (bits--) {
      if ((byte & 1) ^ (crc & 1)) {
        crc = (crc >> 1) ^ CRC16_POLYNOM;
      } else
        crc >>= 1;
      byte >>= 1;
    }
  }
  return crc;
}

bool crc16_CCITT_check(uchar *data, uint16_t length) {
  uint16_t crc = ~crc16_CCITT(data, length);
  return crc == CRC16_GOOD_VALUE;
}

std::string eatToSkipWhitespace(std::vector<char> &v,
                                std::vector<char>::iterator &i, int c,
                                size_t max, bool *eof, bool *err) {
  eatWhitespace(v, i, eof);
  if (*eof) {
    if (c != -1) {
      *err = true;
    }
    return "";
  }
  std::string s = eatTo(v, i, c, max, eof, err);
  trimWhitespace(&s);
  return s;
}

std::string eatTo(std::vector<char> &v, std::vector<char>::iterator &i, int c,
                  size_t max, bool *eof, bool *err) {
  std::string s;

  *eof = false;
  *err = false;
  while (max > 0 && i != v.end() && (c == -1 || *i != c)) {
    s += *i;
    i++;
    max--;
  }
  if (c != -1 && (i == v.end() || *i != c)) {
    *err = true;
  }
  if (i != v.end()) {
    i++;
  }
  if (i == v.end()) {
    *eof = true;
  }
  return s;
}

void eatWhitespace(std::vector<char> &v, std::vector<char>::iterator &i,
                   bool *eof) {
  *eof = false;
  while (i != v.end() && (*i == ' ' || *i == '\t')) {
    i++;
  }
  if (i == v.end()) {
    *eof = true;
  }
}

void trimWhitespace(std::string *s) {
  const char *ws = " \t";
  s->erase(0, s->find_first_not_of(ws));
  s->erase(s->find_last_not_of(ws) + 1);
}

std::string strdate(struct tm *date) {
  char buf[256];
  strftime(buf, sizeof(buf), "%Y-%m-%d", date);
  return std::string(buf);
}

std::string strdate(double v) {
  if (::isnan(v))
    return "null";
  struct tm date;
  time_t t = v;
  localtime_r(&t, &date);
  return strdate(&date);
}

std::string strdatetime(struct tm *datetime) {
  char buf[256];
  strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", datetime);
  return std::string(buf);
}

std::string strdatetime(double v) {
  if (::isnan(v))
    return "null";
  struct tm datetime;
  time_t t = v;
  localtime_r(&t, &datetime);
  return strdatetime(&datetime);
}

std::string strdatetimesec(struct tm *datetime) {
  char buf[256];
  strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", datetime);
  return std::string(buf);
}

std::string strdatetimesec(double v) {
  struct tm datetime;
  time_t t = v;
  localtime_r(&t, &datetime);
  return strdatetimesec(&datetime);
}

bool is_leap_year(int year) {
  year += 1900;
  if (year % 4 != 0)
    return false;
  if (year % 400 == 0)
    return true;
  if (year % 100 == 0)
    return false;
  return true;
}

int days_in_months[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

int get_days_in_month(int year, int month) {
  if (month < 0 || month >= 12) {
    month = 0;
  }

  int days = days_in_months[month];

  if (month == 1 && is_leap_year(year)) {
    // Handle february in a leap year.
    days += 1;
  }

  return days;
}

double addMonths(double t, int months) {
  time_t ut = (time_t)t;
  struct tm time;
  localtime_r(&ut, &time);
  addMonths(&time, months);
  return (double)mktime(&time);
}

void addMonths(struct tm *date, int months) {
  bool is_last_day_in_month =
      date->tm_mday == get_days_in_month(date->tm_year, date->tm_mon);

  int year = date->tm_year + months / 12;
  int month = date->tm_mon + months % 12;

  while (month > 11) {
    year += 1;
    month -= 12;
  }

  while (month < 0) {
    year -= 1;
    month += 12;
  }

  int day;

  if (is_last_day_in_month) {
    day = get_days_in_month(
        year, month); // Last day of month maps to last day of result month
  } else {
    day = std::min(date->tm_mday, get_days_in_month(year, month));
  }

  date->tm_year = year;
  date->tm_mon = month;
  date->tm_mday = day;
}

int countSetBits(int v) {
  int n = 0;
  while (v) {
    v &= (v - 1);
    n++;
  }
  return n;
}

bool startsWith(const std::string &s, std::string &prefix) {
  return startsWith(s, prefix.c_str());
}

bool startsWith(const std::string &s, const char *prefix) {
  size_t len = strlen(prefix);
  if (s.length() < len)
    return false;
  if (s.length() == len)
    return s == prefix;
  return !strncmp(&s[0], prefix, len);
}

std::string makeQuotedJson(const std::string &s) {
  size_t p = s.find('=');
  std::string key, value;
  if (p != std::string::npos) {
    key = s.substr(0, p);
    value = s.substr(p + 1);
  } else {
    key = s;
    value = "";
  }

  return std::string("\"") + key + "\":\"" + value + "\"";
}

std::string currentYear() {
  char datetime[40];
  memset(datetime, 0, sizeof(datetime));

  struct timeval tv;
  gettimeofday(&tv, NULL);

  strftime(datetime, 20, "%Y", localtime(&tv.tv_sec));
  return std::string(datetime);
}

std::string currentYearMonth() {
  char datetime[40];
  memset(datetime, 0, sizeof(datetime));

  struct timeval tv;
  gettimeofday(&tv, NULL);

  strftime(datetime, 20, "%Y-%m", localtime(&tv.tv_sec));
  return std::string(datetime);
}

std::string currentYearMonthDay() {
  char datetime[40];
  memset(datetime, 0, sizeof(datetime));

  struct timeval tv;
  gettimeofday(&tv, NULL);

  strftime(datetime, 20, "%Y-%m-%d", localtime(&tv.tv_sec));
  return std::string(datetime);
}

std::string currentHour() {
  char datetime[40];
  memset(datetime, 0, sizeof(datetime));

  struct timeval tv;
  gettimeofday(&tv, NULL);

  strftime(datetime, 20, "%Y-%m-%d_%H", localtime(&tv.tv_sec));
  return std::string(datetime);
}

std::string currentMinute() {
  char datetime[40];
  memset(datetime, 0, sizeof(datetime));

  struct timeval tv;
  gettimeofday(&tv, NULL);

  strftime(datetime, 20, "%Y-%m-%d_%H:%M", localtime(&tv.tv_sec));
  return std::string(datetime);
}

std::string currentSeconds() {
  char datetime[40];
  memset(datetime, 0, sizeof(datetime));

  struct timeval tv;
  gettimeofday(&tv, NULL);

  strftime(datetime, 20, "%Y-%m-%d_%H:%M:%S", localtime(&tv.tv_sec));
  return std::string(datetime);
}

std::string currentMicros() {
  char datetime[40];
  memset(datetime, 0, sizeof(datetime));

  struct timeval tv;
  gettimeofday(&tv, NULL);

  strftime(datetime, 20, "%Y-%m-%d_%H:%M:%S", localtime(&tv.tv_sec));
  return std::string(datetime) + "." + std::to_string(tv.tv_usec);
}

bool hasBytes(int n, std::vector<uchar>::iterator &pos,
              std::vector<uchar> &frame) {
  int remaining = std::distance(pos, frame.end());
  if (remaining < n)
    return false;
  return true;
}

bool startsWith(const std::string &s, std::vector<uchar> &data) {
  if (s.length() > data.size())
    return false;

  for (size_t i = 0; i < s.length(); ++i) {
    if (s[i] != data[i])
      return false;
  }
  return true;
}

struct TimePeriod {
  int day_in_week_from{}; // 0 = mon 6 = sun
  int day_in_week_to{};   // 0 = mon 6 = sun
  int hour_from{};        // Greater than or equal.
  int hour_to{};          // Less than or equal.
};

bool is_inside(struct tm *nowt, TimePeriod *tp) {
  int day = nowt->tm_wday - 1; // tm_wday 0=sun
  if (day == -1)
    day = 6;                // adjust so 0=mon and 6=sun
  int hour = nowt->tm_hour; // hours since midnight 0-23

  // Test is inclusive. mon-sun(00-23) will cover whole week all hours.
  // mon-tue(00-00) will cover mon and tue one hour after midnight.
  if (day >= tp->day_in_week_from && day <= tp->day_in_week_to &&
      hour >= tp->hour_from && hour <= tp->hour_to) {
    return true;
  }
  return false;
}

bool extract_times(const char *p, TimePeriod *tp) {
  if (strlen(p) != 7)
    return false; // Expect (00-23)
  if (p[3] != '-')
    return false; // Must have - in middle.
  int fa = p[1] - 48;
  if (fa < 0 || fa > 9)
    return false;
  int fb = p[2] - 48;
  if (fb < 0 || fb > 9)
    return false;
  int ta = p[4] - 48;
  if (ta < 0 || ta > 9)
    return false;
  int tb = p[5] - 48;
  if (tb < 0 || tb > 9)
    return false;
  tp->hour_from = fa * 10 + fb;
  tp->hour_to = ta * 10 + tb;
  if (tp->hour_from > 23)
    return false; // Hours are 00-23
  if (tp->hour_to > 23)
    return false; // Ditto.
  if (tp->hour_to < tp->hour_from)
    return false; // To must be strictly larger than from, hence the need
                  // for 23.

  return true;
}

int day_name_to_nr(const std::string &name) {
  if (name == "mon")
    return 0;
  if (name == "tue")
    return 1;
  if (name == "wed")
    return 2;
  if (name == "thu")
    return 3;
  if (name == "fri")
    return 4;
  if (name == "say")
    return 5;
  if (name == "sun")
    return 6;
  return -1;
}

bool extract_days(char *p, TimePeriod *tp) {
  if (strlen(p) == 3) {
    std::string s = p;
    int d = day_name_to_nr(s);
    if (d == -1)
      return false;
    tp->day_in_week_from = d;
    tp->day_in_week_to = d;
    return true;
  }

  if (strlen(p) != 7)
    return false; // Expect mon-fri
  if (p[3] != '-')
    return false; // Must have - in middle.
  std::string from = std::string(p, p + 3);
  std::string to = std::string(p + 4, p + 7);

  int f = day_name_to_nr(from);
  int t = day_name_to_nr(to);
  if (f == -1 || t == -1)
    return false;
  if (f >= t)
    return false;
  tp->day_in_week_from = f;
  tp->day_in_week_to = t;
  return true;
}

bool extract_single_period(char *tok, TimePeriod *tp) {
  // Minimum length is 8 chars, eg "1(00-23)"
  size_t len = strlen(tok);
  if (len < 8)
    return false;
  // tok is for example: mon-fri(00-23) or tue(18-19) or 1(00-23)
  char *p = strchr(tok, '(');
  if (p == NULL)
    return false; // There must be a (
  if (tok[len - 1] != ')')
    return false; // Must end in )
  bool ok = extract_times(p, tp);
  if (!ok)
    return false;
  *p = 0; // Terminate in the middle of tok.
  ok = extract_days(tok, tp);
  if (!ok)
    return false;

  return true;
}

bool extract_periods(const std::string &periods,
                     std::vector<TimePeriod> *period_structs) {
  if (periods.length() == 0)
    return false;

  char buf[periods.length() + 1];
  strcpy(buf, periods.c_str());

  char *saveptr{};
  char *tok = strtok_r(buf, ",", &saveptr);
  if (tok == NULL) {
    // No comma found.
    TimePeriod tp{};
    bool ok = extract_single_period(tok, &tp);
    if (!ok)
      return false;
    period_structs->push_back(tp);
    return true;
  }

  while (tok != NULL) {
    TimePeriod tp{};
    bool ok = extract_single_period(tok, &tp);
    if (!ok)
      return false;
    period_structs->push_back(tp);
    tok = strtok_r(NULL, ",", &saveptr);
  }

  return true;
}

bool isValidTimePeriod(const std::string &periods) {
  std::vector<TimePeriod> period_structs;
  bool ok = extract_periods(periods, &period_structs);
  return ok;
}

bool isInsideTimePeriod(time_t now, std::string periods) {
  struct tm nowt {};
  localtime_r(&now, &nowt);

  std::vector<TimePeriod> period_structs;

  bool ok = extract_periods(periods, &period_structs);
  if (!ok)
    return false;

  for (auto &tp : period_structs) {
    // debug("period %d %d %d %d\n", tp.day_in_week_from, tp.day_in_week_to,
    // tp.hour_from, tp.hour_to);
    if (is_inside(&nowt, &tp))
      return true;
  }
  return false;
}

size_t memoryUsage() { return 0; }

std::vector<std::string> alarm_shells_;

const char *toString(Alarm type) {
  switch (type) {
  case Alarm::DeviceFailure:
    return "DeviceFailure";
  case Alarm::RegularResetFailure:
    return "RegularResetFailure";
  case Alarm::DeviceInactivity:
    return "DeviceInactivity";
  case Alarm::SpecifiedDeviceNotFound:
    return "SpecifiedDeviceNotFound";
  }
  return "?";
}

void setAlarmShells(std::vector<std::string> &alarm_shells) {
  alarm_shells_ = alarm_shells;
}

bool stringFoundCaseIgnored(const std::string &h, const std::string &n) {
  std::string haystack = h;
  std::string needle = n;
  // Modify haystack and needle, in place, to become lowercase.
  for_each(haystack.begin(), haystack.end(), [](char &c) { c = ::tolower(c); });
  for_each(needle.begin(), needle.end(), [](char &c) { c = ::tolower(c); });

  // Now use default c++ find, return true if needle was found in haystack.
  return haystack.find(needle) != std::string::npos;
}

std::vector<std::string> splitString(const std::string &s, char c) {
  auto end = s.cend();
  auto start = end;

  std::vector<std::string> v;
  for (auto i = s.cbegin(); i != end; ++i) {
    if (*i != c) {
      if (start == end) {
        start = i;
      }
      continue;
    }
    if (start != end) {
      v.emplace_back(start, i);
      start = end;
    }
  }
  if (start != end) {
    v.emplace_back(start, end);
  }
  return v;
}

std::set<std::string> splitStringIntoSet(const std::string &s, char c) {
  std::vector<std::string> v = splitString(s, c);
  std::set<std::string> words(v.begin(), v.end());
  return words;
}

std::vector<std::string> splitDeviceString(const std::string &ds) {
  std::string s = ds;
  std::string cmd;

  // The CMD(...) might have colons inside.
  // Check this first.
  size_t p = s.rfind(":CMD(");
  if (s.back() == ')' && p != std::string::npos) {
    cmd = s.substr(p + 1);
    s = s.substr(0, p);
  }

  // Now we can split.
  std::vector<std::string> r = splitString(s, ':');

  if (cmd != "") {
    // Re-append the comand last.
    r.push_back(cmd);
  }
  return r;
}

uint32_t indexFromRtlSdrName(const std::string &s) {
  size_t p = s.find('_');
  if (p == std::string::npos)
    return -1;
  std::string n = s.substr(0, p);
  return (uint32_t)atoi(n.c_str());
}

#define KB 1024ull

std::string helper(size_t scale, size_t s, std::string suffix) {
  size_t o = s;
  s /= scale;
  size_t diff = o - (s * scale);
  if (diff == 0) {
    return std::to_string(s) + ".00" + suffix;
  }
  size_t dec = (int)(100 * (diff + 1) / scale);
  return std::to_string(s) + ((dec < 10) ? ".0" : ".") + std::to_string(dec) +
         suffix;
}

std::string humanReadableTwoDecimals(size_t s) {
  if (s < KB) {
    return std::to_string(s) + " B";
  }
  if (s < KB * KB) {
    return helper(KB, s, " KiB");
  }
  if (s < KB * KB * KB) {
    return helper(KB * KB, s, " MiB");
  }
#if SIZEOF_SIZE_T == 8
  if (s < KB * KB * KB * KB) {
    return helper(KB * KB * KB, s, " GiB");
  }
  if (s < KB * KB * KB * KB * KB) {
    return helper(KB * KB * KB * KB, s, " TiB");
  }
  return helper(KB * KB * KB * KB * KB, s, " PiB");
#else
  return helper(KB * KB * KB, s, " GiB");
#endif
}

std::string dirname(const std::string &p) {
  size_t s = p.rfind('/');
  if (s == std::string::npos)
    return "";
  return p.substr(0, s);
}

bool parseExtras(const std::string &s,
                 std::map<std::string, std::string> *extras) {
  std::vector<std::string> parts = splitString(s, ' ');

  for (auto &p : parts) {
    std::vector<std::string> kv = splitString(p, '=');
    if (kv.size() != 2)
      return false;
    (*extras)[kv[0]] = kv[1];
  }
  return true;
}

bool isValidBps(const std::string &b) {
  if (b == "300")
    return true;
  if (b == "600")
    return true;
  if (b == "1200")
    return true;
  if (b == "2400")
    return true;
  if (b == "4800")
    return true;
  if (b == "9600")
    return true;
  if (b == "14400")
    return true;
  if (b == "19200")
    return true;
  if (b == "38400")
    return true;
  if (b == "57600")
    return true;
  if (b == "115200")
    return true;
  return false;
}

bool findBytes(std::vector<uchar> &v, uchar a, uchar b, uchar c, size_t *out) {
  size_t p = 0;

  while (p + 2 < v.size()) {
    if (v[p + 0] == a && v[p + 1] == b && v[p + 2] == c) {
      *out = p;
      return true;
    }
    p++;
  }
  *out = 999999;
  return false;
}

std::string reverseBCD(const std::string &v) {
  int vlen = v.length();
  if (vlen % 2 != 0) {
    return "BADHEX:" + v;
  }

  std::string n = "";
  for (int i = 0; i < vlen; i += 2) {
    n += v[vlen - 2 - i];
    n += v[vlen - 1 - i];
  }
  return n;
}

std::string reverseBinaryAsciiSafeToString(const std::string &v) {
  std::vector<uchar> bytes;
  bool ok = hex2bin(v, &bytes);
  if (!ok)
    return "BADHEX:" + v;
  reverse(bytes.begin(), bytes.end());
  return safeString(bytes);
}

#define SLIP_END 0xc0     /* indicates end of packet */
#define SLIP_ESC 0xdb     /* indicates byte stuffing */
#define SLIP_ESC_END 0xdc /* ESC ESC_END means END data byte */
#define SLIP_ESC_ESC 0xdd /* ESC ESC_ESC means ESC data byte */

void addSlipFraming(std::vector<uchar> &from, std::vector<uchar> &to) {
  to.push_back(SLIP_END);
  for (uchar c : from) {
    if (c == SLIP_END) {
      to.push_back(SLIP_ESC);
      to.push_back(SLIP_ESC_END);
    } else if (c == SLIP_ESC) {
      to.push_back(SLIP_ESC);
      to.push_back(SLIP_ESC_ESC);
    } else {
      to.push_back(c);
    }
  }
  to.push_back(SLIP_END);
}

void removeSlipFraming(std::vector<uchar> &from, size_t *frame_length,
                       std::vector<uchar> &to) {
  *frame_length = 0;
  to.clear();
  to.reserve(from.size());
  bool esc = false;
  size_t i;
  bool found_end = false;

  for (i = 0; i < from.size(); ++i) {
    uchar c = from[i];
    if (c == SLIP_END) {
      if (to.size() > 0) {
        found_end = true;
        i++;
        break;
      }
    } else if (c == SLIP_ESC) {
      esc = true;
    } else if (esc) {
      esc = false;
      if (c == SLIP_ESC_END)
        to.push_back(SLIP_END);
      else if (c == SLIP_ESC_ESC)
        to.push_back(SLIP_ESC);
      else
        to.push_back(c); // This is an error......
    } else {
      to.push_back(c);
    }
  }

  if (found_end) {
    *frame_length = i;
  } else {
    *frame_length = 0;
    to.clear();
  }
}

// Check if hex string is likely to be ascii
bool isLikelyAscii(const std::string &v) {
  std::vector<uchar> val;
  bool ok = hex2bin(v, &val);

  // For example 64 bits:
  // 0000 0000 4142 4344
  // is probably the string DCBA

  if (!ok)
    return false;

  size_t i = 0;
  for (; i < val.size(); ++i) {
    if (val[i] != 0)
      break;
  }

  if (i == val.size()) {
    // Value is all zeroes, this is probably a number.
    return false;
  }

  for (; i < val.size(); ++i) {
    if (val[i] < 20 || val[i] > 126)
      return false;
  }

  return true;
}

std::string joinStatusOKStrings(const std::string &aa, const std::string &bb) {
  std::string a = aa;
  while (a.length() > 0 && a.back() == ' ')
    a.pop_back();
  std::string b = bb;
  while (b.length() > 0 && b.back() == ' ')
    b.pop_back();

  if (a == "" || a == "OK" || a == "null") {
    if (b == "" || b == "null")
      return "OK";
    return b;
  }
  if (b == "" || b == "OK" || b == "null") {
    if (a == "" || a == "null")
      return "OK";
    return a;
  }

  return a + " " + b;
}

std::string joinStatusEmptyStrings(const std::string &aa,
                                   const std::string &bb) {
  std::string a = aa;
  while (a.length() > 0 && a.back() == ' ')
    a.pop_back();
  std::string b = bb;
  while (b.length() > 0 && b.back() == ' ')
    b.pop_back();

  if (a == "" || a == "null") {
    if (b == "" || b == "null")
      return "";
    return b;
  }
  if (b == "" || b == "null") {
    if (a == "" || a == "null")
      return "";
    return a;
  }

  if (a != "OK" && b == "OK")
    return a;
  if (a == "OK" && b != "OK")
    return b;
  if (a == "OK" && b == "OK")
    return a;

  return a + " " + b;
}

std::string sortStatusString(const std::string &a) {
  std::set<std::string> flags;
  std::string curr;

  for (size_t i = 0; i < a.length(); ++i) {
    uchar c = a[i];
    if (c == ' ') {
      if (curr.length() > 0) {
        flags.insert(curr);
        curr = "";
      }
    } else {
      curr += c;
    }
  }

  if (curr.length() > 0) {
    flags.insert(curr);
    curr = "";
  }

  std::string result;
  for (auto &s : flags) {
    result += s + " ";
  }

  while (result.size() > 0 && result.back() == ' ')
    result.pop_back();

  // This feature is only used for the em24 deprecated backwards compatible
  // error field. This should go away in the future.
  replace(result.begin(), result.end(), '~', ' ');

  return result;
}

uchar *safeButUnsafeVectorPtr(std::vector<uchar> &v) {
  if (v.size() == 0)
    return NULL;
  return &v[0];
}

int strlen_utf8(const char *s) {
  int len = 0;
  for (; *s; ++s)
    if ((*s & 0xC0) != 0x80)
      ++len;
  return len;
}

std::string strTimestampUTC(double v) {
  char datetime[40];
  memset(datetime, 0, sizeof(datetime));
  time_t d = v;
  struct tm ts;
  gmtime_r(&d, &ts);
  strftime(datetime, sizeof(datetime), "%FT%TZ", &ts);
  return std::string(datetime);
}

int toMfctCode(char a, char b, char c) {
  return ((a - 64) * 1024 + (b - 64) * 32 + (c - 64));
}

bool is_lowercase_alnum_text(const char *text) {
  const char *i = text;
  while (*i) {
    char c = *i;
    if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'z'))) {
      return false;
    }
    i++;
  }
  return true;
}

bool endsWith(const std::string &str, const std::string &suffix) {
  return str.size() >= suffix.size() &&
         0 == str.compare(str.size() - suffix.size(), suffix.size(), suffix);
}

std::string lang_;

const std::string &language() {
  if (lang_.length() > 0)
    return lang_;

  const char *la = getenv("LANG");
  if (!la || strlen(la) < 2) {
    lang_ = "en";
  } else {
    if (la[2] == '_' || la[2] == 0) {
      lang_ = std::string(la, la + 2);
    } else {
      lang_ = "en";
    }
  }

  return lang_;
}

TestBit toTestBit(const char *s) {
  if (!strcmp(s, "Set"))
    return TestBit::Set;
  if (!strcmp(s, "NotSet"))
    return TestBit::NotSet;
  return TestBit::Unknown;
}
