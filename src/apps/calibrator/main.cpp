#include <sgct/actions.h>
#include <sgct/clustermanager.h>
#include <sgct/commandline.h>
#include <sgct/engine.h>
#include <sgct/image.h>
#include <sgct/keys.h>
#include <sgct/logger.h>
#include <sgct/settings.h>
#include <sgct/shadermanager.h>
#include <sgct/shareddata.h>
#include <sgct/shareddatatypes.h>
#include <sgct/texturemanager.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <numeric>

#ifdef SGCT_HAS_TEXT
#include <sgct/font.h>
#include <sgct/fontmanager.h>
#include <sgct/freetype.h>
#endif // SGCT_HAS_TEXT

namespace {
    struct {
        GLuint vao = 0;
        GLuint vbo = 0;
        GLuint iboLine = 0;
        int nVertLine = 0;
        GLuint iboTriangle = 0;
        int nVertTriangle = 0;
    } geometry;

    sgct::SharedBool showId(false);
    sgct::SharedBool showStats(false);
    sgct::SharedBool takeScreenshot(false);
    sgct::SharedBool captureBackbuffer(false);

    float tilt = 0.f;
    float radius = 7.4f;
    std::string texture;
    unsigned int textureId = 0;

    struct Vertex {
        float elevation, azimuth;
    };
    GLint matrixLocation = -1;

    constexpr const char* vertexShader = R"(
#version 330 core

layout(location = 0) in vec2 vertPosition;

out vec4 color;
out vec2 texCoord;

uniform float radius;
uniform mat4 matrix;

const float PI = 3.1415926;
const float PI_HALF = PI / 2.0;

void main() {
  float elevation = vertPosition[0];
  float azimuth = vertPosition[1];

  vec3 p = vec3(
    radius * cos(elevation) * sin(azimuth),
    radius * sin(elevation),
    -radius * cos(elevation) * cos(azimuth)
  );
  gl_Position = matrix * vec4(p, 1.0);
  color = vec4(p, 1.0);

  float e = 1.0 - (elevation / PI_HALF);
  texCoord = vec2(e * sin(azimuth), e * -cos(azimuth));
  texCoord = (texCoord * vec2(0.5)) + 0.5;
}
)";

    constexpr const char* fragmentShader = R"(
#version 330 core

in vec4 color;
in vec2 texCoord;
out vec4 FragOut;

uniform int hasTex;
uniform sampler2D tex;

void main() {
  if (hasTex == 1) {
    FragOut = texture(tex, texCoord);
  }
  else {
    FragOut = color;
  }
}
)";
} // namespace

using namespace sgct;

void draw(RenderData) {
    ShaderManager::instance().getShaderProgram("simple").bind();
    const glm::mat4 mvp = Engine::instance().getCurrentModelViewProjectionMatrix();

    // Inverting tilt to keep the backwards compatibility with the previous implementation
    const glm::mat4 mat = glm::rotate(mvp, -glm::radians(tilt), glm::vec3(1.f, 0.f, 0.f));

    glUniformMatrix4fv(matrixLocation, 1, GL_FALSE, glm::value_ptr(mat));

    glBindVertexArray(geometry.vao);
    if (textureId != 0) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureId);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geometry.iboTriangle);
        glDrawElements(GL_TRIANGLES, geometry.nVertTriangle, GL_UNSIGNED_SHORT, nullptr);
    }
    else {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geometry.iboLine);
        glDrawElements(GL_LINE_STRIP, geometry.nVertLine, GL_UNSIGNED_SHORT, nullptr);
    }
    glBindVertexArray(0);

    ShaderManager::instance().getShaderProgram("simple").unbind();
}

void draw2D() {
#ifdef SGCT_HAS_TEXT
    if (showId.getVal()) {
        Window& win = Engine::instance().getCurrentWindow();
        core::BaseViewport* vp = win.getCurrentViewport();
        const float w = static_cast<float>(win.getResolution().x) * vp->getSize().x;
        const float h = static_cast<float>(win.getResolution().y) * vp->getSize().y;

        const float offset = w / 2.f - w / 7.f;

        const float s1 = h / 8.f;
        const unsigned int fontSize1 = static_cast<unsigned int>(s1);
        text::Font* f1 = text::FontManager::instance().getFont("SGCTFont", fontSize1);

        text::print(
            *f1,
            sgct::text::TextAlignMode::TopLeft,
            offset,
            h / 2.f - s1,
            glm::vec4(0.f, 0.f, 1.f, 1.f),
            "%d",
            sgct::core::ClusterManager::instance().getThisNodeId()
        );

        const float s2 = h / 20.f;
        const unsigned int fontSize2 = static_cast<unsigned int>(s2);
        text::Font* f2 = text::FontManager::instance().getFont("SGCTFont", fontSize2);
        text::print(
            *f2,
            text::TextAlignMode::TopLeft,
            offset,
            h / 2.f - (s1 + s2) * 1.2f,
            glm::vec4(0.f, 0.f, 1.f, 1.f),
            "%s",
            core::ClusterManager::instance().getThisNode().getAddress().c_str()
        );
    }
#endif // SGCT_HAS_TEXT
}

