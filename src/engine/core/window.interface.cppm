export module window:interface;
export import message_bus;
export import disable_copy;
import std;

namespace fyuu_engine::core {

	export template <class DerivedWindow> class BaseWindow
		: util::DisableCopy<BaseWindow<DerivedWindow>> {
	protected:
		concurrency::SyncMessageBus m_message_bus;

	public:
		~BaseWindow() noexcept = default;
		void Show() {
			static_cast<DerivedWindow*>(this)->ShowImpl();
		}

		void Hide() {
			static_cast<DerivedWindow*>(this)->HideImpl();
		}

		void ProcessEvents() {
			static_cast<DerivedWindow*>(this)->ProcessEventsImpl();
		}

		void SetTitle(std::string_view title) {
			static_cast<DerivedWindow*>(this)->SetTitleImpl(title);
		}

		std::pair<std::uint32_t, std::uint32_t> GetWidthAndHeight() const noexcept {
			return static_cast<const DerivedWindow*>(this)->GetWidthAndHeightImpl();
		}

		concurrency::SyncMessageBus& MessageBus() noexcept {
			return m_message_bus;
		}

	};

}