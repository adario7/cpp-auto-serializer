#include <pos_t.hh>
#include <iostream>

using namespace std;

ostream& operator<<(ostream& os, const pos_t& p) {
	os << p.line << ":" << p.col;
    return os;
}

string to_string(const pos_t& p) {
	return to_string(p.line) + ":" + to_string(p.col);
}

ostream& operator<<(ostream& os, const segment_t& p) {
	os << p.start << "->" << p.end;
    return os;
}

string to_string(const segment_t& p) {
	return to_string(p.start) + "->" + to_string(p.end);
}