void initGL() {
    constexpr const uint16_t RestartIndex = std::numeric_limits<uint16_t>::max();
    glEnable(GL_PRIMITIVE_RESTART);
    glPrimitiveRestartIndex(RestartIndex);

    glGenVertexArrays(1, &geometry.vao);
    glGenBuffers(1, &geometry.vbo);
    glGenBuffers(1, &geometry.iboLine);
    glGenBuffers(1, &geometry.iboTriangle);

    glBindVertexArray(geometry.vao);
    glBindBuffer(GL_ARRAY_BUFFER, geometry.vbo);

    constexpr const int ElevationSteps = 40;
    constexpr const int AzimuthSteps = 160;

    std::vector<Vertex> vertices;
    // (abock, 2019-10-09) We generate the vertices ring-wise;  first iterating over the
    // elevation and then the azimuth will lead to the bottom most ring be filled first,
    // before going to the upper rings.  That also means that two vertices on top of each
    // other should be separated in the vertices list by 'AzimuthSteps' positions
    for (int e = 0; e <= ElevationSteps; ++e) {
        for (int a = 0; a < AzimuthSteps; ++a) {
            Vertex vertex;
            float ev = static_cast<float>(e) / static_cast<float>(ElevationSteps - 1);
            float av = static_cast<float>(a) / static_cast<float>(AzimuthSteps - 1);
            vertex.elevation = glm::radians(ev * 90.f);
            vertex.azimuth = glm::radians(av * 360.f);
            vertices.push_back(vertex);
        }
    }

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);
    glBufferData(
        GL_ARRAY_BUFFER,
        vertices.size() * sizeof(Vertex),
        vertices.data(),
        GL_STATIC_DRAW
    );

    {
        // Line representation
        // (abock, 2019-10-09) It's possible to compute a better size of how many indices
        // we'll need, but I can't be asked (and we are seriously not performance limited)
        std::vector<uint16_t> indices;
        indices.reserve(2 * ElevationSteps * AzimuthSteps);

        // First the horizontal, azimuth lines
        for (int e = 0; e < ElevationSteps; ++e ) {
            std::vector<uint16_t> t(AzimuthSteps);
            std::iota(t.begin(), t.end(), static_cast<uint16_t>(e * AzimuthSteps));
            t.push_back(static_cast<uint16_t>(e * AzimuthSteps)); // close the ring
            t.push_back(RestartIndex); // restart for the next ring
            indices.insert(indices.end(), t.begin(), t.end());
        }
        indices.push_back(RestartIndex);
        // Then the vertical lines; see above; every vertical vertex is separated by
        // exactly 'AzimuthSteps' positions in the vertex array
        for (int a = 0; a < AzimuthSteps; ++a) {
            for (int e = 0; e < ElevationSteps; ++e) {
                indices.push_back(static_cast<uint16_t>(a + e * AzimuthSteps));
            }
            indices.push_back(RestartIndex);
        }

        geometry.nVertLine = static_cast<int>(indices.size());
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geometry.iboLine);
        glBufferData(
            GL_ELEMENT_ARRAY_BUFFER,
            indices.size() * sizeof(uint16_t),
            indices.data(),
            GL_STATIC_DRAW
        );
    }
    {
        // Triangle representation
        std::vector<uint16_t> indices;
        for (int e = 0; e < ElevationSteps; ++e) {
            for (int a = 0; a < AzimuthSteps; ++a) {
                const uint16_t base = static_cast<uint16_t>(e * AzimuthSteps + a);
                // first triangle
                indices.push_back(base);
                indices.push_back(base + 1);
                indices.push_back(base + AzimuthSteps);

                //// second triangle
                indices.push_back(base + 1);
                indices.push_back(base + AzimuthSteps + 1);
                indices.push_back(base + AzimuthSteps);
            }
        }
        geometry.nVertTriangle = static_cast<int>(indices.size());
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geometry.iboTriangle);
        glBufferData(
            GL_ELEMENT_ARRAY_BUFFER,
            indices.size() * sizeof(uint16_t),
            indices.data(),
            GL_STATIC_DRAW
        );
    }
    glBindVertexArray(0);

    if (!texture.empty()) {
        textureId = TextureManager::instance().loadTexture(texture, true, 0);
    }

    ShaderManager::instance().addShaderProgram("simple", vertexShader, fragmentShader);
    const ShaderProgram& prog = ShaderManager::instance().getShaderProgram("simple");
    prog.bind();
    matrixLocation = glGetUniformLocation(prog.getId(), "matrix");

    glUniform1f(glGetUniformLocation(prog.getId(), "radius"), radius);
    glUniform1i(glGetUniformLocation(prog.getId(), "tex"), 0);
    glUniform1i(glGetUniformLocation(prog.getId(), "hasTex"), (textureId != 0) ? 1 : 0);

    prog.unbind();
}

