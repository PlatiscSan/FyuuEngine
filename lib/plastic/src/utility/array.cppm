export module plastic.array;
import plastic.arithmetic;
import std;

namespace plastic::utility {

	export class Array;

	export class ArrayElement {
	private:
		using ValueType = std::variant<
			std::monostate,
			Arithmetic,
			std::string,
			std::unique_ptr<Array>
		>;

		ValueType m_value;

		static ValueType ConstructValueFrom(ArrayElement const& other);

	public:
		ArrayElement() = default;

		ArrayElement(ArrayElement const& other);

		ArrayElement(Arithmetic const& value);
		ArrayElement(Arithmetic&& value);

		ArrayElement(std::string_view value);
		ArrayElement(std::string&& value);

		ArrayElement(std::unique_ptr<Array>&& array);
		ArrayElement(Array&& array);

		bool IsArithmetic() const noexcept;
		bool IsString() const noexcept;
		bool IsArray() const noexcept;

		bool HoldsArithmetic() const noexcept;
		bool HoldsString() const noexcept;
		bool HoldsArray() const noexcept;

		Arithmetic& AsArithmetic();
		std::string& AsString();
		Array& AsArray();

		Arithmetic const& AsArithmetic() const;
		std::string const& AsString() const;
		Array const& AsArray() const;

	};

	export class Array {
	private:
		using Container = std::vector<ArrayElement>;
		using iterator = Container::iterator;
		using const_iterator = Container::const_iterator;

		std::vector<ArrayElement> m_data;

	public:
		iterator begin();
		iterator end();

		const_iterator begin() const;
		const_iterator end() const;

		std::size_t size() const;
		bool empty() const;
		void reserve(std::size_t capacity);
		void clear();

		template <class T>
		void push_back(T&& value) {
			m_data.emplace_back(std::forward<T>(value));
		}

		template <class... Args>
		void emplace_back(Args&&... args) {
			m_data.emplace_back(std::forward<Args>(args)...);
		}

		ArrayElement& operator[](std::size_t index);
		ArrayElement const& operator[](std::size_t index) const;

	};

}