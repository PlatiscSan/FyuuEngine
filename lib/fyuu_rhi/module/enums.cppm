/* enums.cppm */
module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <cctype>
#include <cstdint>
#include <type_traits>
#include <bit>
#include <concepts>
#endif // !defined(__cpp_lib_modules)

#define DEFINE_BITWISE_ENUM_OPS(EnumType)								 \
	constexpr EnumType operator|(EnumType lhs, EnumType rhs) noexcept {  \
		using T = std::underlying_type_t<EnumType>;					  \
		return static_cast<EnumType>(static_cast<T>(lhs) | static_cast<T>(rhs)); \
	}																	 \
	constexpr EnumType operator&(EnumType lhs, EnumType rhs) noexcept {  \
		using T = std::underlying_type_t<EnumType>;					  \
		return static_cast<EnumType>(static_cast<T>(lhs) & static_cast<T>(rhs)); \
	}															 \
	constexpr EnumType operator^(EnumType lhs, EnumType rhs) noexcept {  \
		using T = std::underlying_type_t<EnumType>;					  \
		return static_cast<EnumType>(static_cast<T>(lhs) ^ static_cast<T>(rhs)); \
	}																	 \
	constexpr EnumType operator~(EnumType val) noexcept {				 \
		using T = std::underlying_type_t<EnumType>;					  \
		return static_cast<EnumType>(~static_cast<T>(val));			  \
	}																	 \
	constexpr EnumType& operator|=(EnumType& lhs, EnumType rhs) noexcept { \
		return lhs = lhs | rhs;										   \
	}																	 \
	constexpr EnumType& operator&=(EnumType& lhs, EnumType rhs) noexcept { \
		return lhs = lhs & rhs;										   \
	}																	 \
	constexpr EnumType& operator^=(EnumType& lhs, EnumType rhs) noexcept { \
		return lhs = lhs ^ rhs;										   \
	}
	
export module fyuu_rhi:enums;
#if defined(__cpp_lib_modules)
import std;			  // Import the entire standard library if module support is available.
// In non‑module builds, the required headers were already included.
#endif

export namespace fyuu_rhi {

	template <class T>
	struct Flags128 {
		
		using Tag = T;

		std::uint64_t low;   // Low word
		std::uint64_t high;  // High word

		constexpr Flags128() noexcept : low(0), high(0) {}
		constexpr Flags128(std::uint64_t l, std::uint64_t h) noexcept : low(l), high(h) {}
		constexpr Flags128(std::uint64_t l) noexcept : low(l), high(0) {}  // low word only

		// Comparison
		constexpr bool operator==(Flags128 const& other) const noexcept {
			return low == other.low && high == other.high;
		}
		constexpr bool operator!=(Flags128 const& other) const noexcept {
			return !(*this == other);
		}

		// Bitwise operators
		constexpr Flags128 operator|(Flags128 const& other) const noexcept {
			return Flags128(low | other.low, high | other.high);
		}
		constexpr Flags128 operator&(Flags128 const& other) const noexcept {
			return Flags128(low & other.low, high & other.high);
		}
		constexpr Flags128 operator^(Flags128 const& other) const noexcept {
			return Flags128(low ^ other.low, high ^ other.high);
		}
		constexpr Flags128 operator~() const noexcept {
			return Flags128(~low, ~high);
		}

		// Assignment compound operators
		constexpr Flags128& operator|=(Flags128 const& other) noexcept {
			low |= other.low;
			high |= other.high;
			return *this;
		}
		constexpr Flags128& operator&=(Flags128 const& other) noexcept {
			low &= other.low;
			high &= other.high;
			return *this;
		}
		constexpr Flags128& operator^=(Flags128 const& other) noexcept {
			low ^= other.low;
			high ^= other.high;
			return *this;
		}

