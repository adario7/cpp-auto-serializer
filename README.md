# cpp-auto-serializer
cpp-auto-serializer is a code generator which automatically adds serialization/deserialization functions to C++ headers. Inputs look very similar to standard C/C++ headers, with little extra syntax. Outputs, for every input, are: a header file identical to the original but with `void serialize_to(std::ostream& output) const;` and  `bool deserialize_from(std::istream& source, std::function<bool(std::string)> error_callback);` added; and a source file with implementation for those methods.

## usage

### usage example

Given an input file which looks like:

```c++
`
#include <string>
#include <vector>
`

class example_data {
public:
	int a = 3, b;
	std::string str = "initial value";
	std::vector<double> vec;

` // parts within backticks will be copied to the destianation header without changes
	int getSum(int j) { return a + b + j; }
	int getDiff(int k);
`
};

```

It can be used in other source files like:

```c++
// serialization
example_data data1 = <...>;
ofstream file_out(<...>);
data1.serialize_to(file_out);
// deserialization
ifstream file_in(<...>);
example_data data2;
bool got_err = data2.deserialize_from(file_int,
	[&](const string& err) -> bool { // error callback
		std::cerr << "deserialization error: " << err << std::endl;
		return true; // true stop deserialization in case of errors
	}
);
```

For more examples, see `test/`.

### why the backticks?

cpp-auto-serializer does not need information about class methods and included files to generate serialization methods. To avoid having a complete C++ parser, such parts shall be enclosed in backticks, and will be translated in the output header as they are, and in the order in which they appear.

### custom types

If your custom types are defined within the header with the `using` keyword, cpp-auto-serializer will pick them up automatically. If the custom types are defined outside of the header (note that the serializer won't look at/expand `#include` directives) you can inform the serializer about them with:

```c++
alias my_custom_int = int64_t;
```

These type definitions won't be copied in the output header (use `using` if you want that!).

For examples, see `test/`.

### polymorphism

Beacuse every file is compiled by the generator independently from the others, you need to manually inform the serializer when a pointer to a class could be of other types at runtime:

```c++

polymorphic Animal :
	Dog,
	Cat include "cat.h";
	
virtual class Animal {};
class Dog {};
// class Cat is defined in cat.hdef

```

The `polymorphic` statement informs the serializer of all possible runtime types of ponters to Animal, and when a the child class is defined in another header what is needed to be `#include`d in order to instantiate it (this is needed, as an `#include` is necessary for using the class in the output source file, bu would create a dependency loop if also added in the output header).

You will also often want to add the `virtual` modifier to the class, which tells the serializer to make the serialization methods virtual for that class.

For more examples, see `test/`.

### code generation

Code can be generated manually with:

```sh
auto-serializer <input.hdef> <output-header.h> <output-source.cpp>
```

Code can be generated automatically with CMake, by having it call cpp-auto-serializer whenever an input header is changed; a possible implementation can be found in `test/`.

## Feauters and limitations

cpp-auto-serializer is not space efficient and as such not ideal for sending data over a network, focusing instead on saving data to disk. Names and types of every field are validated during deserialization, as such save files from different versions are incompatible but also can't be mismatch.

Supported:
- file compatibility when reordering fields (but not partent classes)
- inheritance and multiple inheritance
- polymorphism
- most common `std` containers and native types
- static arrays
- pointers, even with cyclical dependencies between them
- compiling the source headers without reading/depending on the other header files

Not supported:
- file compatibility when adding, removing, renaming, changing the type of fileds
- some `std` containers and native types
- pointers used as arrays (support planned)
- virtual inheritance
- unions
