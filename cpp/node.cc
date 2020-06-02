#include <node.hh>
#include <ostream>
#include <sstream>

using namespace std;

// converts to the internal name format
string to_string(const NType& t) {
	stringstream ss;
	ss << t;
	return ss.str();
}

// converts to the internal name format
ostream& operator<<(ostream& o, const NType& t) {
	if (t.isArray) // `name` is the array size
		o << "[]";
	else // `name` is the actual type
		o << t.name->value;
	GenericsList& list = *t.generics;
	if (list.empty()) return o;
	o << "<";
	for (int i = 0; i < list.size(); i++) {
		o << *list[i];
		if (i != list.size() - 1) o << ","; // no whitespaces!
	}
	o << ">";
	return o;
}

// converts to the C++ name format
string to_cpp_type(const NType& t) {
	GenericsList& list = *t.generics;
	string& name = t.name->value;
	if (t.isArray) // `name` is the array size
		return to_cpp_type(*list[0]) + "[" + t.name->value + "]";
	if (name == "*")
		return to_cpp_type(*list[0]) + "*";
	if (list.empty())
		return name;
	string res = name;
	res += "<";
	for (int i = 0; i < list.size(); i++) {
		res += to_cpp_type(*list[i]);
		if (i != list.size() - 1) res += ","; // no whitespaces!
	}
	res += ">";
	return res;
}

bool operator==(const NType& a, const NType& b) {
	if (a.name->value != b.name->value) return false;
	if (a.generics->size() != b.generics->size()) return false;
	for (int i = 0; i < a.generics->size(); i++)
		if ((*a.generics)[i] != (*b.generics)[i]) return false;
	return true;
}

size_t hash<NType>::operator()(const NType& t) const {
	size_t h = hash<string>()(t.name->value);
	for (const NType* o : *t.generics) {
		h *= 7;
		h += 13 * (*this)(*o); // (*this) is this hash functor
	}
	return h;
}
