Namespace alias:

	namespace util = ml::panther::util;

# Any

	#include <boost/any.hpp>

To store value:

	boost::any v(std::string("hello"));

	// it has an empty state that can be checked by
	if (v.empty()) ...

There are two ways to retrieve a value:

	// this may throw boost::bad_any_cast exception
	std::string s1 = boost::any_cast<std::string>(v);
	std::string& s2 = boost::any_cast<std::string&>(v);

	// this returns NULL if casting fails
	std::string* ps = boost::any_cast<std::string*>(&v);

To dispatch on type:

	const boost::any val; ...
	const std::type_info& t = val.type();
	if (t == typeid(int)) {
		...
	} else if (t == typeid(float)) {
		...
	}

It uses **type erasure**.  When any_cast<T>() is used, boost::any checks that
std::type_info of a stored value is equal to typeid(T).

Copy construction, value construction, copy assignment to boost::any all involve 
dynamic memory allocation:

All of the type casts need to get runtime type information, so it cannot be used
with RTTI disabled.

It uses virtual functions heavily.

# Variant

	#include <boost/variant.hpp>

Usage

	typdef boost::variant<int, const char*, std::string>  my_var_t;
	std::vector<my_var_t> values;
	values.push_back(10);
	values.push_back("Hello");
	values.push_back(std::string("hello"));

It has no empty state and its empty() function always returns false. To
represent empty state, use boost::blank:

	typdef boost::variant<boost::blank, int, const char*, std::string>  my_var_t;
	my_var_t var;
	// which() returns an index of a type currently held by variant
	assert(var.which() == 0);
	var = "hello";
	assert(var.which() != 0);

There are two ways to get values:

	// this may throw boost::bad_get
	std::string& s = boost::get<std::string>(values.back());

	// this returns NULL if casting fails
	std::string* ps = boost::get<std::string*>(values.back());

boost::variant holds an array of characters and stores values in the array.
Size of array is determined at compile time using sizeof() and functions to
get alignment.

Values are destroyed and constructed in-place on top of the internal array.
It usuallly do not allocate memory on the heap and do not require RTTI. 
It is extremely fast and widely used by other Boost libraries.

boost.variant implements a visitor pattern to generate code at compile time 
that looks like this:
	
	// some internal member function that takes a visitor
	switch (which()) {
		case 0: return visitor(*reinterpret_cast<int*>(address()));
		case 1: return visitor(*reinterpret_cast<float*>(address()));
		case 2: return visitor(*reinterpret_cast<std::string*>(address()));
	}

this can be used to access data:

	typedef boost::variant<int, float, std::string> val_t;

	struct my_visitor : public boost::static_visitor<double> {
		double operator() (int value) const {
			return value;
		}
		double operator() (float value) const {
			return value;
		}
		double operator() (const std::string& value) const {
			return ...;
		}
	}

	std::vector<val_t> values;
	double sum = 0.0;
	for (... it = values.begin(); it != values.end(); ++it)
		sum += boost::apply_visitor(my_visitor(), *it);

# Optional

Examples:

	#include <boost/optional.hpp>

	boost::optional<char> get() {
		if (...)
			return boost::optional<char>(queue.top());
		return boost::optional<char>();
	}

	boost::optional<std::string> name;
	if (...) 
		name.reset(database.lookup(emp_name));
	if (name)
		print(*name);


	// to bypass expensive default construction
	std::vector< optional<ClassWithExpCtor> > v;

# Array

	#include <boost/array.hpp>

	boost::array<char, 4> a;
	assert(a.size() == 4);

	// for C++11 compilers, it can be inited like this
	boost::array<char, 4> a = {'a', 'b', 'c', 'd'};
	// for C++03:
	boost::array<char, 4> a = {{'a', 'b', 'c', 'd'}};

	assert(a[0] == 'a');

The class has no handwritten constructors and all all of its members are public.
The compiler will think of it as a POD. It provides exactly the same performance
as a normal C array.

# Tuple

	#include <boost/tuple/tuple.hpp>
	// this is required for set
	#include <boost/tuple/tuple_comparison.hpp"

	boost::tuple<int, char, double> t(3, 'a', 4.1);
	int i = boost::get<0>(t);
	char c = boost::get<1>(t);
	double f = boost::get<2>(t);

	// or like this
	boost::tie(i, c, f) = t;

	// use make_tuple
	std::set< boost::tuple<int, char, double> > s;
	s.insert(boost::make_tuple(1, 'a', 1.0));
	s.insert(boost::make_tuple(2, 'b', 2.0));

