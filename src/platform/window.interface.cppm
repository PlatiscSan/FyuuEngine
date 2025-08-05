export module window:interface;
import message_bus;
import std;

export namespace platform {
	class IWindow {
	public:
		virtual ~IWindow() noexcept = default;
		virtual void Show() = 0;
		virtual void Hide() = 0;

		virtual void ProcessEvents() = 0;
		virtual void SetTitle(std::string_view title) = 0;
		virtual std::pair<std::uint32_t, std::uint32_t> GetWidthAndHeight() const noexcept = 0;
		virtual void* Native() const noexcept = 0;

		virtual util::MessageBus* GetMessageBus() noexcept = 0;

	};

}