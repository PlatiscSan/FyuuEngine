import std;
import lock_free_hash_table;

using namespace fyuu_engine::concurrency;

int main() {
	test::lock_free_hash_table::RunLockFreeTableTests();
	test::lock_free_hash_table::RunCompleteBenchmarkSuite();
	std::getchar();
	return 0;
}