		// Explicit bool conversion: true if non-zero
		constexpr operator bool() const noexcept {
			return low != 0 || high != 0;
		}
	};

// Macros for bit positions (for readability)
#define LOW_BIT(FLAG128, pos)  (FLAG128(std::uint64_t(1) << (pos), 0))
#define HIGH_BIT(FLAG128, pos) (FLAG128(0, std::uint64_t(1) << (pos)))

	template <class T>
	struct Flags256 {
		using Tag = T;
	
		std::uint64_t w0; // Bits 0-63
		std::uint64_t w1; // Bits 64-127
		std::uint64_t w2; // Bits 128-191
		std::uint64_t w3; // Bits 192-255

		constexpr Flags256() noexcept : w0(0), w1(0), w2(0), w3(0) {}
		constexpr Flags256(std::uint64_t l) noexcept : w0(l), w1(0), w2(0), w3(0) {}
		constexpr Flags256(std::uint64_t l, std::uint64_t h) noexcept : w0(l), w1(h), w2(0), w3(0) {}
		constexpr Flags256(std::uint64_t w0, std::uint64_t w1, std::uint64_t w2, std::uint64_t w3) noexcept
			: w0(w0), w1(w1), w2(w2), w3(w3) {}

		constexpr bool operator==(Flags256 const& other) const noexcept {
			return w0 == other.w0 && w1 == other.w1 && w2 == other.w2 && w3 == other.w3;
		}
		constexpr bool operator!=(Flags256 const& other) const noexcept {
			return !(*this == other);
		}

		constexpr Flags256 operator|(Flags256 const& other) const noexcept {
			return Flags256(w0 | other.w0, w1 | other.w1, w2 | other.w2, w3 | other.w3);
		}
		constexpr Flags256 operator&(Flags256 const& other) const noexcept {
			return Flags256(w0 & other.w0, w1 & other.w1, w2 & other.w2, w3 & other.w3);
		}
		constexpr Flags256 operator^(Flags256 const& other) const noexcept {
			return Flags256(w0 ^ other.w0, w1 ^ other.w1, w2 ^ other.w2, w3 ^ other.w3);
		}
		constexpr Flags256 operator~() const noexcept {
			return Flags256(~w0, ~w1, ~w2, ~w3);
		}

		constexpr Flags256& operator|=(Flags256 const& other) noexcept {
			w0 |= other.w0; w1 |= other.w1; w2 |= other.w2; w3 |= other.w3;
			return *this;
		}
		constexpr Flags256& operator&=(Flags256 const& other) noexcept {
			w0 &= other.w0; w1 &= other.w1; w2 &= other.w2; w3 &= other.w3;
			return *this;
		}
		constexpr Flags256& operator^=(Flags256 const& other) noexcept {
			w0 ^= other.w0; w1 ^= other.w1; w2 ^= other.w2; w3 ^= other.w3;
			return *this;
		}
	
		constexpr operator bool() const noexcept {
			return w0 != 0 || w1 != 0 || w2 != 0 || w3 != 0;
		}
	};

// Macros for bit positions (for readability)
#define WORD0_BIT(FLAG256, pos) (FLAG256(std::uint64_t(1) << (pos), 0, 0, 0))
#define WORD1_BIT(FLAG256, pos) (FLAG256(0, std::uint64_t(1) << (pos), 0, 0))
#define WORD2_BIT(FLAG256, pos) (FLAG256(0, 0, std::uint64_t(1) << (pos), 0))
#define WORD3_BIT(FLAG256, pos) (FLAG256(0, 0, 0, std::uint64_t(1) << (pos)))

	enum class API : std::uint8_t {
		PlatformDefault = 0,
		Vulkan,
		DirectX12,
		Metal,
		WebGPU,
		OpenGL,
	};

	enum class LogSeverity : std::uint8_t {
		Info = 0,
		Warning,
		Error,
		Fatal
	};

	enum class CommandObjectType : std::uint8_t {
		AllCommands,
		Compute,
		Copy,
	};

