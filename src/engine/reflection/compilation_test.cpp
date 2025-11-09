#include <yaml-cpp/yaml.h>

import reflection;
import std;

struct Rect {
	float x;
	float y;
};

struct Point {
	float x;
	float y;
	std::string name;
	Rect* rect;

};

class CustomJSONSerializer : public fyuu_engine::reflection::BaseJSONSerializer<CustomJSONSerializer> {
	friend class fyuu_engine::reflection::BaseJSONSerializer<CustomJSONSerializer>;
private:
	void SerializeImpl(std::string const& field_name, Rect const& field_value) {
		auto& config = Config();
		config[field_name].AsNode();
		config[field_name]["x"].Set(field_value.x);
		config[field_name]["y"].Set(field_value.y);
	}

};

int main(int argc, char** argv) {
	Rect rect{ 1.0f, 2.0f };
	Point point{ 1.0f, 2.0f, "test", &rect };
	auto reflective = fyuu_engine::reflection::MakeReflective(
		point,
		fyuu_engine::reflection::FieldDescriptor<"x", &Point::x>(),
		fyuu_engine::reflection::FieldDescriptor<"y", &Point::y>(),
		fyuu_engine::reflection::FieldDescriptor<"name", &Point::name>(),
		fyuu_engine::reflection::FieldDescriptor<"rect", &Point::rect>()
	);

	std::cout << reflective.TypeName() << std::endl;
	reflective.Field<"name">().Set("Hello World");
	reflective.Visit(
		[](auto&& field) {
			std::cout << field.Name() << " : " << field.Get() << std::endl;
		}
	);

	CustomJSONSerializer serializer;
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