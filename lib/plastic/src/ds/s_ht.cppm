module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <utility>
#include <iterator>
#include <stdexcept>
#include <type_traits>
#include <array>
#include <functional>
#include <optional>
#include <string_view>
#include <compare>
#include <concepts>
#endif // !defined(__cpp_lib_modules)
export module plastic.static_hash_table;
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)

namespace plastic::ds {

	export template <class Key> struct ConstexprHasher;

	export template <std::integral Key> struct ConstexprHasher<Key> {
		constexpr std::size_t operator()(Key key) const noexcept {
			return static_cast<std::size_t>(key);
		}
	};

	export template <std::constructible_from<std::string_view> Key> struct ConstexprHasher<Key> {
		constexpr std::size_t operator()(Key const& key) const noexcept {
			std::size_t hash = 0xcbf29ce484222325; // FNV-1a 64-bit offset basis
			for (unsigned char c : key) {
				hash ^= c;
				hash *= 0x100000001b3; // FNV-1a 64-bit prime
			}
			return hash;
		}
	};

	export template <
		class Key, class Value, std::size_t TableSize,
		class Hash = ConstexprHasher<Key>,
		class Equal = std::equal_to<Key>
	> class StaticHashTable {
	public:
		// public type aliases
		using key_type = Key;
		using mapped_type = Value;
		using value_type = std::pair<Key const, Value>;
		using size_type = std::size_t;
		using difference_type = std::ptrdiff_t;
		using hasher = Hash;
		using key_equal = Equal;
		using reference = value_type&;
		using const_reference = value_type const&;

	private:
		enum class SlotState : std::uint8_t { EMPTY, OCCUPIED, TOMBSTONE };

		struct Slot {
			SlotState state = SlotState::EMPTY;
			std::optional<value_type> data;   // valid only when state == OCCUPIED
		};

		std::array<Slot, TableSize> m_slots{};
		size_type m_size = 0;
		[[no_unique_address]] Hash m_hasher;
		[[no_unique_address]] Equal m_equal;

	public:
		// iterator definition
		template <bool IsConst>
		class BasicIterator {
			friend class StaticHashTable;
		private:
			using SlotPtr = std::conditional_t<IsConst, Slot const*, Slot*>;
			SlotPtr m_ptr = nullptr;
			Slot const* m_end = nullptr;

			constexpr BasicIterator(SlotPtr ptr, Slot const* end) noexcept
				: m_ptr(ptr), m_end(end) {}

			constexpr void AdvanceToNextOccupied() noexcept {
				if (!m_ptr) return;
				++m_ptr;
				while (m_ptr != m_end && m_ptr->state != SlotState::OCCUPIED)
					++m_ptr;
				if (m_ptr == m_end) m_ptr = nullptr;
			}

		public:
			using iterator_category = std::forward_iterator_tag;
			using value_type = value_type;
			using difference_type = std::ptrdiff_t;
			using pointer = std::conditional_t<IsConst, value_type const*, value_type*>;
			using reference = std::conditional_t<IsConst, value_type const&, value_type&>;

			constexpr BasicIterator() = default;
			constexpr BasicIterator(BasicIterator const&) = default;
			constexpr BasicIterator& operator=(BasicIterator const&) = default;

			template <bool OtherConst, class = std::enable_if_t<IsConst && !OtherConst>>
			constexpr BasicIterator(BasicIterator<OtherConst> const& other) noexcept
				: m_ptr(other.m_ptr), m_end(other.m_end) {}

			constexpr reference operator*() const noexcept {
				return *m_ptr->data;
			}
			constexpr pointer operator->() const noexcept {
				return &(*m_ptr->data);
			}

			constexpr BasicIterator& operator++() noexcept {
				AdvanceToNextOccupied();
				return *this;
			}
			constexpr BasicIterator operator++(int) noexcept {
				BasicIterator tmp = *this;
				++(*this);
				return tmp;
			}

			friend constexpr std::strong_ordering operator<=>(BasicIterator const& a, BasicIterator const& b) noexcept = default;

		};

		using iterator = BasicIterator<false>;
		using const_iterator = BasicIterator<true>;

	private:
		// internal helper functions (PascalCase)
		constexpr size_type NextProbe(size_type index) const noexcept {
			return (index + 1) % TableSize;
		}

		constexpr size_type FindIndex(Key const& key) const noexcept {
			size_type h = m_hasher(key) % TableSize;
			size_type i = h;
			while (m_slots[i].state != SlotState::EMPTY) {
				if (m_slots[i].state == SlotState::OCCUPIED && m_equal(m_slots[i].data->first, key))
					return i;
				i = NextProbe(i);
				if (i == h) break;
			}
			return TableSize;
		}

		template <class K, class V>
		constexpr std::pair<iterator, bool> InsertImpl(K&& key, V&& value) {
			size_type h = m_hasher(key) % TableSize;
			size_type i = h;
			size_type first_tombstone = TableSize;

			while (m_slots[i].state != SlotState::EMPTY) {
				if (m_slots[i].state == SlotState::OCCUPIED && m_equal(m_slots[i].data->first, key)) {
					// key already exists: update value
					m_slots[i].data->second = std::forward<V>(value);
					return { iterator(&m_slots[i], m_slots.data() + TableSize), false };
				}
				if (m_slots[i].state == SlotState::TOMBSTONE && first_tombstone == TableSize) {
					first_tombstone = i;
				}
				i = NextProbe(i);
				if (i == h) {
					if (first_tombstone != TableSize) {
						Slot& target = m_slots[first_tombstone];
						target.state = SlotState::OCCUPIED;
						target.data.emplace(std::forward<K>(key), std::forward<V>(value));
						++m_size;
						return { iterator(&target, m_slots.data() + TableSize), true };
					}
					return { end(), false }; // table full
				}
			}
			size_type const targetIdx = (first_tombstone != TableSize) ? first_tombstone : i;
			Slot& target = m_slots[targetIdx];
			target.state = SlotState::OCCUPIED;
			target.data.emplace(std::forward<K>(key), std::forward<V>(value));
			++m_size;
			return { iterator(&target, m_slots.data() + TableSize), true };
		}

	public:
		// constructors
		constexpr StaticHashTable() = default;
		constexpr StaticHashTable(Hash const& hf, Equal const& eqf)
			: m_hasher(hf), m_equal(eqf) {}

		// capacity
		constexpr size_type size() const noexcept { return m_size; }
		constexpr bool empty() const noexcept { return m_size == 0; }
		static constexpr size_type max_size() noexcept { return TableSize; }
		static constexpr size_type capacity() noexcept { return TableSize; }

		// iterators
		constexpr iterator begin() noexcept {
			for (size_type i = 0; i < TableSize; ++i) {
				if (m_slots[i].state == SlotState::OCCUPIED)
					return iterator(&m_slots[i], m_slots.data() + TableSize);
			}
			return end();
		}
		constexpr const_iterator begin() const noexcept {
			for (size_type i = 0; i < TableSize; ++i) {
				if (m_slots[i].state == SlotState::OCCUPIED)
					return const_iterator(&m_slots[i], m_slots.data() + TableSize);
			}
			return end();
		}
		constexpr const_iterator cbegin() const noexcept { return begin(); }

		constexpr iterator end() noexcept {
			return iterator(nullptr, m_slots.data() + TableSize);
		}
		constexpr const_iterator end() const noexcept {
			return const_iterator(nullptr, m_slots.data() + TableSize);
		}
		constexpr const_iterator cend() const noexcept { return end(); }

		// lookup
		constexpr iterator find(key_type const& key) noexcept {
			size_type const idx = FindIndex(key);
			if (idx == TableSize) return end();
			return iterator(&m_slots[idx], m_slots.data() + TableSize);
		}
		constexpr const_iterator find(key_type const& key) const noexcept {
			size_type const idx = FindIndex(key);
			if (idx == TableSize) return end();
			return const_iterator(&m_slots[idx], m_slots.data() + TableSize);
		}

		constexpr size_type count(key_type const& key) const noexcept {
			return find(key) != end() ? 1 : 0;
		}
		constexpr bool contains(key_type const& key) const noexcept {
			return find(key) != end();
		}

		// insert / emplace
		constexpr std::pair<iterator, bool> insert(value_type const& value) {
			return InsertImpl(value.first, value.second);
		}
		constexpr std::pair<iterator, bool> insert(value_type&& value) {
			return InsertImpl(std::move(value.first), std::move(value.second));
		}
		template <class P>
		constexpr std::pair<iterator, bool> insert(P&& value) {
			return InsertImpl(std::forward<P>(value).first,
				std::forward<P>(value).second);
		}

		template <class... Args>
		constexpr std::pair<iterator, bool> emplace(Args&&... args) {
			value_type val(std::forward<Args>(args)...);
			return insert(std::move(val));
		}

		template <class... Args>
		constexpr std::pair<iterator, bool> try_emplace(key_type const& k, Args&&... args) {
			return InsertImpl(k, mapped_type(std::forward<Args>(args)...));
		}
		template <class... Args>
		constexpr std::pair<iterator, bool> try_emplace(key_type&& k, Args&&... args) {
			return InsertImpl(std::move(k), mapped_type(std::forward<Args>(args)...));
		}

		// element access
		constexpr mapped_type& operator[](key_type const& key) {
			auto it = find(key);
			if (it != end()) {
				return it->second;
			}
			auto result = InsertImpl(key, mapped_type{});
			return result.first->second;
		}
		constexpr mapped_type& operator[](key_type&& key) {
			auto it = find(key);
			if (it != end()) {
				return it->second;
			}
			auto result = InsertImpl(std::move(key), mapped_type{});
			return result.first->second;
		}

		template <typename Self>
		constexpr auto& at(this Self&& self, key_type const& key) {
			if consteval {
				auto it = self.find(key);
				static_assert(it != self.end(), "StaticHashTable::at: key not found in constexpr context");
				return it->second;
			}
			else {
				auto it = self.find(key);
				if (it == self.end()) {
					throw std::out_of_range("StaticHashTable::at: key not found");
				}
				return it->second;
			}
		}

		// erase
		constexpr size_type erase(key_type const& key) noexcept {
			size_type const idx = FindIndex(key);
			if (idx == TableSize) {
				return 0;
			}
			Slot& slot = m_slots[idx];
			slot.state = SlotState::TOMBSTONE;
			slot.data.reset();
			--m_size;
			return 1;
		}

		constexpr iterator erase(iterator pos) noexcept {
			if (pos == end()) {
				return end();
			}
			Slot* slot = const_cast<Slot*>(pos.m_ptr);
			slot->state = SlotState::TOMBSTONE;
			slot->data.reset();
			--m_size;
			iterator next = pos;
			++next;
			return next;
		}

		constexpr iterator erase(const_iterator pos) noexcept {
			return erase(iterator(const_cast<Slot*>(pos.m_ptr), const_cast<Slot*>(pos.m_end)));
		}

		// clear
		constexpr void clear() noexcept {
			for (auto& slot : m_slots) {
				if (slot.state == SlotState::OCCUPIED) {
					slot.data.reset();
				}
				slot.state = SlotState::EMPTY;
			}
			m_size = 0;
		}

		// hash and equality policy
		constexpr hasher hash_function() const { return m_hasher; }
		constexpr key_equal key_eq() const { return m_equal; }
	};

}