void postSyncPreDraw() {
    Settings::instance().setCaptureFromBackBuffer(captureBackbuffer.getVal());
    Engine::instance().setStatsGraphVisibility(showStats.getVal());
    if (takeScreenshot.getVal()) {
        Engine::instance().takeScreenshot();
        takeScreenshot.setVal(false);
    }
}

void keyboardCallback(Key key, Modifier, Action action, int) {
    if (key == Key::Esc && action == Action::Press) {
        Engine::instance().terminate();
    }
    if (key == Key::I && action == Action::Press) {
        showId.setVal(!showId.getVal());
    }

    if (key == Key::S && action == Action::Press) {
        showStats.setVal(!showStats.getVal());
    }

    if (key == Key::P && action == Action::Press) {
        takeScreenshot.setVal(true);
    }

    if (key == Key::B && action == Action::Press) {
        captureBackbuffer.setVal(!captureBackbuffer.getVal());
    }
}

void encode() {
    SharedData::instance().writeBool(showId);
    SharedData::instance().writeBool(showStats);
    SharedData::instance().writeBool(takeScreenshot);
    SharedData::instance().writeBool(captureBackbuffer);
}

void decode() {
    SharedData::instance().readBool(showId);
    SharedData::instance().readBool(showStats);
    SharedData::instance().readBool(takeScreenshot);
    SharedData::instance().readBool(captureBackbuffer);
}

void cleanUp() {
    glDeleteVertexArrays(1, &geometry.vao);
    glDeleteBuffers(1, &geometry.vbo);
    glDeleteBuffers(1, &geometry.iboLine);
    glDeleteBuffers(1, &geometry.iboTriangle);
}

int main(int argc, char* argv[]) {
    std::vector<std::string> arg(argv + 1, argv + argc);
    Configuration config = parseArguments(arg);
    config::Cluster cluster = loadCluster(config.configFilename);

    // parse arguments
    for (int i = 0; i < argc; i++) {
        std::string_view argg = argv[i];
        if (argg == "-tilt" && argc > (i + 1)) {
            tilt = static_cast<float>(atof(argv[i + 1]));
            Logger::Info("Setting tilt to: %f", tilt);
        }
        else if (argg == "-radius" && argc > (i + 1)) {
            radius = static_cast<float>(atof(argv[i + 1]));
            Logger::Info("Setting radius to: %f", radius);
        }
        else if (argg == "-tex" && argc > (i + 1)) {
            texture = argv[i + 1];
            Logger::Info("Using texture: %s", texture.c_str());
        }
    }

    Engine::Callbacks callbacks;
    callbacks.postSyncPreDraw = postSyncPreDraw;
    callbacks.draw = draw;
    callbacks.draw2D = draw2D;
    callbacks.initOpenGL = initGL;
    callbacks.cleanUp = cleanUp;
    callbacks.keyboard = keyboardCallback;
    callbacks.encode = encode;
    callbacks.decode = decode;

    try {
        Engine::create(cluster, callbacks, config);
    }
    catch (const std::runtime_error& e) {
        Logger::Error("%s", e.what());
        Engine::destroy();
        return EXIT_FAILURE;
    }
    
    Engine::instance().render();
    Engine::destroy();
    exit(EXIT_SUCCESS);
}
