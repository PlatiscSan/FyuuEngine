import std;
import application;

int main(int argc, char** argv) {
	core::Application::Instance()->Initialize();
	core::Application::Instance()->Run();
	return 0;
}