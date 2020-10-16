#include <unordered_map>
#include <string>
#include <functional>

#include <types.hh>
#include <node.hh>
#include <polym.hh>

using namespace std;

#define DEREF_AS(new_type, ptr) (*((new_type*) (ptr)))

using rw_function = function<void(const string&, const NType&, ostream&)>;
struct rw_pair { rw_function read, write; };
#define _P(name) rw_pair{r_##name, w_##name}

extern rw_pair rw_object, rw_static_array; // declared down

std::unordered_map<std::string, rw_pair> types_map;
std::unordered_map<std::string, const NType*> alias_map;

/* finds an appropriate `rw_pair` for the type `t`.
 * if the type is found to be an alias, it is replaced by the real type
 * to avoid losing generic types information (hence the reference to pointer) */
const rw_pair& find_type_pair(const NType*& t) {
	// recursively resolve aliases
	while (true) {
		// arrays are saved like: int[`5`] --> []<int>
		if (t->isArray) return rw_static_array;

		auto itr = alias_map.find(t->name->value);
		if (itr == alias_map.end()) break;
		t = itr->second;
	}

	const string tname = t->name->value;
	auto itr = types_map.find(tname);
	if (itr == types_map.end()) return rw_object;
	return itr->second;
}

void serialize_value(const std::string& fname, const NType& t, const rw_pair& pair, std::ostream& o) {
	pair.write(fname, t, o);
}
void serialize_value(const std::string& fname, const NType& t, std::ostream& o) {
	const NType* real_t = &t;
	const rw_pair& pair = find_type_pair(real_t);
	serialize_value(fname, *real_t, pair, o);
}

void serialize_field(const std::string& fname, const NType& orig_t, std::ostream& o) {
	// find the pair immediately to reasolve aliases, and use only resolved names in the output file
	const NType* real_t = &orig_t;
	const rw_pair& pair = find_type_pair(real_t);
	o << "\t__s << \"" << fname << " " << *real_t << " \";" << endl;
	serialize_value(fname, *real_t, pair, o);
	o << "\t__s << endl;" << endl;
}

void deserialize_value(const std::string& fname, const NType& t, const rw_pair& pair, std::ostream& o) {
	pair.read(fname, t, o);
}
void deserialize_value(const std::string& fname, const NType& t, std::ostream& o) {
	// find the pair immediately to reasolve aliases, and use only resolved names in the output file
	const NType* real_t = &t;
	const rw_pair& pair = find_type_pair(real_t);
	deserialize_value(fname, *real_t, pair, o);
}

void deserialize_field(const std::string& fname, const NType& orig_t, std::ostream& o) {
	const NType* real_t = &orig_t;
	const rw_pair& pair = find_type_pair(real_t);
	o << "\t\t\t__TYPE_CHK(\"" << *real_t << "\");" << endl
		<< "\t\t\tauto& " << fname << " = __v." << fname << ";" << endl;
	deserialize_value(fname, *real_t, pair, o);
}

#define _NATIVE_M(type) \
	void w_##type(const string& fname, const NType&, ostream& o) { \
		o << "\t__s << ((" << #type << ") " << fname << ");" << endl; \
	} \
	void r_##type(const string& fname, const NType&, ostream& o) { \
		o << "\t\t\t__s >> ((" << #type << "&) " << fname << ");" << endl; \
	}

_NATIVE_M(bool)
_NATIVE_M(int8_t) _NATIVE_M(int16_t) _NATIVE_M(int32_t) _NATIVE_M(int64_t)
_NATIVE_M(uint8_t) _NATIVE_M(uint16_t) _NATIVE_M(uint32_t) _NATIVE_M(uint64_t)
_NATIVE_M(float) _NATIVE_M(double)

void w_string(const string& fname, const NType&, ostream& o) {
	o << "\t__s << " << fname << ".size() << ' ' << " << fname << ";" << endl;
}

void r_string(const string& fname, const NType&, ostream& o) {
	o << "\t\t\tsize_t __" << fname << "_sz; ; __s >> __" << fname << "_sz;" << endl
		<< "\t\t\t" << fname << ".resize(__" << fname << "_sz);" << endl
		<< "\t\t\t__s.ignore(1);" << endl // skip the whitespace separator
		<< "\t\t\t__s.get(&" << fname << ".front(), __" << fname << "_sz+1, '\\0');" << endl; // +1 because .get reads n-1 chars
}

#define _GENERATE_FOR_SZ(sz_value)					 \
	"\tfor (size_t __i_" << fname << " = 0; "	  \
	<< "__i_" << fname << " < " << sz_value << "; "	\
	<< "__i_" << fname << "++) {" << endl
