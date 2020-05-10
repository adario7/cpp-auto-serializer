#pragma once

#include <string>

struct pos_t {
	int line, col;
};

struct segment_t {
	pos_t start, end;
};

std::ostream& operator<<(std::ostream& os, const pos_t& p);
std::ostream& operator<<(std::ostream& os, const segment_t& p);

std::string to_string(const pos_t& p);
std::string to_string(const segment_t& p);
