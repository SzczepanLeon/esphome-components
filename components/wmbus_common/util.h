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

#ifndef UTIL_H
#define UTIL_H

#include <signal.h>
#include <cstdint>
#include <string>
#include <functional>
#include <map>
#include <set>
#include <vector>

#include "esphome/core/log.h"

void setVersion(const char *v);
const char *getVersion();

typedef unsigned char uchar;

enum class TestBit
{
    Unknown,
    Set,
    NotSet
};

uchar bcd2bin(uchar c);
uchar revbcd2bin(uchar c);
uchar reverse(uchar c);
// A BCD string 102030405060 is reversed to 605040302010
std::string reverseBCD(const std::string &v);
// A hex string encoding ascii chars is reversed and safely translated into a readble string.
std::string reverseBinaryAsciiSafeToString(const std::string &v);
// Check if hex string is likely to be ascii
bool isLikelyAscii(const std::string &v);

bool isHexChar(uchar c);

// Flex strings contain hexadecimal digits and permit # | and whitespace.
bool isHexStringFlex(const char *txt, bool *invalid);
bool isHexStringFlex(const std::string &txt, bool *invalid);
// Strict strings contain only hexadecimal digits.
bool isHexStringStrict(const char *txt, bool *invalid);
bool isHexStringStrict(const std::string &txt, bool *invalid);
int char2int(char input);
bool hex2bin(const char *src, std::vector<uchar> *target);
bool hex2bin(const std::string &src, std::vector<uchar> *target);
bool hex2bin(std::vector<uchar> &src, std::vector<uchar> *target);
std::string bin2hex(const std::vector<uchar> &target);
std::string bin2hex(std::vector<uchar>::iterator data, std::vector<uchar>::iterator end, int len);
std::string bin2hex(std::vector<uchar> &data, int offset, int len);
std::string safeString(std::vector<uchar> &target);
void strprintf(std::string *s, const char *fmt, ...);
std::string tostrprintf(const char *fmt, ...);
std::string tostrprintf(const std::string *fmt, ...);
bool endsWith(const std::string &str, const std::string &suffix);

// Return for example: 2010-03-21
std::string strdate(struct tm *date);
std::string strdate(double v);
// Return for example: 2010-03-21 15:22
std::string strdatetime(struct tm *date);
std::string strdatetime(double v);
// Return for example: 2010-03-21 15:22:03
std::string strdatetimesec(struct tm *date);
std::string strdatetimesec(double v);
// Return UTC timestamp:
std::string strTimestampUTC(double v);
void addMonths(struct tm *date, int m);
double addMonths(double t, int m);

bool stringFoundCaseIgnored(const std::string &haystack, const std::string &needle);

void xorit(uchar *srca, uchar *srcb, uchar *dest, int len);
void shiftLeft(uchar *srca, uchar *srcb, int len);
std::string format3fdot3f(double v);

#define trace(...) esph_log_d("wmbusmeters", __VA_ARGS__)
#define verbose(...) esph_log_d("wmbusmeters", __VA_ARGS__)
#define debug(...) esph_log_v("wmbusmeters", __VA_ARGS__)
#define notice(...) esph_log_i("wmbusmeters", __VA_ARGS__)
#define warning(...) esph_log_w("wmbusmeters", __VA_ARGS__)
#define error(...) esph_log_e("wmbusmeters", __VA_ARGS__)

void debugPayload(const std::string &intro, std::vector<uchar> &payload);
void debugPayload(const std::string &intro, std::vector<uchar> &payload, std::vector<uchar>::iterator &pos);
void logTelegram(std::vector<uchar> &original, std::vector<uchar> &parsed, int header_size, int suffix_size);

enum class Alarm
{
    DeviceFailure,
    RegularResetFailure,
    DeviceInactivity,
    SpecifiedDeviceNotFound
};

const char *toString(Alarm type);
void setAlarmShells(std::vector<std::string> &alarm_shells);

bool isValidAlias(const std::string &alias);

bool isNumber(const std::string &fq);

// Split s into strings separated by c.
std::vector<std::string> splitString(const std::string &s, char c);
// Split s into strings separated by c and store inte set.
std::set<std::string> splitStringIntoSet(const std::string &s, char c);
// Split device string cul:c1:CMD(bar 1:2) into cul c1 CMD(bar 1:2)
// I.e. the : colon inside CMD is not used for splitting.
std::vector<std::string> splitDeviceString(const std::string &s);

void incrementIV(uchar *iv, size_t len);

std::string eatTo(std::vector<uchar> &v, std::vector<uchar>::iterator &i, int c, size_t max, bool *eof, bool *err);

void padWithZeroesTo(std::vector<uchar> *content, size_t len, std::vector<uchar> *full_content);
std::string padLeft(const std::string &input, int width);

// Parse text string into seconds, 5h = (3600*5) 2m = (60*2) 1s = 1
int parseTime(const std::string &time);