#define _GENERATE_FOR _GENERATE_FOR_SZ("__" << fname << "_sz")

void w_static_array(const string& fname, const NType& t, ostream& o) {
	const NType& e_t = *(*t.generics)[0];
	const string& size = t.name->value;	
	o << "\t__s << (" << size << ");" << endl
		<< _GENERATE_FOR_SZ("(" << size << ")")
		<< "\tconst auto& __e_" << fname << " = " << fname << "[__i_" << fname << "];" << endl
		<< "\t__s << ' ';" << endl;
	serialize_value("__e_" + fname, e_t, o);
	o << "\t}" << endl;
}

void r_static_array(const string& fname, const NType& t, ostream& o) {
	const NType& e_t = *(*t.generics)[0];
	const string& size = t.name->value;	
	o << "\t\t\tsize_t __" << fname << "_sz; __s >> __" << fname << "_sz;" << endl
		<< "\t\t\tif (__" << fname << "_sz != (" << size << ")) "
		<< "if (__e(\"wrong static array size: got \" + to_string(__" << fname << "_sz)"
		<< "+ \", expected \" + to_string(" << size << "))) return true;" << endl
		<< "\t\t" << _GENERATE_FOR
		<< "\t\t\tconst auto& __e_" << fname << " = " << fname << "[__i_" << fname << "];" << endl;
	deserialize_value("__e_" + fname, e_t, o);
	o << "\t}" << endl;
}

void w_vector(const string& fname, const NType& t, ostream& o) {
	GenericsList& list = *t.generics;
	if (list.size() < 1) throw runtime_error("std::vector, std:set or std::unordered_set are expected to have at least one generic type, but got: " + to_string(t));
	const NType& e_t = *list[0];
	o << "\t__s << " << fname << ".size() << ' '; " << endl
		<< "\tfor (const auto& __e_" << fname << " : " << fname << ") {" << endl;
	serialize_value("__e_" + fname, e_t, o);
	o << "\t__s << ' ';" << endl;
	o << "\t}" << endl;
}

void r_vector(const string& fname, const NType& t, ostream& o) {
	const NType& e_t = *(*t.generics)[0];  // checks already performed when writing
	o << "\t\t\tsize_t __" << fname << "_sz; __s >> __" << fname << "_sz;" << endl;
	// can't preallocate sets and unorderes_sets
	if (t.name->value == "vector" || t.name->value == "std::vector") {
		o << "\t\t\t" << fname << ".resize(__" << fname << "_sz);" << endl
			<< "\t\t" << _GENERATE_FOR
			<< "\t\t\tauto& __e_" << fname << " = " << fname << "[__i_" << fname << "];" << endl;
		deserialize_value("__e_" + fname, e_t, o);
	} else {
		o << "\t\t" << _GENERATE_FOR
			<< "\t\t\t" << e_t << " __e_" << fname << ";" << endl;
		deserialize_value("__e_" + fname, e_t, o);
		o << "\t\t\t" << fname << ".insert(__e_" << fname << ");" << endl;
	}
	o << "\t\t\t}" << endl;
}

void w_map(const string& fname, const NType& t, ostream& o) {
	GenericsList& list = *t.generics;
	if (list.size() < 2) throw runtime_error("std::map or std::unordered_map are expected to have at least two generic types, but got: " + to_string(t));
	const NType& k_t = *list[0], &v_t = *list[1];
	o << "\t__s << " << fname << ".size() << ' '; " << endl
		<< "\tfor (const auto& __e_" << fname << " : " << fname << ") {" << endl
		<< "\tconst auto& __k_" << fname << " = __e_" << fname << ".first; "
		<< "const auto& __v_" << fname << " = __e_" << fname << ".second;" << endl;
	serialize_value("__k_" + fname, k_t, o);
	o << "\t__s << ' ';" << endl;
	serialize_value("__v_" + fname, v_t, o);
	o << "\t__s << ' ';" << endl;
	o << "\t}" << endl;
}

void r_map(const string& fname, const NType& t, ostream& o) {
	GenericsList& list = *t.generics;
	const NType& k_t = *list[0], &v_t = *list[1]; // checks already performed when writing
	o << "\t\t\tsize_t __" << fname << "_sz; ; __s >> __" << fname << "_sz;" << endl
		<< "\t\t" << _GENERATE_FOR
		<< "\t\t\t" << to_cpp_type(k_t) << " __k_" << fname << ";" << endl;
	deserialize_value("__k_" + fname, k_t, o);
	o << "\t\t\t" << to_cpp_type(v_t) << "& __v_" << fname << " = " << fname << "[__k_" << fname << "];" << endl;
	deserialize_value("__v_" + fname, v_t, o);
	o << "\t\t\t}" << endl;
}

