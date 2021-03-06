#include <types1.hh>
#include <types2.hh>
#include <types3.hh>
#include <types4.hh>
#include <types4a.hh>
#include <types4b.hh>

#include <iostream>
#include <sstream>

using namespace std;

int mode = 0;

template<typename T>
int test_it(T& v) {
	if (mode == 0) {
		// test both serealization and deserialization, should produce identical outputs
		stringstream ss;
		v.serialize_to(ss);
		cout << "ORIG / " << &v << ": {{{" << endl << endl
			<< ss.str() << endl
			<< endl << "}}}" << endl << endl;
		T w;
		bool ok = w.deserialize_from(ss,
			[&](const string& err) -> bool {
				cerr << "deserialization error: " << err << endl;
				return true; // stop deserialization in case of errors
			}
		);
		if (!ok) return 1;
		stringstream tt;
		w.serialize_to(tt);
		cout << "COPY / " << &w << ": {{{" << endl << endl
			<< tt.str() << endl
			<< endl << "}}}" << endl;
	} else if (mode == 1) {
		// test deserialization, reding from stdin
		T w;
		bool ok = w.deserialize_from(cin,
			[&](const string& err) -> bool {
				cerr << "deserialization error: " << err << endl;
				return true; // stop deserialization in case of errors
			}
		);
		if (!ok) return 1;
		stringstream tt;
		w.serialize_to(tt);
		cout << "COPY / " << &w << ": {{{" << endl << endl
			<< tt.str() << endl
			<< endl << "}}}" << endl;	
	} else if (mode == 2) {
		// test serialization, writing to stdout
		v.serialize_to(cout);
	}
	return 0;
}

// base types, std types, pointers test
int test1() {
	st1 v;
	v.a = ST1_A;
	v.b = (my_int64) 2000;
	v.does_it_work = true;
	v.strings = { "hey", "there" };
	v.matrix = { { 3, 5, 6},
				 { 6, 7, 7, 9 },
				 { 3, 1, -3 } };
	v.doubles = { 3.14, 6.28, 9.42 };
	v.intToFloat = { {0, 1}, {1, 2.72}, {2, 7.39} };
	v.ptr = new int;
	*v.ptr = 1234321;
	v.ptrVec = { new float, new float };
	*(v.ptrVec[0]) = 123.456;
	*(v.ptrVec[1]) = 654.321;
	v.s_array[3] = 444444;
	v.s_matrix[1][1] = 444111;

	return test_it<st1>(v);	
}

// object encapsulation test
int test2() {
	st2 t2;

	st1& v = t2.sub1;
	v.strings = { "hey", "there" };
	v.matrix = { { 3, 5, 6},
				 { 6, 7, 7, 9 },
				 { 3, 1, -3 } };
	v.doubles = { 3.14, 6.28, 9.42 };
	v.intToFloat = { {0, 1}, {1, 2.72}, {2, 7.39} };

	t2.vec1.resize(2);
	t2.vec1[0].strings.push_back("FIRST SUB ELEM");
	t2.vec1[1].b = 123456;
	t2.vec1[0].ptr = new int;
	*t2.vec1[0].ptr = 444;
	t2.vec1[1].ptrVec.push_back(new float);
	*t2.vec1[1].ptrVec[1] = 88.88;

	return test_it<st2>(t2);
}

// recursive pointers test
int test3() {
	st3a* a = new st3a; a->val_a = 100;
	st3b* b = new st3b; b->val_b = 321.5;
	a->ptr_to_b = b;
	b->ptr_to_a = a;

	st3 v;
	v.real = a;

	return test_it<st3>(v);
}

// polymorphism test
int test4() {
	st4 v;
	v.base_ptr_a = new child4a(222, "new_data_a");
	v.base_ptr_b = new child4b(333, { 4, 8, 16, 22.5 });
	v.base_ptr_c = new child4c(444, 71.923);
	
	return test_it<st4>(v);
}

#define _TEST(n) \
	cerr << "--- TEST " << #n << " ---" << endl << endl; \
	return test##n();

int main(int argc, char** argv) {
	if (argc > 1) {
		// input mode
		if (argv[1][0] == 'i') mode = 1;
		// output mode
		if (argv[1][0] == 'o') mode = 2;
	}
	_TEST(1);
}
