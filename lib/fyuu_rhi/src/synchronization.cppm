export module fyuu_rhi:synchronization;
import std;

namespace fyuu_rhi {

	export class IFence {
	public:
		void Wait(this auto&& self) {
			self.WaitImpl();
		}

		void Reset(this auto&& self) {
			self.ResetImpl();
		}

		void Signal(this auto&& self) {
			self.SignalImpl();
		}

	};

}