// Test if current time is inside any of the specified periods.
// For example: mon-sun(00-24) is always true!
//              mon-fri(08-20) is true monday to friday from 08.00 to 19.59
//              tue(09-10),sat(00-24) is true tuesday 09.00 to 09.59 and whole of saturday.
bool isInsideTimePeriod(time_t now, std::string periods);
bool isValidTimePeriod(const std::string &periods);

uint16_t crc16_EN13757(uchar *data, size_t len);

// This crc is used by im871a for its serial communication.
uint16_t crc16_CCITT(uchar *data, uint16_t length);
bool crc16_CCITT_check(uchar *data, uint16_t length);

void addSlipFraming(std::vector<uchar> &from, std::vector<uchar> &to);
// Frame length is set to zero if no frame was found.
void removeSlipFraming(std::vector<uchar> &from, size_t *frame_length, std::vector<uchar> &to);

// Eat characters from the vector v, iterating using i, until the end char c is found.
// If end char == -1, then do not expect any end char, get all until eof.
// If the end char is not found, return error.
// If the maximum length is reached without finding the end char, return error.
std::string eatTo(std::vector<char> &v, std::vector<char>::iterator &i, int c, size_t max, bool *eof, bool *err);
// Eat whitespace (space and tab, not end of lines).
void eatWhitespace(std::vector<char> &v, std::vector<char>::iterator &i, bool *eof);
// First eat whitespace, then start eating until c is found or eof. The found string is trimmed from beginning and ending whitespace.
std::string eatToSkipWhitespace(std::vector<char> &v, std::vector<char>::iterator &i, int c, size_t max, bool *eof, bool *err);
// Remove leading and trailing white space
void trimWhitespace(std::string *s);
// Count the number of 1:s in the binary number v.
int countSetBits(int v);

bool startsWith(const std::string &s, const char *prefix);
bool startsWith(const std::string &s, std::string &prefix);

// Given alfa=beta it returns "alfa":"beta"
std::string makeQuotedJson(const std::string &s);

std::string currentYear();
std::string currentYearMonth();
std::string currentYearMonthDay();
std::string currentHour();
std::string currentMinute();
std::string currentSeconds();
std::string currentMicros();

#define CHECK(n)                  \
    if (!hasBytes(n, pos, frame)) \
        return expectedMore(__LINE__);

bool hasBytes(int n, std::vector<uchar>::iterator &pos, std::vector<uchar> &frame);

bool startsWith(const std::string &s, std::vector<uchar> &data);

// Sum the memory used by the heap and stack.
size_t memoryUsage();

std::string humanReadableTwoDecimals(size_t s);

uint32_t indexFromRtlSdrName(const std::string &s);

bool check_if_rtlwmbus_exists_in_path();
bool check_if_rtlsdr_exists_in_path();

// Return the actual executable binary that is running.
std::string currentProcessExe();

std::string dirname(const std::string &p);

std::string lookForExecutable(const std::string &prog, std::string bin_dir, std::string default_dir);

// Extract from "ppm=5 radix=7" and put key values into map.
bool parseExtras(const std::string &s, std::map<std::string, std::string> *extras);
void checkIfMultipleWmbusMetersRunning();

bool findBytes(std::vector<uchar> &v, uchar a, uchar b, uchar c, size_t *out);

enum class OutputFormat
{
    NONE,
    PLAIN,
    TERMINAL,
    JSON,
    HTML
};

// Joing two status strings with a space, but merge OKs.
// I.e. "OK" + "OK" --> "OK"
//      "ERROR" + "OK"  --> "ERROR"
//      "OK" + "ERROR FLOW" --> "ERROR FLOW"
//      "ERROR" + "FLOW"    --> "ERROR FLOW"
//      It also translates empty strings into OK.
//      "" + "OK" --> "OK"
//      "" + "" --> "OK"
std::string joinStatusOKStrings(const std::string &a, const std::string &b);

// Same but do not introduce OK, keep empty strings empty.
std::string joinStatusEmptyStrings(const std::string &a, const std::string &b);

// Sort the words in a status string: "GAMMA BETA ALFA" --> "ALFA BETA GAMMA"
// Also identical flags are merged: "BETA ALFA ALFA" --> "ALFA BETA"
// Finally ~ is replaced with a space, this is only used for backwards compatibilty for deprecated fields.
std::string sortStatusString(const std::string &a);

// If a vector is empty, then there will be an assert (with some compiler flags) if we use &v[0]
// even if we do not intend to actually use the pointer to uchars!
// So provide safeVectorPtr which will return NULL instead of assert-crashing.
uchar *safeButUnsafeVectorPtr(std::vector<uchar> &v);

// Count utf8 unicode code points.
int strlen_utf8(const char *s);

int toMfctCode(char a, char b, char c);

bool is_lowercase_alnum_text(const char *text);

// The language that the user expects driver and other messages in.
const std::string &language();

TestBit toTestBit(const char *s);

#ifndef FUZZING
#define FUZZING false
#endif

#endif