	enum class QueuePriority : std::uint8_t {
		High,
		Medium,
		Low
	};

	enum class ShaderLanguage : std::uint8_t {
		Unknown,
		HLSL,
		GLSL,
		MSL,	   // Metal Shading Language
		WGSL	   // WebGPU
	};

	enum class ShaderStage : std::uint16_t {
		Unknown,
		Base1 = 1,
		Vertex = Base1,
		Fragment = Base1<< 1,
		Pixel = Fragment,
		Compute = Base1 << 2,
		Geometry = Base1 << 3,
		TessellationControl = Base1 << 4,
		Hull = TessellationControl,
		TessellationEvaluation = Base1 << 5,
		Domain = TessellationEvaluation,
		Amplification = Base1 << 6,
		Mesh = Base1 << 7,
		RayGeneration = Base1 << 8,
		RayIntersection = Base1 << 9,
		RayAnyHit = Base1 << 10,
		RayClosestHit = Base1 << 11,
		RayMiss = Base1 << 12,
		Callable = Base1 << 13,

		RayTracing = RayGeneration  | RayIntersection | RayAnyHit | RayClosestHit | RayMiss | Callable,
		Graphics = Vertex | Fragment | Geometry | TessellationControl | TessellationEvaluation | Amplification | Mesh,
		All = Vertex | Fragment | Compute | Geometry | TessellationControl | TessellationEvaluation | Amplification | Mesh | RayTracing

	};

	DEFINE_BITWISE_ENUM_OPS(ShaderStage)

	enum class HLSLOption : std::uint16_t {
		None = 0,
		Base1 = 1,
		Enable16BitType = Base1
	};

	DEFINE_BITWISE_ENUM_OPS(HLSLOption)

	enum class VideoMemoryType : std::uint8_t {
		DeviceLocal,
		HostVisible,
		DeviceReadback
	};

	enum class ResourceFlags : std::uint64_t {
		Unknown = 0u,
		Base1 = 1u,
	
		UniformTexelBuffer = Base1,
		StorageTexelBuffer = Base1 << 1u,
		UniformBuffer = Base1 << 2u,
		StorageBuffer = Base1 << 3u,
		IndexBuffer = Base1 << 4u,
		VertexBuffer = Base1 << 5u,	
		IndirectBuffer = Base1 << 6u,
	
		Buffer = UniformTexelBuffer | StorageTexelBuffer | UniformBuffer | StorageBuffer | IndexBuffer | VertexBuffer | IndirectBuffer,
	
		Base2 = Base1 << 7u,
	
		SampledTexture = Base2,
		StorageTexture = Base2 << 1u,
		RenderTarget = Base2 << 2u,	
		DepthStencil = Base2 << 3u,
		Input = Base2 << 4u,
	
		Texture = SampledTexture | StorageTexture | RenderTarget | DepthStencil | Input,
	
		Base3 = Base2 << 5u,
	
		Texture1D = Base3,
		Texture2D = Base3 << 1u,
		Texture3D = Base3 << 2u,
		TextureCube = Base3 << 3u,
	
		TextureDimension = Texture1D | Texture2D | Texture3D | TextureCube,
	
		Base4 = Base3 << 4u,
	
		// Existing formats (0–24)
		BGRA8_Unorm = Base4,
		RGBA8_Uint = Base4 << 1u,
		RGBA8_Unorm = Base4 << 2u,
		RGBA16_Unorm = Base4 << 3u,
		RGBA16_Snorm = Base4 << 4u,
		RGBA16_Sfloat = Base4 << 5u,
		RGBA32_Sfloat = Base4 << 6u,
		R8_Uint = Base4 << 7u,
		R16_Float = Base4 << 8u,
		R32_Uint = Base4 << 9u,
		R32_Float = Base4 << 10u,
		BC1_RGB_Unorm = Base4 << 11u,
		BC1_RGB_SRGB = Base4 << 12u,
		BC1_RGBA_Unorm = Base4 << 13u,
		BC1_RGBA_SRGB = Base4 << 14u,
		BC2_Unorm = Base4 << 15u,
		BC2_SRGB = Base4 << 16u,
		BC3_Unorm = Base4 << 17u,
		BC3_SRGB = Base4 << 18u,
		BC4_Unorm = Base4 << 19u,
		BC4_SRGB = Base4 << 20u,
		BC5_Unorm = Base4 << 21u,
		BC5_SRGB = Base4 << 22u,
		D32SFloat = Base4 << 23u,
		D24UnormS8Uint = Base4 << 24u,