void r_object(const string& fname, const NType& t, ostream& o) {
	o << "\t\t\t" << fname << "._deserialize_from(__s, __e, __pm);" << endl;	
}

void w_object(const string& fname, const NType& t, ostream& o) {
	o << "\t" << fname << "._serialize_to(__s, __pm);" << endl;	
}

void r_pointer(const string& fname, const NType& t, ostream& o) {
	// tell the root deserializer that the pointer needs to be filled here
	const NType* ptr_pointed_t = (*t.generics)[0];
	const NType& pointed_t = *ptr_pointed_t;
	o << "\t\t\tsize_t __p_" << fname << "; __s >> __p_" << fname << ";" << endl
		<< "\t\t\tif (__p_" << fname << " == 0) {" << endl
		<< "\t\t\t" << fname << " = nullptr;" << endl
		<< "\t\t\t" << "} else {" << endl
		<< "\t\t\t__deserialization_ptr& __d_" << fname << " = __pm[__p_" << fname << "];" << endl
		<< "\t\t\t__d_" << fname << ".refs.push_back(&" << fname << ");" << endl
		<< "\t\t\t__d_" << fname << ".fun = [&, __e]() -> void* {" << endl;
	if (&find_type_pair(ptr_pointed_t) == &rw_object) {
		// for serializable objects, use the deserialize_to_ptr, which handles polymorphism
		o << "\t\t\treturn " << pointed_t << "::_deserialize_to_ptr(__s, __e, __pm);" << endl;
	} else {
		// for other types, deserialize as usual
		o << "\t\t\t" << pointed_t << "* __v_" << fname << " = new " << pointed_t << "();" << endl
			<< "\t\t\t" << pointed_t << "& __r_" << fname << " = *__v_" << fname << ";" << endl;
		deserialize_value("__r_" + fname, pointed_t, o);
		o << "\t\t\treturn __v_" << fname << ";" << endl;
	}
	o << "\t\t\t};" << endl
		<< "\t\t\t}" << endl;
}

void w_pointer(const string& fname, const NType& t, ostream& o) {
	// tell the root serializer to serialize this pointer later
	const NType& pointed_t = *(*t.generics)[0];
	o << "\t__s << ((size_t) " << fname << ");" << endl
		<< "\tif (" << fname << ") {" << endl // don't serialize null pointers
		<< "\t" << pointed_t << "& __p_" << fname << " = *" << fname << ";" << endl
		<< "\t__pm[" << fname << "] = [&](){" << endl;
	serialize_value("__p_" + fname, pointed_t, o);
	o << "\t};" << endl
		<< "\t}" << endl;
}

// forward declared at the beginning of the file
rw_pair rw_object = _P(object);
rw_pair rw_static_array = _P(static_array);

// shortcut macros for accessing `type_map`
#define _T(x) types_map[#x]
#define _STD_T(x) _T(x) = _T(std::x) // `string` and `std::string`
#define _US_T(x) _T(u ## x) = _T(unsigned x) // `uint` and `unsigned int`

void init_types() {
	_T(bool) = _P(bool);
	_T(char) = _P(int8_t);
	_US_T(char) = _P(uint8_t);
	_T(int8_t) = _P(int8_t);
	_T(int16_t) = _P(int16_t);
	_T(int32_t) = _P(int32_t);
	_T(int64_t) = _P(int64_t);
	_T(uint8_t) = _P(uint8_t);
	_T(uint16_t) = _P(uint16_t);
	_T(uint32_t) = _P(uint32_t);
	_T(uint64_t) = _P(uint64_t);
	_T(int) = sizeof(int) == 4 ? _P(int32_t) : _P(int16_t);
	_US_T(int) = sizeof(int) == 4 ? _P(uint32_t) : _P(uint16_t);
	_T(long) = sizeof(long) == 8 ? _P(int64_t) : _P(int32_t);
	_US_T(long) = sizeof(long) == 4 ? _P(uint64_t) : _P(uint32_t);
	_T(long long) = _P(int64_t);
	_US_T(long long) = _P(uint64_t);
	_T(float) = _P(float);
	_T(double) = _P(double);
	_T(*) = _P(pointer); // pointers are saved like: int* --> *<int>
	_STD_T(string) = _P(string);
	_STD_T(vector) = _STD_T(set) = _STD_T(unordered_set) = _P(vector);
	_STD_T(map) = _STD_T(unordered_map) = _P(map);
}

void add_alias(const segment_t pos, const string& name, const NType* real) {
	auto itr = alias_map.find(name);
	if (itr != alias_map.end())
		throw runtime_error("at " + to_string(pos) + ": redeclaration of alias " + name);
	alias_map[name] = real;
}
