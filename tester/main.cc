#include <types1.hh>
#include <types2.hh>
#include <types3.hh>
#include <types4.hh>
#include <iostream>
#include <sstream>

using namespace std;

template<typename T>
int do_121(T& val) {
	stringstream ss;
	val.serialize_to(ss);
	cout << "V / " << &val << ": {{{" << endl << endl
		<< ss.str() << endl
		<< endl << "}}}" << endl << endl;

	T w;
	bool got_err = w.deserialize_from(ss,
		[&](const string& err) -> bool {
			cout << "deserialization error: " << err << endl;
			cout.flush();
			return true; // stop deserialization in case of errors
		}
	);
	if (got_err) return 1;

	stringstream tt;
	w.serialize_to(tt);
	cout << "W / " << &w << ": {{{" << endl << endl
		<< tt.str() << endl
		<< endl << "}}}" << endl;

	return 0;
}

// std types test
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

	return do_121<st1>(v);	
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

	return do_121<st2>(t2);
}

// recursive pointers test
int test3() {
	st3a* a = new st3a; a->val_a = 100;
	st3b* b = new st3b; b->val_b = 321.5;
	a->ptr_to_b = b;
	b->ptr_to_a = a;

	st3 v;
	v.real = a;

	return do_121<st3>(v);
}

// polymorphism
int test4() {
	st4 v;
	v.base_ptr_a = new child4a(222, "new_data_a");
	v.base_ptr_b = new child4b(333, { 4, 8, 16, 22.5 });
	
	return do_121<st4>(v);
}

#define _TEST(n) \
	cout << "--- TEST " << #n << " ---" << endl << endl; \
	return test##n();

int main() {
	_TEST(1);
}