		R8_Unorm = Base4 << 25u,
		R8_Snorm = Base4 << 26u,
		RG8_Unorm = Base4 << 27u,
		RG8_Snorm = Base4 << 28u,
		R16_Unorm = Base4 << 29u,
		R16_Snorm = Base4 << 30u,
		RG16_Sfloat = Base4 << 31u,
		R32_Sint = Base4 << 32u,
		RG32_Sfloat = Base4 << 33u,
		RGB10A2_Unorm = Base4 << 34u,
		R11G11B10_UFloat = Base4 << 35u,
		RGBA8_SRGB = Base4 << 36u,
		BGRA8_SRGB = Base4 << 37u,
		RGBA8_Snorm = Base4 << 38u,
		RGBA8_Sint = Base4 << 39u,
		R16_Uint = Base4 << 40u,
		D16_Unorm = Base4 << 41u,
		D32FloatS8Uint = Base4 << 42u,
		BC6H_UF16 = Base4 << 43u,
		BC6H_SF16 = Base4 << 44u,
		BC7_Unorm = Base4 << 45u,
		BC7_SRGB = Base4 << 46u,
	
		AllFormats =  
			BGRA8_Unorm |
			RGBA8_Uint |
			RGBA8_Unorm |
			RGBA16_Unorm |
			RGBA16_Snorm |
			RGBA16_Sfloat |
			RGBA32_Sfloat |
			R8_Uint |
			R16_Float |
			R32_Uint |
			R32_Float |
			BC1_RGB_Unorm |
			BC1_RGB_SRGB |
			BC1_RGBA_Unorm |
			BC1_RGBA_SRGB |
			BC2_Unorm |
			BC2_SRGB |
			BC3_Unorm |
			BC3_SRGB |
			BC4_Unorm |
			BC4_SRGB |
			BC5_Unorm |
			BC5_SRGB |
			D32SFloat |
			D24UnormS8Uint |
			R8_Unorm |
			R8_Snorm |
			RG8_Unorm |
			RG8_Snorm |
			R16_Unorm |
			R16_Snorm |
			RG16_Sfloat |
			R32_Sint |
			RG32_Sfloat |
			RGB10A2_Unorm |
			R11G11B10_UFloat |
			RGBA8_SRGB |
			BGRA8_SRGB |
			RGBA8_Snorm |
			RGBA8_Sint |
			R16_Uint |
			D16_Unorm |
			D32FloatS8Uint |
			BC6H_UF16 |
			BC6H_SF16 |
			BC7_Unorm |
			BC7_SRGB
	};

	DEFINE_BITWISE_ENUM_OPS(ResourceFlags)

	enum class OptimizationLevel : std::uint8_t {
		None,
		O1,		// -O1
		O2,		// -O2
		O3,		// -O3
		Size,	// -Os
	};

	enum class SamplerFlags : std::uint64_t {
		Unknown = 0u,
		Base1 = 1ull,

		// ----- Filter modes (independent mag/min/mip) -----

		MagNearest = Base1,				// bit 0
		MagLinear  = Base1 << 1u,		   // bit 1
		MagMask	= MagNearest | MagLinear,

		MinNearest = Base1 << 2u,		   // bit 2
		MinLinear  = Base1 << 3u,		   // bit 3
		MinMask	= MinNearest | MinLinear,

