module;

#include <GL/glcorearb.h>

export module graphics:opengl_shader;
import :interface;
import std;

namespace graphics::api::opengl {

	class OpenGLShader : public IShader {
    private:
        std::string m_source;
        ShaderType m_type;
        GLuint m_shader_id;

	public:
        OpenGLShader(std::string_view source, ShaderType type);
        ~OpenGLShader() noexcept override;

        ShaderType GetType() const noexcept override;

        ShaderFormat GetFormat() const noexcept override;

        std::string_view GetSource() const noexcept override;

        std::string_view GetEntryPoint() const noexcept override;

        void Bind(BaseRenderDevice& renderer) const noexcept override;

        void Unbind() const noexcept override;

        void* GetNativeHandle() const noexcept override;
	};

}