The main purpose of tuple is to simplify template programming.


# Bind

Use bind with functions:

	int f(int a, int b, int c) { return a + b + c; }
	boost::bind(f, 1, 2, 3)(); // 6
	boost::bind(f, 1, _1, _2)(2, 3); // 6
	boost::bind(f, 1, _1, _1)(2); // 5

Function objects:

	struct F {
		int operator()(int a, int b) { return a + b; }
		bool operator()(long a, long b) { return a == b; }
	}

	F f;
	bind<int>(f, 1, _1)(2); // 3
	bind<bool>(f, 3L, 4L)(); // false

	// when a nested type named result_type, the explicit return type can be omitted
	struct F {
		typedef int result_type; ...
	}
	bind(f, ...);

	// by default, bind makes a copy of the object, to avoid that, use boost::ref
	// or boost::cref

	bind(boost::ref(f)...);

Pointers to members:

	struct X {
		bool f(int a);
	}
	X x;
	boost::bind(&X::f, x, _1)(3); // this makes a copy of x, thus is self-contained
	boost::bind(&X::f, &x, _1)(3);
	boost::bind(&X::f, boost::ref(x), _1)(3);

Function composition:

	// when this is called
	// bind(g, _1)(x) is evaluated first yielding g(x)
	// bind(f, _1)(g(x)) is then evaludated
	// => f(g(x))
	bind(f, bind(g, _1))(x);

Bind overload operators !, ==, !=, <=, &&, || etc. {{!bind(f, ...)}} is equivalent 
to {{bind(logical_not, bind(f, ...))}}. Similarly, {{bind(f, ...) == x }} is 
equivalent to {{bind(is_equal, bind(f, ...), x)}}:

	std::remove_if(first, last, !bind(&X::is_visible, _1));
	std::find_if(first, last, bind(&X::name, _1) == "Peter");
	std::find_if(first, last, bind(&X::name, _1) == "Peter" || bind(&X::name, _1) == "Paul");
	std::sort(first, last, bind(&X::name, _1) < bind(&X::name, _2));


# Emulate C++11 Move

Need to implement move assignment and move constructor

	namespace other {
		// Its default ctor is cheap/fast
		class characteristics {};
	}

	class person_info {
	public:
		bool is_male_;
		std::string name_;
		std::string second_name_;
		other::characteristics characteristic_;
	}

The correct implementation of move assignment is the same as {{swap}} and {{clear}} (if an empty
state is allowed). The correct implementation of move constructor is close to the
default constructor and {{swap}}

	#include <boost/swap.hpp>

	class person_info {

	// put the following macro in the private section
	private:
		BOOST_COPYABLE_AND_MOVABLE(person_info)

	public:
		// for this example, assume the default constructor is cheap and fast
		person_info() {}

		// copy constructor is required
		person_info(const person_info& p)
			: is_male_(p.is_male_),
			  name_(p.name_),
			  second_name_(p.second_name_),
			  characteristic_(p.characteristic_)
		{}

		// create a move constructor like this
		person_info(BOOST_RV_REF(person_info) person) {
			swap(person);
		}

		// copy assignment is required, note the parameter
		person_info& operator=(BOOST_COPY_ASSIGN_REF(person_info) person) {
			if (this != &person) {
				person_info tmp(person);
				swap(tmp);
			}
			return *this;
		}

		// create a move assignment like this
		person_info& operator=(BOOST_RV_REF(person_info) person) {
			if (this != person) {
				swap(person);
				// clear person
				person_info tmp;
				tmp.swap(person);
			}
			return *this;
		}

		void swap(person_info& rhs) {
			std::swap(is_male_, rhs.is_male_);
			name_.swap(rhs.name_);
			second_name_.swap(rhs.second_name_);
			// boost::swap will first search the namespace other 
			// for a {{swap}} function and if not found, use std::swap
			boost::swap(characteristic_, rhs.characteristic_);
		}
	}

Usage:

	person_info john
	...
	person_info john2(boost::move(john));
	assert(john.name_.empty());
	john = boost::move(john2);
	assert(john2.name_.empty());

boost::move will be expanded to C++11 when C++ compiler is used.