		MipNearest = Base1 << 4u,		   // bit 4
		MipLinear  = Base1 << 5u,		   // bit 5
		MipMask	= MipNearest | MipLinear,

		FilterModes = MagMask | MinMask | MipMask,   // mask for all filter bits (0-5)

		Base2 = Base1 << 6u,				// bit 6

		// ----- Address modes (uniform across all axes) -----

		Repeat			= Base2,		   // bit 6
		MirroredRepeat	= Base2 << 1u,	 // bit 7
		ClampToEdge	   = Base2 << 2u,	 // bit 8
		ClampToBorder	 = Base2 << 3u,	 // bit 9
		MirrorClampToEdge = Base2 << 4u,	 // bit 10
		AddressModes	  = Repeat | MirroredRepeat | ClampToEdge | ClampToBorder | MirrorClampToEdge,

		Base3 = Base2 << 5u,				// bit 11

		// ----- Compare functions -----

		Never		   = Base3,			 // bit 11
		Less			= Base3 << 1u,	   // bit 12
		Equal		   = Base3 << 2u,	   // bit 13
		LessOrEqual	 = Base3 << 3u,	   // bit 14
		Greater		 = Base3 << 4u,	   // bit 15
		NotEqual		= Base3 << 5u,	   // bit 16
		GreaterOrEqual  = Base3 << 6u,	   // bit 17
		Always		  = Base3 << 7u,	   // bit 18
		CompareFunctions = Never | Less | Equal | LessOrEqual | Greater | NotEqual |
						GreaterOrEqual | Always,

		Base4 = Base3 << 8u,				// bit 19

		// ----- Border colors -----

		FloatTransparentBlack = Base4,		// bit 19
		FloatOpaqueBlack	  = Base4 << 1u,  // bit 20
		FloatOpaqueWhite	  = Base4 << 2u,  // bit 21
		IntTransparentBlack   = Base4 << 3u,  // bit 22
		IntOpaqueBlack		= Base4 << 4u,  // bit 23
		IntOpaqueWhite		= Base4 << 5u,  // bit 24
		BorderColors = FloatTransparentBlack | FloatOpaqueBlack | FloatOpaqueWhite |
					IntTransparentBlack | IntOpaqueBlack | IntOpaqueWhite,

		Base5 = Base4 << 6u,				// bit 25

		// ----- Reduction modes (min/max/average) -----

		Average		= Base5,			  // bit 25
		Min			= Base5 << 1u,		// bit 26
		Max			= Base5 << 2u,		// bit 27
		ReductionModes = Average | Min | Max,

		// ---- Anisotropy levels (mutually exclusive) ----

		Base6 = Base5 << 3u,				// bit 28
		AnisotropyOff  = Base6,			   // bit 30
		Anisotropy2x   = Base6 << 1u,		 // bit 31
		Anisotropy4x   = Base6 << 2u,		 // bit 32
		Anisotropy8x   = Base6 << 3u,		 // bit 33
		Anisotropy16x  = Base6 << 4u,		 // bit 34
		AnisotropyModes = AnisotropyOff | Anisotropy2x | Anisotropy4x | Anisotropy8x | Anisotropy16x,

		// ---- Unnormalized coordinates ----

		Base7 = Base6 << 5u,				  // bit 35
		UnnormalizedCoordinates = Base7,	  // bit 35

		// ---- Optional advanced features (reserved) ----

		Base8 = Base7 << 1u,				  // bit 36
		Subsampled		  = Base8,		   // bit 36
		SubsampledCoherent  = Base8 << 1u,	 // bit 37

		// ----- Full mask including all defined flags -----

		All = FilterModes | AddressModes | CompareFunctions | BorderColors |
			ReductionModes |
			AnisotropyModes | UnnormalizedCoordinates |
			Subsampled | SubsampledCoherent

	};	
	
	DEFINE_BITWISE_ENUM_OPS(SamplerFlags)

