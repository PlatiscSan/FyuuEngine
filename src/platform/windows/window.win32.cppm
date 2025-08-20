module;
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#ifdef WIN32
#include <tchar.h>
#include <Windows.h>
#endif // WIN32

export module window:win32;
import :interface;
import std;
import concurrent_vector;
import synchronized_function;

namespace platform {
#ifdef WIN32

	extern concurrency::SynchronizedFunction<boost::uuids::uuid()> GenerateUUID;

#ifdef UNICODE
	export std::string TStringToString(std::wstring_view str);
	export std::wstring StringToTString(std::string_view str);
#else
	export inline std::string_view TStringToString(std::string_view str) {
		return str;
	}
	export inline std::string_view StringToTString(std::string_view str) {
		return str;
	}
#endif // UNICODE


	export class Win32Exception : public std::exception {
	private:
		std::vector<TCHAR> m_error_message;
#ifdef UNICODE
		mutable std::string m_multi_bytes_error_message;
#endif // UNICODE

	public:
		explicit Win32Exception(DWORD error_code = ::GetLastError());
		TCHAR const* What() const noexcept;
		char const* what() const noexcept override;

	};

	export class Win32Window : public IWindow {
	public:
		using MsgProcessor = std::function<LRESULT(HWND, UINT, WPARAM, LPARAM)>;
		using ClassName = std::array<TCHAR, 36u>;

	private:
		concurrency::ConcurrentVector<std::pair<boost::uuids::uuid, MsgProcessor>> m_msg_processors;
		util::MessageBus m_message_bus;
		ClassName m_class_name;
		HWND m_hwnd = nullptr;

		static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) noexcept;

		static ClassName GenerateClassName();

		static HWND CreateWindowImp(
			Win32Window* window,
			std::string_view title,
			std::uint32_t width,
			std::uint32_t height
		);

		void OnDestroy() noexcept;
		void OnResize(LPARAM lparam) noexcept;
		void OnKeyDown(WPARAM wparam, LPARAM lparam) noexcept;
		void OnMouseButtonDown(UINT msg, LPARAM lparam) noexcept;
		void OnMouseButtonUp(UINT msg, LPARAM lparam) noexcept;

		LRESULT DefaultHandler(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) noexcept;

	public:
		Win32Window(std::string_view title, std::uint32_t width, std::uint32_t height);

		~Win32Window() noexcept override;

		template <std::convertible_to<MsgProcessor> Compatible>
		boost::uuids::uuid AttachMsgProcessor(Compatible&& processor) {
			auto uuid = GenerateUUID();
			(void)m_msg_processors.emplace_back(uuid, std::forward<Compatible>(processor));
			return uuid;
		}

		template <std::convertible_to<MsgProcessor> Compatible>
		boost::uuids::uuid StrictAttachMsgProcessor(Compatible&& processor) {
			auto uuid = GenerateUUID();
			auto modifier = m_msg_processors.LockedModifier();
			(void)modifier.emplace_back(uuid, std::forward<Compatible>(processor));
			return uuid;
		}

		void DetachBackMsgProcessor();

		void DetachMsgProcessor(boost::uuids::uuid const& uuid);

		void Show() override;

		void Hide() override;

		void ProcessEvents() override;

		void SetTitle(std::string_view title) override;

		std::pair<std::uint32_t, std::uint32_t> GetWidthAndHeight() const noexcept override;

		void* Native() const noexcept override;

		HWND GetHWND() const noexcept;

		util::MessageBus* GetMessageBus() noexcept override;

	};

#endif 
}