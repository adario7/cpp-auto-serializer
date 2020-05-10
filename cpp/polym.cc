#include <unordered_map>

#include <types.hh>
#include <node.hh>

using namespace std;

// wrappers for pointers to NType, as default pointer operators won't call the pointed value
struct NTypeHash {
	hash<NType> hasher;
	size_t operator()(const NType* x) const { return hasher(*x); }
};
struct NTypeEqu {
	size_t operator()(const NType* a, const NType* b) const { return *a == *b; }
};

// maps a type to its polymorphic-factory-map index
unordered_map<const NType*, int, NTypeHash, NTypeEqu> polymMap;
// start at 1, as 0 means not polymorphic
int polym_counter = 1;

void register_polym(NPolym* np, ostream& dout) {
	NType* k = np->subject;
	np->children->push_back(k); // a polymorphic type could also be itself
	auto itr = polymMap.find(k);
	if (itr != polymMap.end())
		throw runtime_error("error at " + to_string(k->pos) + ": polymorphic children of '"
			+ to_string(*k) + "' declared twice");
	int n = polymMap[k] = polym_counter++;

	dout << "static unordered_map<string, function<void*("
		<< "istream&, function<bool(string)>, unordered_map<size_t,__deserialization_ptr>&"
		<< ")>>  __polym_map_" << n << " = {" << endl;
	for (const NType* _child : *np->children) {
		const NType& child = *_child;
		dout << "\t{\"" << child << "\", [](istream& __s, function<bool(string)> __e, unordered_map<size_t, __deserialization_ptr>& __pm) -> void* {" << endl
			<< "\t\t" << child << "* __v = new " << child << "();" << endl
			<< "\t\t" << child << "& __r = *__v;" << endl;
		deserialize_value("__r", child, dout);
		dout << "\t\treturn __v;" << endl
			<< "\t} }," << endl;
	}
	dout << "};" << endl;
}

int getPolymOf(const NType* in) {
	auto itr = polymMap.find(in);
	if (itr != polymMap.end()) return itr->second;
	else return 0; // 0 = not polymorphic
}
