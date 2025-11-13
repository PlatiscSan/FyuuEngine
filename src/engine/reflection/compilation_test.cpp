#include <yaml-cpp/yaml.h>

import reflective;
import serialization;
import std;

struct Test {
	std::string name;
};

struct Point {
	float x;
	float y;
	std::string name;
	Test* test;
};

struct Rect {
	Point* p1;
	Point* p2;
	std::string name;
};

class CustomJSONSerializer : public fyuu_engine::reflection::BaseJSONSerializer<CustomJSONSerializer> {
public:
	static auto MakeReflective(Point& field_value) {

		return fyuu_engine::reflection::MakeReflective(
			field_value,
			fyuu_engine::reflection::FieldDescriptor<"x", &Point::x>(),
			fyuu_engine::reflection::FieldDescriptor<"y", &Point::y>(),
			fyuu_engine::reflection::FieldDescriptor<"name", &Point::name>(),
			fyuu_engine::reflection::FieldDescriptor<"test", &Point::test>()
		);

	}

	static auto MakeReflective(Test& field_value) {

		return fyuu_engine::reflection::MakeReflective(
			field_value,
			fyuu_engine::reflection::FieldDescriptor<"name", &Test::name>()
		);

	}

};

int main(int argc, char** argv) {
	Test test{ "test" };
	Point p1{ 1.0f, 2.0f, "point1", &test };
	Point p2{ 3.0f, 4.0f, "point2", nullptr };
	Rect rect{ &p1, &p2, "rect" };

	auto reflective = fyuu_engine::reflection::MakeReflective(
		rect,
		fyuu_engine::reflection::FieldDescriptor<"p1", &Rect::p1>(),
		fyuu_engine::reflection::FieldDescriptor<"p2", &Rect::p2>(),
		fyuu_engine::reflection::FieldDescriptor<"name", &Rect::name>()
	);

	std::cout << reflective.TypeName() << std::endl;
	reflective.Field<"name">().Set("Hello World");
	reflective.Visit(
		[](auto&& field) {
			/*std::cout << field.Name() << " : " << field.Get() << std::endl;*/
		}
	);

	CustomJSONSerializer serializer;
	reflective.Serialize(serializer);
	std::cout << serializer.Serialized() << std::endl;

	CustomJSONSerializer deserializer;
	deserializer.Parse(serializer.Serialized());

	Rect rect2{};

	auto reflective2 = fyuu_engine::reflection::MakeReflective(
		rect2,
		fyuu_engine::reflection::FieldDescriptor<"p1", &Rect::p1>(),
		fyuu_engine::reflection::FieldDescriptor<"p2", &Rect::p2>(),
		fyuu_engine::reflection::FieldDescriptor<"name", &Rect::name>()
	);

	reflective2.Deserialize(deserializer);
	//reflective.Visit(
	//	[](auto&& field) {
	//		std::cout << field.Name() << " : " << field.Get() << std::endl;
	//	}
	//);

	std::getchar();
	return 0;

}