	enum class InputRate : std::uint8_t {
		Instance,
		Vertex
	};
	
	enum class BlendFactor : std::uint8_t {
		Zero,
		One,
		SrcColor,
		OneMinusSrcColor,
		DstColor,
		OneMinusDstColor,
		SrcAlpha,
		OneMinusSrcAlpha,
		DstAlpha,
		OneMinusDstAlpha,
		ConstantColor,
		OneMinusConstantColor,
		ConstantAlpha,
		OneMinusConstantAlpha,
		SrcAlphaSaturate,
		Src1Color,
		OneMinusSrc1Color,
		Src1Alpha,
		OneMinusSrc1Alpha
	};

	enum class BlendOp : std::uint8_t {
		Add,
		Subtract,
		ReverseSubtract,
		Min,
		Max
	};

	enum class LogicOp : std::uint8_t {
		Clear,
		And,
		AndReverse,
		Copy,
		AndInverted,
		Noop,
		Xor,
		Or,
		Nor,
		Equiv,
		Invert,
		OrReverse,
		CopyInverted,
		OrInverted,
		Nand,
		Set
	};

    enum class StencilOp : std::uint8_t {
        Keep,
        Zero,
        Replace,
        IncrementAndClamp,
        DecrementAndClamp,
        Invert,
        IncrementAndWrap,
        DecrementAndWrap
    };

    enum class CompareOp : std::uint8_t {
        Never,
        Less,
        Equal,
        LessOrEqual,
        Greater,
        NotEqual,
        GreaterOrEqual,
        Always
    };

	enum class LoadOp : uint8_t {
		Load,
		Clear,
		DontCare,
	};

	enum class StoreOp : uint8_t {
		Store,
		DontCare,
	};

	struct GPUExecutableTag {};
	using GPUExecutableFlags = Flags256<GPUExecutableTag>;

	namespace GPUExecutableFlagBits {

		constexpr std::uint64_t GPUExecutableW0Base1 = 1u; 
		constexpr GPUExecutableFlags Graphics		= WORD0_BIT(GPUExecutableFlags, GPUExecutableW0Base1);
		constexpr GPUExecutableFlags Compute		= WORD0_BIT(GPUExecutableFlags, GPUExecutableW0Base1 + 1u);
		constexpr GPUExecutableFlags Mesh		 	= WORD0_BIT(GPUExecutableFlags, GPUExecutableW0Base1 + 2u);
		constexpr GPUExecutableFlags RayTracing		= WORD0_BIT(GPUExecutableFlags, GPUExecutableW0Base1 + 3u);
		constexpr GPUExecutableFlags AllType		= Graphics | Compute | Mesh | RayTracing;
	
		constexpr std::uint64_t GPUExecutableW0Base2 = GPUExecutableW0Base1 + 4u;
		constexpr GPUExecutableFlags PrimitiveTopologyPointList		= WORD0_BIT(GPUExecutableFlags, GPUExecutableW0Base2);
		constexpr GPUExecutableFlags PrimitiveTopologyLineList		= WORD0_BIT(GPUExecutableFlags, GPUExecutableW0Base2 + 1u);
		constexpr GPUExecutableFlags PrimitiveTopologyLineStrip		= WORD0_BIT(GPUExecutableFlags, GPUExecutableW0Base2 + 2u);
		constexpr GPUExecutableFlags PrimitiveTopologyTriangleList	= WORD0_BIT(GPUExecutableFlags, GPUExecutableW0Base2 + 3u);
		constexpr GPUExecutableFlags PrimitiveTopologyTriangleStrip	= WORD0_BIT(GPUExecutableFlags, GPUExecutableW0Base2 + 4u);
		constexpr GPUExecutableFlags PrimitiveTopologyPatchList		= WORD0_BIT(GPUExecutableFlags, GPUExecutableW0Base2 + 5u);
		constexpr GPUExecutableFlags PrimitiveTopologyAll 			=
			PrimitiveTopologyPointList | PrimitiveTopologyLineList | PrimitiveTopologyLineStrip |
			PrimitiveTopologyTriangleList | PrimitiveTopologyTriangleStrip | PrimitiveTopologyPatchList;
			
