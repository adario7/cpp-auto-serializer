#include <node.hh>
#include <parser.hh>
#include <types.hh>
#include <polym.hh>

#include <iostream>
#include <unordered_map>
#include <fstream>
#include <typeinfo>

extern FILE* yyin;
extern int yyparse();

using namespace std;

#define IF_TYPE(val, newtype, newname) \
	if (newtype* newname = dynamic_cast<newtype*>(val))
// the __LINE__ is an int, must be strigified
#define __S(x) #x
#define _S(x) __S(x)
#define TYPE_UNKN(var) \
	else throw runtime_error(string(__FILE__  ":" _S(__LINE__) " - unknown type for " #var ": ") + typeid(*var).name());

ofstream hout, // header-out (.hh)
		 dout; // data-out   (.cc)

void compileRoot(NPolym* np) {
	register_polym(np, dout);
}

void compileRoot(NAlias* alias) {
	string name_s = to_string(*alias->name);
	add_alias(alias->pos, name_s, alias->real);
	if (alias->in_code) { // `using` is reflected on the header, `alias` is not
		hout << "using " << name_s << " = " << to_cpp_type(*alias->real) << ";" << endl;
	}
}

void compileCode(string* value) {
	if (value) // code blocks can be optional
		hout << *value;
}

void compileRoot(NCode* code_node) {
	compileCode(code_node->code);
	hout << endl;
}

// compile into the real header and into the serialization function
void compileBlock(NStruct* st, NVarBlock* block) {
	hout << "\t" << to_cpp_type(*block->type);
	VarDeclList& list = *block->vars;
	for (int i = 0; i < list.size(); i++) {
		NVarDeclaration& dec = *list[i];
		string& fname = dec.name->value;
		// insert in the header
		hout << " ";
		for (int j = 0; j < dec.tSuffixes->size(); j++)
			hout << "*"; // can't be anything else
		hout << fname;
		for (string* size : *dec.arraySuffixes)
			hout << "[" << *size << "]";
		if (dec.assignment)
			hout << " = " << *dec.assignment;
		if (i != list.size() - 1) hout << ",";
		else hout << ";" << endl;
		// insert in the data
		serialize_field(fname, *dec.completeType, dout);
	}
}

// compile into deserialization function
void compileDsBlock(NStruct* st, NVarBlock* block) {
	VarDeclList& list = *block->vars;
	for (NVarDeclaration* decl_ptr : list) {
		NVarDeclaration& dec = *decl_ptr;
		string& fname = dec.name->value;
		dout << "\t\t{\"" << fname << "\", [&, __e](" << *st->name << "& __v) -> bool {" << endl;
		deserialize_field(fname, *dec.completeType, dout);
		dout << "\t\t\treturn 1;" << endl // got to the end -> success
			<< "\t\t} }," << endl;
	}
}

void compileRoot(NStruct* st) {
	// header preface
	hout << (st->isClass ? "class " : "struct ") << *st->name;
	if (int parents_len = st->parents->size()) {
		hout << " : ";
		int i = 0;
		for (NParent* p : *st->parents) {
			compileCode(p->code_prev);
			hout << " " << *p->type << " "; // add spaces to separate from code blocks
			if (++i < parents_len) hout << ",";
		}
	}
	compileCode(st->code_parents);
	hout << " {" << endl;
	// data preface
	dout << "#undef __AS_CTX" << endl // prevent compilation warnings
		<< "#define __AS_CTX \"" << *st->name << "\"s" << endl << endl
		<< "void " << *st->name << "::_serialize_to(ostream& __s, unordered_map<void*, function<void()>>& __pm) const {" << endl;
	size_t fields_count = 0;
	for (NBodyElem* elem : *st->body)
		IF_TYPE(elem, NVarBlock, block)
			fields_count += block->vars->size();
	dout << "\t__s << \"" << *st->name << " " << fields_count << "\" << \"\\n\";" << endl;
	// before fileds, serialize parent classes
	for (NParent* p : *st->parents)
		dout << "\t" << *p->type << "::_serialize_to(__s, __pm);" << endl;
	// header & serialization body
	for (NBodyElem* elem : *st->body) {
		IF_TYPE(elem, NCode, code) {
			compileRoot(code); // this might not be very flexible
		} else IF_TYPE(elem, NVarBlock, block) {
			compileBlock(st, block);
		} TYPE_UNKN(elem);
	}
	// header ending
	hout << "public:" << endl;
	hout << "\t"; if (st->isVirtual) hout << "virtual ";
	hout << "void serialize_to(std::ostream& output) const;" << endl
		<< "\tbool deserialize_from(std::istream& source, std::function<bool(std::string)> error_callback);" << endl;
	hout << "\t"; if (st->isVirtual) hout << "virtual ";
	hout << "void _serialize_to(std::ostream& output, std::unordered_map<void*, std::function<void()>>& pm) const;" << endl
		<< "\tbool _deserialize_from(std::istream& source, std::function<bool(std::string)> error_callback, std::unordered_map<size_t, __deserialization_ptr>& pm);" << endl
		<< "\tstatic void* _deserialize_to_ptr(std::istream& source, std::function<bool(std::string)> error_callback, std::unordered_map<size_t, __deserialization_ptr>& pm);" << endl
		<< "};" << endl << endl;
	// data ending
	dout << "}" << endl << endl;
	dout << "bool " << *st->name << "::_deserialize_from(istream& __s, function<bool(string)> __e, unordered_map<size_t, __deserialization_ptr>& __pm) {" << endl
		<< "\t__TYPE_CHK(\"" << *st->name << "\");" << endl
		<< "\tsize_t __count; __s >> __count;" << endl
		<< "\tif (__count != " << fields_count << ") ""if (__e( \"'" << *st->name << "': read \" + to_string(__count) + \" fields, expected " << fields_count << "\")) return 0;" << endl;
	// before fileds, deserialize parent classes
	for (NParent* p : *st->parents)
		dout << "\t" << *p->type << "::_deserialize_from(__s, __e, __pm);" << endl;
	dout << "\tstatic const unordered_map<string, function<bool(" << *st->name << "&)>> __map = {" << endl;
	// add deserialization
	for (NBodyElem* elem : *st->body) {
		IF_TYPE(elem, NVarBlock, block)
			compileDsBlock(st, block);
	}
	dout << "\t};" << endl
		<< "\tfor (size_t __i = 0; __i < __count; __i++) {" << endl
		<< "\t\tstring __fn; __s >> __fn;" << endl
		<< "\t\tconst auto& __itr = __map.find(__fn);" << endl
		<< "\t\tif (__itr == __map.end()) if(__e(\"'" << *st->name << "': unknown field '\" + __fn + \"'\")) return 0;" << endl
		// return if the field fails deserializing
		<< "\t\tif (!__itr->second(*this)) return 0;" << endl
		<< "\t}" << endl
		<< "\treturn 1;" << endl // got to the end -> success
		<< "}" << endl << endl;
	// implement user-side methods
	dout << "void " << *st->name << "::serialize_to(ostream& __s) const {" << endl
		<< "\tunordered_map<void*, function<void()>> __pm;" << endl
		<< "\t_serialize_to(__s, __pm);" << endl
		<< "\tunordered_set<void*> __done;" << endl
		<< "\twhile (!__pm.empty()) {" << endl
		// avoid adding to the map while iterating
		<< "\t\tauto __pmc = __pm;" << endl
		<< "\t\t__pm.clear();" << endl
		<< "\t\tfor (const auto& __p : __pmc) {" << endl
		<< "\t\t\tif (__done.find(__p.first) != __done.end()) continue;" << endl // avoid endless recursion
		<< "\t\t\telse __done.insert(__p.first);" << endl
		<< "\t\t\t__s << ((size_t) __p.first) << ' ';" << endl
		<< "\t\t\t__p.second();" << endl
		<< "\t\t\t__s << \"\\n\";" << endl
		<< "\t\t}" << endl
		<< "\t}" << endl
		<< "}" << endl << endl;
	dout << "bool " << *st->name << "::deserialize_from(std::istream& __s, function<bool(string)> __e) {" << endl
		<< "\tunordered_map<size_t, __deserialization_ptr> __pm;" << endl
		<< "\tif (!_deserialize_from(__s, __e, __pm)) return 0;" << endl
		<< "\tunordered_map<size_t, void*> __done;" << endl
		<< "\twhile (!__pm.empty()) {" << endl
		<< "\t\twhile (\" \\t\\n\\r\"s.find(__s.peek()) != string::npos) { __s.ignore(); }" << endl
		<< "\t\tif (__s.peek() == EOF) break;" << endl
		<< "\t\tsize_t __k; __s >> __k;" << endl
		<< "\t\tconst auto& __it = __pm.find(__k);" << endl
		<< "\t\tif (__it == __pm.end()) if (__e(\"definition of undeclared pointer: \" + to_string(__k))) return 0;" << endl
		// avoid modifying the map while iterating
		<< "\t\tconst auto __fun = __it->second.fun;" << endl
		<< "\t\tconst auto __refs = move(__it->second.refs);" << endl
		<< "\t\t__pm.erase(__it);" << endl
		<< "\t\tvoid* __v;" << endl
		<< "\t\tconst auto& __d_it = __done.find(__k);" << endl
		// function callback returning nullptr indicates a failure
		<< "\t\tif (__d_it == __done.end()) { __v = __done[__k] = __fun(); if (!__v) return 0; }" << endl
		<< "\t\telse __v = __d_it->second;" << endl // avoid endless recursion
		<< "\t\t__FILL_REFS;" << endl
		<< "\t}" << endl
		// expect all remaining entries to be in the `__done` map
		<< "\tfor (const auto& __p : __pm) {" << endl
		<< "\t\tsize_t __k = __p.first;" << endl
		<< "\t\tconst auto& __refs = __p.second.refs;" << endl
		<< "\t\tconst auto __it = __done.find(__k);" << endl
		<< "\t\tif (__it == __done.end()) if (__e(\"unexpected EOF, pointer definition still missing: \" + to_string(__k))) return 0;" << endl
		<< "\t\tvoid* __v = __it->second;" << endl
		<< "\t\t__FILL_REFS;" << endl
		<< "\t}" << endl
		<< "\treturn 1;" << endl // got to the end -> success
		<< "}" << endl << endl;
	dout << "void* " << *st->name <<  "::_deserialize_to_ptr(std::istream& __s, std::function<bool(std::string)> __e, std::unordered_map<size_t, __deserialization_ptr>& __pm) {" << endl;
	int polym = getPolymOf(st->name);
	if (!polym) { // standard pointer, no polymorphism involved
		dout << "\t" << *st->name << "* __v = new " << *st->name << "();" << endl
			<< "\t__v->_deserialize_from(__s, __e, __pm);" << endl
			<< "\treturn __v;" << endl;
	} else {
		// seek the type without eating it
		dout << "\tstreampos __pos = __s.tellg();" << endl
			<< "\tstring __t; __s >> __t;" << endl
			<< "\t__s.seekg(__pos);" << endl
			<< "\tconst auto __it = __polym_map_" << polym << ".find(__t);" << endl
			<< "\tif (__it == __polym_map_" << polym << ".end()) if (__e(\"unknown children type of '"
			// returning nullptr indicates a failure and stops execution
			<< *st->name << "': '\" + __t + \"'\")) return ((void*)nullptr);" << endl
			<< "\treturn __it->second(__s, __e, __pm);" << endl;
	}
	dout << "}" << endl << endl;
}

void openStream(ofstream& stream, const char* arg) {
	stream.open(arg);
	if (!stream.good()) throw runtime_error("can't open output for writing: "s + arg);
}

int main(int argc, char **argv) {
	if (argc != 4) throw runtime_error("expected 3 parameters files: input, header, data");
	yyin = fopen(argv[1], "r");
	if (!yyin) throw runtime_error("can't open input for reading: "s + argv[1]);
	openStream(hout, argv[2]);
	openStream(dout, argv[3]);

	// initialize types map
	init_types();

	hout << "#pragma once" << endl
		<< "#include <functional>" << endl // for callbacks
		<< "#include <unordered_map>" << endl // for state
		<< "#include <iosfwd>" << endl // for (de)serialization input/output
		<< "struct __deserialization_ptr;" << endl;
	dout << "#include \"" << argv[2] << "\"" << endl
		<< "#include <ostream>" << endl
		<< "#include <istream>" << endl
		<< "#include <functional>" << endl
		<< "#include <vector>" << endl
		<< "#include <unordered_set>" << endl
		<< "using namespace std;" << endl << endl
		<< "#define __TYPE_CHK(exp) do { \\" << endl
		<< "\t\tstring __tname; __s >> __tname; \\" << endl
		<< "\t\tif (__tname != exp && __e(__AS_CTX + \": expected type '\"s + exp + \"', got '\" + __tname + \"'\")) return 0; \\" << endl
		<< "\t} while (false)" << endl << endl
		<< "#define __FILL_REFS do { \\" << endl
		<< "\t\tfor (void* __r : __refs) \\" << endl
		<< "\t\t\t* (void**) __r = __v; \\" << endl // they are guaranteed to be pointers to pointers
		<< "\t} while (false);" << endl << endl
		<< "struct __deserialization_ptr {" << endl
		<< "\tvector<void*> refs;" << endl
		<< "\tfunction<void*()> fun;" << endl
		<< "};" << endl << endl;

	yyparse();

	fclose(yyin);
	hout.close();
	dout.close();
	return 0;
}
