#include <yaml-cpp/yaml.h>

import reflection;
import std;

struct Point {
	float x;
	float y;
	std::string name;
};

int main(int argc, char** argv) {

	Point point{ 1.0f, 2.0f, "test" };
	auto reflective = fyuu_engine::reflection::MakeReflective(
		point,
		fyuu_engine::reflection::FieldDescriptor<"x", &Point::x>(),
		fyuu_engine::reflection::FieldDescriptor<"y", &Point::y>(),
		fyuu_engine::reflection::FieldDescriptor<"name", &Point::name>()
	);

	std::cout << reflective.TypeName() << std::endl;
	reflective.Field<"name">().Set("Hello World");
	reflective.Visit(
		[](auto&& field) {
			std::cout << field.Name() << " : " << field.Get() << std::endl;
		}
	);

	fyuu_engine::reflection::JSONSerializer serializer;
	reflective.Serialize(serializer);
	std::cout << serializer.Serialized() << std::endl;

	//fyuu_engine::reflection::YAMLDeserializer deserializer{ serializer.result };
	//reflective.Deserialize(deserializer);
	//reflective.Visit(
	//	[](auto&& field) {
	//		std::cout << field.Name() << " : " << field.Get() << std::endl;
	//	}
	//);

	std::getchar();
	return 0;

}