		constexpr std::uint64_t GPUExecutableW0Base3	= GPUExecutableW0Base2 + 6u;
		constexpr GPUExecutableFlags FillModeSolid		= WORD0_BIT(GPUExecutableFlags, GPUExecutableW0Base3);
		constexpr GPUExecutableFlags FillModeWireframe	= WORD0_BIT(GPUExecutableFlags, GPUExecutableW0Base3 + 1u);
		constexpr GPUExecutableFlags FillModeAll		= FillModeSolid | FillModeWireframe;
	
		constexpr std::uint64_t GPUExecutableW0Base4 	= GPUExecutableW0Base3 + 2u;
		constexpr GPUExecutableFlags CullModeNone		= WORD0_BIT(GPUExecutableFlags, GPUExecutableW0Base4);
		constexpr GPUExecutableFlags CullModeFront		= WORD0_BIT(GPUExecutableFlags, GPUExecutableW0Base4 + 1u);
		constexpr GPUExecutableFlags CullModeBack		= WORD0_BIT(GPUExecutableFlags, GPUExecutableW0Base4 + 2u);
		constexpr GPUExecutableFlags CullModeAll		= CullModeNone | CullModeFront | CullModeBack;	
		
		constexpr std::uint64_t GPUExecutableW0Base5 		= GPUExecutableW0Base4 + 3u;
		constexpr GPUExecutableFlags FrontCounterClockwise	= WORD0_BIT(GPUExecutableFlags, GPUExecutableW0Base5);
		constexpr GPUExecutableFlags EnableDepthClip		= WORD0_BIT(GPUExecutableFlags, GPUExecutableW0Base5 + 1u);
		constexpr GPUExecutableFlags EnablePrimitiveRestart	= WORD0_BIT(GPUExecutableFlags, GPUExecutableW0Base5 + 2u);

	} // namespace GPUExecutableFlagBits

	template <class Flags, class... Groups>
		requires (std::same_as<std::decay_t<Flags>, std::decay_t<Groups>> && ...)
	constexpr bool HasConflictingFlags(Flags&& flags, Groups&&... groups) noexcept {
		auto popcount_of = [](auto const& val) -> int {
			using T = std::decay_t<decltype(val)>;
			if constexpr (requires { typename T::Tag; }) {
				// Flags128 or Flags256
				if constexpr (requires { val.w0; val.w1; val.w2; val.w3; }) {
					// Flags256
					return static_cast<std::size_t>(
						std::popcount(val.w0) +
						std::popcount(val.w1) +
						std::popcount(val.w2) +
						std::popcount(val.w3)
					);
				} 
				else if constexpr (requires { val.low; val.high; }) {
					// Flags128
					return static_cast<std::size_t>(
						std::popcount(val.low) + std::popcount(val.high)
					);
				}
				else {
					static_assert(false, "Unknown flag type");
				}
			} else {
				// Enum or integer type
				using U = std::make_unsigned_t<std::underlying_type_t<T>>;
				return static_cast<std::size_t>(std::popcount(static_cast<U>(val)));
			}
		};
	
		// Check if any group has more than one bit set in flags.
		// If groups is empty, the fold yields false.
		return ((popcount_of(flags & groups) > 1u) || ...);
	}

	template <class Flags, class... Groups>
    	requires (std::same_as<std::decay_t<Flags>, std::decay_t<Groups>> && ...)
	constexpr bool HasCrossGroupConflicts(Flags&& flags, Groups&&... groups) noexcept {
		static constexpr std::decay_t<Flags> empty_flag = {};
    	return (((flags & groups) != empty_flag) + ... + 0) > 1;
	}

}