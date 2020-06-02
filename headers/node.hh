#pragma once

#include <pos_t.hh>
#include <iostream>
#include <vector>

class NBodyElem;
class NVarDeclaration;
class NFunctionArg;
class NType;
class NParent;

using VarDeclList = std::vector<NVarDeclaration*>;
using TypeSuffixList = std::vector<int>;
using ArraySuffixList = std::vector<std::string*>;
using GenericsList = std::vector<const NType*>;
using BodyList = std::vector<NBodyElem*>;
using ParentsList = std::vector<NParent*>;

// optionally deletes (if elem is not null)
#define _OPT_DEL(elem) if (elem) delete elem;
// delete vector elements
#define _DEL_ELEMS(vec) for (auto* v : vec) delete v;
// delete vector elements and the vector itself
#define _DEL_VEC(vec) _DEL_ELEMS(*vec); delete vec;

class Node {
public:
	segment_t pos;
	Node(segment_t p) : pos(p) {}
    virtual ~Node() {}
};

class NBodyElem : public Node {
public:
	NBodyElem(segment_t p) : Node(p) { }
	virtual ~NBodyElem() { }
};

class NRoot : public Node {
public:
	NRoot(segment_t p) : Node(p) { }
	virtual ~NRoot() { }
};

class NIdentifier : public Node {
public:
    std::string value;
    NIdentifier(segment_t p, const std::string&& value)
		: Node(p), value(std::move(value)) { }
	virtual ~NIdentifier() {}
};

class NType : public Node {
public:
	NIdentifier* name;
	GenericsList* generics;
	bool isArray = false; // true when `name` is the size of a static array
	NType(segment_t p, NIdentifier* n, GenericsList* g)
		: Node(p), name(n), generics(g) {}
	virtual ~NType() { delete name; delete generics; }

	NType* deepCopy() const {
		GenericsList* l = new GenericsList();
		for (const NType* child : *generics)
			l->push_back(child->deepCopy());
		std::string namecpy = name->value;
		NIdentifier* idt = new NIdentifier(name->pos, std::move(namecpy));
		return new NType(pos, idt, l);
	}

	static NType* pointerTo(NType* obj, segment_t pos) {
		GenericsList* l = new GenericsList();
		l->push_back(obj);
		return new NType(pos, new NIdentifier(pos, "*"), l);
	}

	static NType* arrayOf(NType* obj, std::string* size, segment_t pos) {
		GenericsList* l = new GenericsList();
		l->push_back(obj);
		std::string size_cp = *size;
		NType* r = new NType(pos, new NIdentifier(pos, std::move(size_cp)), l);
		r->isArray = true; // the identifier is the size, not the name
		return r;
	}
};

// converts to the internal name format
std::string to_string(const NType& t);
// converts to the C++ name format
std::string to_cpp_type(const NType& t);
// converts to the internal name format
std::ostream& operator<<(std::ostream& o, const NType& t);
bool operator==(const NType& a, const NType& b);
namespace std {
template<>
class hash<NType> {
public:
    size_t operator()(const NType& s) const;
};
}

class NVarDeclaration : public Node {
public:
	TypeSuffixList* tSuffixes;
    NIdentifier* name;
	ArraySuffixList* arraySuffixes;
	std::string* assignment;
	NType* completeType;
    NVarDeclaration(segment_t p, TypeSuffixList* s, NIdentifier* n, ArraySuffixList* as, std::string* a)
        : Node(p), tSuffixes(s), arraySuffixes(as), name(n), assignment(a) {}
	virtual ~NVarDeclaration() { delete tSuffixes; delete name; _OPT_DEL(assignment); delete completeType; }
};

class NVarBlock : public NBodyElem {
public:
	NType* type;
	VarDeclList* vars;

	NVarBlock(segment_t p, NType* type, VarDeclList* vars)
		: NBodyElem(p), type(type), vars(vars) {
		// complete type: int* --> *<int> and int* a, b --> *<int> a; int b
		for (NVarDeclaration* d : *vars) {
			NType* ct = type->deepCopy(); 
			for (int i = 0; i < d->tSuffixes->size(); i++)
				ct = NType::pointerTo(ct, d->pos);
			// loop backwards otherwise dimesntions will be inverted
			for (int i = d->arraySuffixes->size() - 1; i >= 0; i--) {
				std::string* size = (*d->arraySuffixes)[i];
				ct = NType::arrayOf(ct, size, d->pos);
			}
			d->completeType = ct;
		}
	}
	virtual ~NVarBlock() { delete type; _DEL_VEC(vars); }
};

class NCode : public NRoot, public NBodyElem {
public:
	std::string* code;
	NCode(segment_t p, std::string* c)
		: NRoot(p), NBodyElem(p), code(c) { }
	virtual ~NCode() { delete code; };
};

class NParent : public Node {
public:
	std::string *code_prev, *code_next;
	NType* type;
	NParent(segment_t s, std::string* p, NType* t, std::string* n)
		: Node(s), code_prev(p), type(t), code_next(n) {}
	virtual ~NParent() { delete code_prev; delete type; delete code_next; }
};

class NStruct : public NRoot {
public:
	bool isVirtual, isClass;
	NType* name;
	ParentsList* parents;
	BodyList* body;
	NStruct(segment_t p, bool iv, bool ic, NType* n, ParentsList* e, BodyList* b)
		: NRoot(p), isVirtual(iv), isClass(ic), name(n), parents(e), body(b) {}
	virtual ~NStruct() { delete name; _DEL_VEC(parents); _DEL_VEC(body); }
};

class NPolym : public NRoot {
public:
	NType* subject;
	GenericsList* children;
	NPolym(segment_t p, NType* s, GenericsList* c)
		: NRoot(p), subject(s), children(c) {}
	virtual ~NPolym() { delete subject; _DEL_VEC(children); }
};

class NAlias : public NRoot {
public:
	bool in_code; // wheter it is only useful for the serializer if it goes in the real header
	NType *name, *real;
	NAlias(segment_t p, bool i, NType* n, NType* r)
		: NRoot(p), in_code(i), name(n), real(r) {}
	virtual ~NAlias() { delete name; delete real; }
};
