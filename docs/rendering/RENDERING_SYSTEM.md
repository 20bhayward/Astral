# Rendering System

## Overview

The Rendering System is responsible for visualizing the cellular simulation created by the physics system. It must handle efficient rendering of thousands to millions of cells, with dynamic lighting, particle effects, and post-processing. The system is built on OpenGL for cross-platform support and performance.

Key requirements for the rendering system:

1. High-performance rendering of a large grid of cells
2. Dynamic lighting effects for emissive materials
3. Fluid and particle visual effects
4. Camera controls (pan, zoom)
5. Debugging visualization tools
6. Support for layer-based rendering (background, cells, foreground, UI)

## Rendering Architecture

The rendering system follows a layered architecture:

```
+---------------------+
| Renderer Interface  |
+---------------------+
          |
+---------v-----------+
| OpenGL Renderer     |
|---------------------|
| - Shader Management |
| - Texture Management|
| - Batch Rendering   |
| - Framebuffer Mgmt  |
+---------------------+
          |
+---------v-----------+
| Rendering Modules   |
|---------------------|
| - Grid Renderer     |
| - Particle Renderer |
| - Lighting System   |
| - Post-Processing   |
| - Debug Visualizer  |
| - UI Renderer       |
+---------------------+
```

## Core Renderer Interface

```cpp
class RenderingSystem {
public:
    virtual ~RenderingSystem() = default;
    
    // Initialization
    virtual bool initialize(int width, int height, const std::string& title) = 0;
    virtual void shutdown() = 0;
    
    // Window management
    virtual void setWindowTitle(const std::string& title) = 0;
    virtual void setWindowSize(int width, int height) = 0;
    virtual void setVSync(bool enabled) = 0;
    virtual void setFullscreen(bool fullscreen) = 0;
    
    // Frame management
    virtual void beginFrame() = 0;
    virtual void endFrame() = 0;
    
    // Viewport
    virtual void setViewport(int x, int y, int width, int height) = 0;
    virtual void setClearColor(float r, float g, float b, float a) = 0;
    
    // Camera
    virtual void setCamera(const Camera& camera) = 0;
    virtual Camera& getCamera() = 0;
    virtual glm::mat4 getViewMatrix() const = 0;
    virtual glm::mat4 getProjectionMatrix() const = 0;
    
    // Drawing methods
    virtual void renderWorld(const ChunkManager* chunkManager, const MaterialRegistry* materials) = 0;
    virtual void renderBackground(const TextureHandle& texture, const glm::vec2& parallaxFactor) = 0;
    virtual void renderParticles(const ParticleSystem* particles) = 0;
    virtual void renderLighting() = 0;
    virtual void renderPostProcessing() = 0;
    virtual void renderUI() = 0;
    
    // Debug rendering
    virtual void renderDebugInfo(bool showPhysics, bool showPerformance) = 0;
    
    // Utility
    virtual glm::vec2 screenToWorld(const glm::vec2& screenPos) = 0;
    virtual glm::vec2 worldToScreen(const glm::vec2& worldPos) = 0;
    virtual WorldRect getVisibleWorldRect() const = 0;
    
    // Window state
    virtual bool isWindowClosed() const = 0;
    virtual void pollEvents() = 0;
    
    // Getters
    virtual int getWidth() const = 0;
    virtual int getHeight() const = 0;
    virtual float getAspectRatio() const = 0;
};
```

## OpenGL Implementation

```cpp
class OpenGLRenderer : public RenderingSystem {
private:
    // Window and context
    GLFWwindow* window;
    int width;
    int height;
    bool vsyncEnabled;
    
    // Camera
    Camera camera;
    
    // Shader management
    std::unordered_map<std::string, ShaderProgram> shaders;
    
    // Texture management
    TextureManager textureManager;
    
    // Render targets
    std::unique_ptr<Framebuffer> worldFramebuffer;
    std::unique_ptr<Framebuffer> lightingFramebuffer;
    std::unique_ptr<Framebuffer> postProcessFramebuffer;
    
    // Rendering components
    std::unique_ptr<GridRenderer> gridRenderer;
    std::unique_ptr<ParticleRenderer> particleRenderer;
    std::unique_ptr<LightingSystem> lightingSystem;
    std::unique_ptr<PostProcessor> postProcessor;
    std::unique_ptr<UIRenderer> uiRenderer;
    std::unique_ptr<DebugRenderer> debugRenderer;
    
    // Performance monitoring
    PerformanceMetrics metrics;
    
    // Helper methods
    bool initializeShaders();
    bool initializeFramebuffers();
    void setupGL();
    
public:
    OpenGLRenderer();
    ~OpenGLRenderer();
    
    // RenderingSystem implementation
    bool initialize(int width, int height, const std::string& title) override;
    void shutdown() override;
    
    void setWindowTitle(const std::string& title) override;
    void setWindowSize(int width, int height) override;
    void setVSync(bool enabled) override;
    void setFullscreen(bool fullscreen) override;
    
    void beginFrame() override;
    void endFrame() override;
    
    void setViewport(int x, int y, int width, int height) override;
    void setClearColor(float r, float g, float b, float a) override;
    
    void setCamera(const Camera& camera) override;
    Camera& getCamera() override;
    glm::mat4 getViewMatrix() const override;
    glm::mat4 getProjectionMatrix() const override;
    
    void renderWorld(const ChunkManager* chunkManager, const MaterialRegistry* materials) override;
    void renderBackground(const TextureHandle& texture, const glm::vec2& parallaxFactor) override;
    void renderParticles(const ParticleSystem* particles) override;
    void renderLighting() override;
    void renderPostProcessing() override;
    void renderUI() override;
    
    void renderDebugInfo(bool showPhysics, bool showPerformance) override;
    
    glm::vec2 screenToWorld(const glm::vec2& screenPos) override;
    glm::vec2 worldToScreen(const glm::vec2& worldPos) override;
    WorldRect getVisibleWorldRect() const override;
    
    bool isWindowClosed() const override;
    void pollEvents() override;
    
    int getWidth() const override { return width; }
    int getHeight() const override { return height; }
    float getAspectRatio() const override { return (float)width / (float)height; }
    
    // OpenGL-specific methods
    GLuint getShaderProgram(const std::string& name);
    void bindShader(const std::string& name);
    void unbindShader();
};
```

## Shader Management

Shaders are a critical component for high-performance rendering:

```cpp
class ShaderProgram {
private:
    GLuint programID;
    std::unordered_map<std::string, GLint> uniformLocations;
    
    bool compileShader(const std::string& source, GLenum shaderType, GLuint& shaderID);
    bool linkProgram(GLuint vertexShader, GLuint fragmentShader);
    
public:
    ShaderProgram();
    ~ShaderProgram();
    
    bool loadFromFiles(const std::string& vertexPath, const std::string& fragmentPath);
    bool loadFromSource(const std::string& vertexSource, const std::string& fragmentSource);
    
    void use();
    void unuse();
    
    GLint getUniformLocation(const std::string& name);
    
    // Uniform setters
    void setUniform(const std::string& name, bool value);
    void setUniform(const std::string& name, int value);
    void setUniform(const std::string& name, float value);
    void setUniform(const std::string& name, const glm::vec2& value);
    void setUniform(const std::string& name, const glm::vec3& value);
    void setUniform(const std::string& name, const glm::vec4& value);
    void setUniform(const std::string& name, const glm::mat3& value);
    void setUniform(const std::string& name, const glm::mat4& value);
    
    GLuint getID() const { return programID; }
};
```

### Default Shaders

The engine contains several default shaders for different rendering tasks:

```glsl
// grid_vertex.glsl
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec4 aColor;
layout (location = 3) in float aEmission;

out vec2 TexCoord;
out vec4 Color;
out float Emission;

uniform mat4 uView;
uniform mat4 uProjection;

void main() {
    gl_Position = uProjection * uView * vec4(aPos, 0.0, 1.0);
    TexCoord = aTexCoord;
    Color = aColor;
    Emission = aEmission;
}

// grid_fragment.glsl
#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
in vec4 Color;
in float Emission;

uniform sampler2D uTexture;
uniform bool uUseTexture;

void main() {
    vec4 color = Color;
    
    if (uUseTexture) {
        color *= texture(uTexture, TexCoord);
    }
    
    // Add emission glow (will be used by lighting pass)
    FragColor = color;
    FragColor.a *= color.a; // Preserve transparency
}

// lighting_vertex.glsl
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

void main() {
    gl_Position = vec4(aPos, 0.0, 1.0);
    TexCoord = aTexCoord;
}

// lighting_fragment.glsl
#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D uColorTex;
uniform sampler2D uEmissionTex;
uniform sampler2D uLightTex;

uniform vec3 uAmbientLight;
uniform float uTime;

void main() {
    vec4 color = texture(uColorTex, TexCoord);
    vec4 emission = texture(uEmissionTex, TexCoord);
    vec3 lighting = texture(uLightTex, TexCoord).rgb;
    
    // Apply ambient light
    vec3 ambient = color.rgb * uAmbientLight;
    
    // Apply dynamic lighting
    vec3 lit = color.rgb * lighting;
    
    // Add emission 
    lit += emission.rgb * emission.a;
    
    // Apply some subtle animation to emissive materials
    float flickerAmount = 0.9 + 0.1 * sin(uTime * 10.0 + TexCoord.x * 20.0 + TexCoord.y * 30.0);
    lit += emission.rgb * emission.a * flickerAmount;
    
    // Final color
    FragColor = vec4(ambient + lit, color.a);
}

// particle_vertex.glsl
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aSize;
layout (location = 2) in vec4 aColor;
layout (location = 3) in float aRotation;
layout (location = 4) in vec2 aTexCoord;

out vec2 TexCoord;
out vec4 Color;

uniform mat4 uView;
uniform mat4 uProjection;

void main() {
    // Calculate rotation matrix
    float cosA = cos(aRotation);
    float sinA = sin(aRotation);
    mat2 rot = mat2(cosA, -sinA, sinA, cosA);
    
    // Generate quad from point
    vec2 quad[4];
    quad[0] = vec2(-0.5, -0.5);
    quad[1] = vec2(0.5, -0.5);
    quad[2] = vec2(0.5, 0.5);
    quad[3] = vec2(-0.5, 0.5);
    
    // Apply rotation and size
    int vertexID = gl_VertexID % 4;
    vec2 pos = aPos + rot * quad[vertexID] * aSize;
    
    gl_Position = uProjection * uView * vec4(pos, 0.0, 1.0);
    
    // Texture coordinates
    vec2 texCoords[4];
    texCoords[0] = vec2(0.0, 0.0);
    texCoords[1] = vec2(1.0, 0.0);
    texCoords[2] = vec2(1.0, 1.0);
    texCoords[3] = vec2(0.0, 1.0);
    
    TexCoord = texCoords[vertexID];
    Color = aColor;
}

// particle_fragment.glsl
#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
in vec4 Color;

uniform sampler2D uTexture;
uniform bool uUseTexture;

void main() {
    vec4 color = Color;
    
    if (uUseTexture) {
        color *= texture(uTexture, TexCoord);
    }
    
    FragColor = color;
}

// post_vertex.glsl
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

void main() {
    gl_Position = vec4(aPos, 0.0, 1.0);
    TexCoord = aTexCoord;
}

// post_fragment.glsl
#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D uTexture;
uniform float uTime;
uniform bool uEnableBloom;
uniform bool uEnableVignette;
uniform bool uEnableChromatic;
uniform bool uEnableFilmGrain;

// Bloom
uniform sampler2D uBloomTex;
uniform float uBloomStrength;

// Vignette
uniform float uVignetteStrength;
uniform float uVignetteSize;

// Chromatic aberration
uniform float uChromaticStrength;

// Film grain
uniform float uGrainStrength;

float random(vec2 st) {
    return fract(sin(dot(st.xy, vec2(12.9898, 78.233))) * 43758.5453123);
}

void main() {
    vec2 uv = TexCoord;
    vec4 color = texture(uTexture, uv);
    
    // Chromatic aberration
    if (uEnableChromatic) {
        float aberrationAmount = uChromaticStrength * 0.01;
        vec2 dir = uv - 0.5;
        float dist = length(dir);
        dir = normalize(dir);
        
        vec2 uvR = uv + dir * aberrationAmount * dist;
        vec2 uvB = uv - dir * aberrationAmount * dist;
        
        color.r = texture(uTexture, uvR).r;
        color.b = texture(uTexture, uvB).b;
    }
    
    // Bloom
    if (uEnableBloom) {
        vec4 bloomColor = texture(uBloomTex, uv);
        color.rgb += bloomColor.rgb * uBloomStrength;
    }
    
    // Vignette
    if (uEnableVignette) {
        vec2 center = vec2(0.5, 0.5);
        float dist = distance(uv, center);
        float vignette = smoothstep(uVignetteSize, uVignetteSize - 0.2, dist);
        color.rgb = mix(color.rgb * vignette, color.rgb, 1.0 - uVignetteStrength);
    }
    
    // Film grain
    if (uEnableFilmGrain) {
        float grain = random(uv * uTime) * uGrainStrength;
        color.rgb += vec3(grain);
    }
    
    FragColor = color;
}
```

## Optimized Grid Rendering

The most performance-critical part of the rendering system is the grid renderer:

```cpp
class GridRenderer {
private:
    struct GridVertex {
        glm::vec2 position;
        glm::vec2 texCoord;
        glm::vec4 color;
        float emission;
    };
    
    // OpenGL objects
    GLuint VAO;
    GLuint VBO;
    GLuint instanceVBO;
    GLuint texture;
    
    // Batch rendering
    std::vector<GridVertex> vertices;
    
    // Rendering parameters
    int gridWidth;
    int gridHeight;
    float cellSize;
    
    // Textures
    TextureAtlas materialTextureAtlas;
    
    // Shaders
    ShaderProgram* gridShader;
    
    // Camera reference
    Camera* camera;
    
    // Statistics
    int renderedCells;
    int skippedCells;
    
public:
    GridRenderer(ShaderProgram* shader, Camera* camera);
    ~GridRenderer();
    
    bool initialize();
    
    void setCellSize(float size) { cellSize = size; }
    void setGridDimensions(int width, int height) { gridWidth = width; gridHeight = height; }
    
    // Main render method
    void render(const ChunkManager* chunkManager, const MaterialRegistry* materials);
    
    // Instanced rendering - much faster for large grids
    void renderInstanced(const ChunkManager* chunkManager, const MaterialRegistry* materials);
    
    // Get rendering statistics
    int getRenderedCellCount() const { return renderedCells; }
    int getSkippedCellCount() const { return skippedCells; }
    
private:
    void setupBuffers();
    void buildInstanceData(const ChunkManager* chunkManager, const MaterialRegistry* materials, 
                          std::vector<glm::vec4>& instanceData);
    void renderCellsBatched(const ChunkManager* chunkManager, const MaterialRegistry* materials);
};

GridRenderer::GridRenderer(ShaderProgram* shader, Camera* camera)
    : gridShader(shader), camera(camera), cellSize(1.0f), gridWidth(0), gridHeight(0),
      renderedCells(0), skippedCells(0) {
}

bool GridRenderer::initialize() {
    // Create VAO and VBO
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &instanceVBO);
    
    // Set up vertex data for a quad
    setupBuffers();
    
    // Load material textures
    materialTextureAtlas.initialize(2048, 2048);
    
    return true;
}

void GridRenderer::setupBuffers() {
    // Basic quad positions (2 triangles forming a square)
    float quadVertices[] = {
        // positions        // texCoords
        0.0f, 0.0f,         0.0f, 0.0f,
        1.0f, 0.0f,         1.0f, 0.0f,
        1.0f, 1.0f,         1.0f, 1.0f,
        0.0f, 0.0f,         0.0f, 0.0f,
        1.0f, 1.0f,         1.0f, 1.0f,
        0.0f, 1.0f,         0.0f, 1.0f
    };
    
    glBindVertexArray(VAO);
    
    // Position and texture
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    
    // Position attribute
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Texture attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    // Set up instance VBO for cell data
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    // We'll fill this with data at render time
    
    // Instance data layout: vec4(x, y, materialID, flags)
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), (void*)0);
    glEnableVertexAttribArray(2);
    glVertexAttribDivisor(2, 1); // Tell OpenGL this is per-instance
    
    glBindVertexArray(0);
}

void GridRenderer::renderInstanced(const ChunkManager* chunkManager, const MaterialRegistry* materials) {
    if (!chunkManager) return;
    
    renderedCells = 0;
    skippedCells = 0;
    
    // Get visible area from camera
    WorldRect visibleArea = camera->getVisibleWorldRect();
    
    // Expand by 1 chunk in each direction to ensure we render everything
    visibleArea.x -= CHUNK_SIZE;
    visibleArea.y -= CHUNK_SIZE;
    visibleArea.width += CHUNK_SIZE * 2;
    visibleArea.height += CHUNK_SIZE * 2;
    
    // Build instance data for visible cells
    std::vector<glm::vec4> instanceData;
    buildInstanceData(chunkManager, materials, instanceData);
    
    if (instanceData.empty()) return;
    
    // Upload instance data to GPU
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, instanceData.size() * sizeof(glm::vec4), instanceData.data(), GL_STREAM_DRAW);
    
    // Bind shader
    gridShader->use();
    
    // Set uniforms
    gridShader->setUniform("uView", camera->getViewMatrix());
    gridShader->setUniform("uProjection", camera->getProjectionMatrix());
    gridShader->setUniform("uCellSize", cellSize);
    gridShader->setUniform("uUseTexture", true);
    
    // Bind texture atlas
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, materialTextureAtlas.getTextureID());
    gridShader->setUniform("uTexture", 0);
    
    // Render all cells in one draw call
    glBindVertexArray(VAO);
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, instanceData.size());
    glBindVertexArray(0);
    
    // Unbind shader
    gridShader->unuse();
    
    renderedCells = instanceData.size();
}

void GridRenderer::buildInstanceData(const ChunkManager* chunkManager, const MaterialRegistry* materials, 
                                    std::vector<glm::vec4>& instanceData) {
    // Get visible area from camera
    WorldRect visibleArea = camera->getVisibleWorldRect();
    
    // Expand by 1 chunk in each direction to ensure we render everything
    visibleArea.x -= CHUNK_SIZE;
    visibleArea.y -= CHUNK_SIZE;
    visibleArea.width += CHUNK_SIZE * 2;
    visibleArea.height += CHUNK_SIZE * 2;
    
    // Clamp to world boundaries
    visibleArea.x = std::max(0, visibleArea.x);
    visibleArea.y = std::max(0, visibleArea.y);
    visibleArea.width = std::min(gridWidth - visibleArea.x, visibleArea.width);
    visibleArea.height = std::min(gridHeight - visibleArea.y, visibleArea.height);
    
    // Reserve memory to avoid reallocations
    instanceData.reserve(visibleArea.width * visibleArea.height);
    
    // For each visible chunk
    for (int chunkY = visibleArea.y / CHUNK_SIZE; chunkY <= (visibleArea.y + visibleArea.height) / CHUNK_SIZE; chunkY++) {
        for (int chunkX = visibleArea.x / CHUNK_SIZE; chunkX <= (visibleArea.x + visibleArea.width) / CHUNK_SIZE; chunkX++) {
            ChunkCoord chunkCoord{chunkX, chunkY};
            Chunk* chunk = chunkManager->getChunk(chunkCoord);
            
            if (!chunk) {
                continue; // Skip unloaded chunks
            }
            
            // Calculate visible area within this chunk
            int startX = std::max(0, visibleArea.x - chunkX * CHUNK_SIZE);
            int startY = std::max(0, visibleArea.y - chunkY * CHUNK_SIZE);
            int endX = std::min(CHUNK_SIZE, (visibleArea.x + visibleArea.width) - chunkX * CHUNK_SIZE);
            int endY = std::min(CHUNK_SIZE, (visibleArea.y + visibleArea.height) - chunkY * CHUNK_SIZE);
            
            // For each visible cell in the chunk
            for (int localY = startY; localY < endY; localY++) {
                for (int localX = startX; localX < endX; localX++) {
                    const Cell& cell = chunk->getCell(localX, localY);
                    
                    // Skip empty cells
                    if (cell.material == materialRegistry->air()) {
                        skippedCells++;
                        continue;
                    }
                    
                    // Calculate world position
                    int worldX = chunkX * CHUNK_SIZE + localX;
                    int worldY = chunkY * CHUNK_SIZE + localY;
                    
                    // Add instance data: position, material ID, and flags
                    float flags = 0.0f;
                    if (chunk->isCellActive(localX, localY)) {
                        flags = 1.0f; // Example flag for active cells
                    }
                    
                    // Pack data: x, y, materialID, flags
                    glm::vec4 data(worldX, worldY, static_cast<float>(cell.material), flags);
                    instanceData.push_back(data);
                }
            }
        }
    }
}
```

## Texture Atlas System

To optimize rendering, materials use a texture atlas:

```cpp
class TextureAtlas {
private:
    GLuint textureID;
    int width;
    int height;
    int cellSize;
    int columns;
    int rows;
    std::unordered_map<MaterialID, glm::vec4> materialUVs;
    
public:
    TextureAtlas();
    ~TextureAtlas();
    
    bool initialize(int atlasWidth, int atlasHeight, int cellSize = 32);
    bool addMaterial(MaterialID id, const std::string& texturePath);
    bool addMaterialColor(MaterialID id, const glm::vec4& color);
    
    glm::vec4 getMaterialUV(MaterialID id) const;
    GLuint getTextureID() const { return textureID; }
};

bool TextureAtlas::initialize(int atlasWidth, int atlasHeight, int cellSize) {
    this->width = atlasWidth;
    this->height = atlasHeight;
    this->cellSize = cellSize;
    
    columns = atlasWidth / cellSize;
    rows = atlasHeight / cellSize;
    
    // Create texture
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    
    // Allocate memory for atlas
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    
    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    return true;
}

bool TextureAtlas::addMaterial(MaterialID id, const std::string& texturePath) {
    // Load texture
    int width, height, channels;
    unsigned char* data = stbi_load(texturePath.c_str(), &width, &height, &channels, 4);
    
    if (!data) {
        // Failed to load texture
        return false;
    }
    
    // Find next free spot in atlas
    int materialIndex = materialUVs.size();
    int row = materialIndex / columns;
    int col = materialIndex % columns;
    
    if (row >= rows) {
        // Atlas is full
        stbi_image_free(data);
        return false;
    }
    
    // Calculate UV coordinates
    float uMin = (float)(col * cellSize) / (float)this->width;
    float vMin = (float)(row * cellSize) / (float)this->height;
    float uMax = (float)((col + 1) * cellSize) / (float)this->width;
    float vMax = (float)((row + 1) * cellSize) / (float)this->height;
    
    materialUVs[id] = glm::vec4(uMin, vMin, uMax, vMax);
    
    // Copy texture data to atlas
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexSubImage2D(GL_TEXTURE_2D, 0, col * cellSize, row * cellSize, cellSize, cellSize, GL_RGBA, GL_UNSIGNED_BYTE, data);
    
    stbi_image_free(data);
    return true;
}

bool TextureAtlas::addMaterialColor(MaterialID id, const glm::vec4& color) {
    // Find next free spot in atlas
    int materialIndex = materialUVs.size();
    int row = materialIndex / columns;
    int col = materialIndex % columns;
    
    if (row >= rows) {
        // Atlas is full
        return false;
    }
    
    // Calculate UV coordinates
    float uMin = (float)(col * cellSize) / (float)this->width;
    float vMin = (float)(row * cellSize) / (float)this->height;
    float uMax = (float)((col + 1) * cellSize) / (float)this->width;
    float vMax = (float)((row + 1) * cellSize) / (float)this->height;
    
    materialUVs[id] = glm::vec4(uMin, vMin, uMax, vMax);
    
    // Create a solid color texture
    std::vector<unsigned char> pixelData(cellSize * cellSize * 4);
    for (int i = 0; i < cellSize * cellSize; i++) {
        pixelData[i * 4 + 0] = static_cast<unsigned char>(color.r * 255.0f);
        pixelData[i * 4 + 1] = static_cast<unsigned char>(color.g * 255.0f);
        pixelData[i * 4 + 2] = static_cast<unsigned char>(color.b * 255.0f);
        pixelData[i * 4 + 3] = static_cast<unsigned char>(color.a * 255.0f);
    }
    
    // Copy texture data to atlas
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexSubImage2D(GL_TEXTURE_2D, 0, col * cellSize, row * cellSize, cellSize, cellSize, GL_RGBA, GL_UNSIGNED_BYTE, pixelData.data());
    
    return true;
}

glm::vec4 TextureAtlas::getMaterialUV(MaterialID id) const {
    auto it = materialUVs.find(id);
    if (it != materialUVs.end()) {
        return it->second;
    }
    
    // Return default UV (0,0,1,1)
    return glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
}
```

## Lighting System

Dynamic lighting for the cellular simulation:

```cpp
class LightingSystem {
private:
    // Lighting data
    std::vector<PointLight> pointLights;
    glm::vec3 ambientLight;
    
    // Framebuffers for light calculation
    std::unique_ptr<Framebuffer> lightAccumulationBuffer;
    std::unique_ptr<Framebuffer> blurBuffer;
    
    // Shaders
    ShaderProgram* lightShader;
    ShaderProgram* blurShader;
    
    // Screen quad for deferred lighting
    GLuint quadVAO;
    GLuint quadVBO;
    
    // Dimensions
    int width;
    int height;
    
public:
    LightingSystem(ShaderProgram* lightShader, ShaderProgram* blurShader, int width, int height);
    ~LightingSystem();
    
    bool initialize();
    void resize(int width, int height);
    
    // Light management
    void setAmbientLight(const glm::vec3& color);
    PointLight* addPointLight(const glm::vec2& position, const glm::vec3& color, float radius, float intensity);
    void removePointLight(PointLight* light);
    void clearLights();
    
    // Dynamic light generation from emissive materials
    void generateLightsFromMaterials(const ChunkManager* chunkManager, const MaterialRegistry* materials);
    
    // Main render method
    void render(const Framebuffer* worldColorBuffer, const Framebuffer* worldEmissionBuffer);
    
private:
    void setupQuad();
    void renderLightPass(const Framebuffer* worldColorBuffer, const Framebuffer* worldEmissionBuffer);
    void blurPass(const Framebuffer* inputBuffer);
};

struct PointLight {
    glm::vec2 position;
    glm::vec3 color;
    float radius;
    float intensity;
    bool active;
    
    PointLight(const glm::vec2& pos, const glm::vec3& col, float rad, float intens)
        : position(pos), color(col), radius(rad), intensity(intens), active(true) {}
};

void LightingSystem::generateLightsFromMaterials(const ChunkManager* chunkManager, const MaterialRegistry* materials) {
    // Clear existing dynamic lights
    clearLights();
    
    // Get visible area from camera
    WorldRect visibleArea = camera->getVisibleWorldRect();
    
    // Expand by the maximum light radius to ensure we catch all lights
    float maxLightRadius = 20.0f; // Example - should be based on the maximum light radius in your game
    visibleArea.x -= maxLightRadius;
    visibleArea.y -= maxLightRadius;
    visibleArea.width += maxLightRadius * 2;
    visibleArea.height += maxLightRadius * 2;
    
    // For each visible chunk
    for (int chunkY = visibleArea.y / CHUNK_SIZE; chunkY <= (visibleArea.y + visibleArea.height) / CHUNK_SIZE; chunkY++) {
        for (int chunkX = visibleArea.x / CHUNK_SIZE; chunkX <= (visibleArea.x + visibleArea.width) / CHUNK_SIZE; chunkX++) {
            ChunkCoord chunkCoord{chunkX, chunkY};
            Chunk* chunk = chunkManager->getChunk(chunkCoord);
            
            if (!chunk) {
                continue; // Skip unloaded chunks
            }
            
            // For each cell in the chunk
            for (int localY = 0; localY < CHUNK_SIZE; localY++) {
                for (int localX = 0; localX < CHUNK_SIZE; localX++) {
                    const Cell& cell = chunk->getCell(localX, localY);
                    
                    // Skip empty cells
                    if (cell.material == materials->air()) {
                        continue;
                    }
                    
                    const MaterialProperties& props = materials->getMaterial(cell.material);
                    
                    // Check if material is emissive
                    if (props.emissive && props.emissiveStrength > 0.0f) {
                        // Calculate world position
                        int worldX = chunkX * CHUNK_SIZE + localX;
                        int worldY = chunkY * CHUNK_SIZE + localY;
                        
                        // Create light
                        float radius = props.emissiveStrength * 10.0f;
                        float intensity = props.emissiveStrength;
                        
                        // Convert material color to light color
                        glm::vec3 lightColor = glm::vec3(props.color.r, props.color.g, props.color.b);
                        
                        // Add point light
                        addPointLight(glm::vec2(worldX, worldY), lightColor, radius, intensity);
                    }
                }
            }
        }
    }
}

void LightingSystem::render(const Framebuffer* worldColorBuffer, const Framebuffer* worldEmissionBuffer) {
    // Render lights to accumulation buffer
    renderLightPass(worldColorBuffer, worldEmissionBuffer);
    
    // Apply blur to smooth lighting
    blurPass(lightAccumulationBuffer.get());
}

void LightingSystem::renderLightPass(const Framebuffer* worldColorBuffer, const Framebuffer* worldEmissionBuffer) {
    // Bind light accumulation buffer
    lightAccumulationBuffer->bind();
    
    // Clear with ambient light
    glClearColor(ambientLight.r, ambientLight.g, ambientLight.b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    // Bind light shader
    lightShader->use();
    
    // Set uniforms
    lightShader->setUniform("uScreenSize", glm::vec2(width, height));
    lightShader->setUniform("uTime", static_cast<float>(glfwGetTime()));
    
    // Bind color and emission textures
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, worldColorBuffer->getColorTexture());
    lightShader->setUniform("uColorTex", 0);
    
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, worldEmissionBuffer->getColorTexture());
    lightShader->setUniform("uEmissionTex", 1);
    
    // Enable additive blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    
    // Draw each point light
    for (const auto& light : pointLights) {
        if (!light.active) continue;
        
        // Set light properties
        lightShader->setUniform("uLightPos", light.position);
        lightShader->setUniform("uLightColor", light.color);
        lightShader->setUniform("uLightRadius", light.radius);
        lightShader->setUniform("uLightIntensity", light.intensity);
        
        // Draw fullscreen quad
        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
    
    // Restore default blend mode
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Unbind
    lightShader->unuse();
    lightAccumulationBuffer->unbind();
}

void LightingSystem::blurPass(const Framebuffer* inputBuffer) {
    // Horizontal blur
    blurBuffer->bind();
    blurShader->use();
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, inputBuffer->getColorTexture());
    blurShader->setUniform("uTexture", 0);
    blurShader->setUniform("uDirection", glm::vec2(1.0f, 0.0f));
    blurShader->setUniform("uResolution", glm::vec2(width, height));
    
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    blurBuffer->unbind();
    
    // Vertical blur
    lightAccumulationBuffer->bind();
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, blurBuffer->getColorTexture());
    blurShader->setUniform("uDirection", glm::vec2(0.0f, 1.0f));
    
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    blurShader->unuse();
    lightAccumulationBuffer->unbind();
}
```

## Particle System and Renderer

For additional visual effects:

```cpp
struct Particle {
    glm::vec2 position;
    glm::vec2 velocity;
    glm::vec2 size;
    glm::vec4 color;
    float rotation;
    float rotationSpeed;
    float lifetime;
    float maxLifetime;
};

class ParticleSystem {
private:
    std::vector<Particle> particles;
    size_t activeParticles;
    
    // Particle emitters
    std::vector<ParticleEmitter*> emitters;
    
public:
    ParticleSystem(size_t maxParticles = 10000);
    ~ParticleSystem();
    
    void update(float deltaTime);
    
    // Particle management
    Particle* createParticle();
    void killParticle(size_t index);
    
    // Emitter management
    ParticleEmitter* createEmitter(const ParticleEmitterDesc& desc);
    void removeEmitter(ParticleEmitter* emitter);
    
    // Access particles for rendering
    const std::vector<Particle>& getParticles() const { return particles; }
    size_t getActiveParticleCount() const { return activeParticles; }
};

class ParticleRenderer {
private:
    // OpenGL objects
    GLuint VAO;
    GLuint VBO;
    
    // Shader
    ShaderProgram* particleShader;
    
    // Textures
    GLuint particleTexture;
    
    // Camera reference
    Camera* camera;
    
    // Rendering data
    std::vector<ParticleVertex> vertices;
    
public:
    ParticleRenderer(ShaderProgram* shader, Camera* camera);
    ~ParticleRenderer();
    
    bool initialize();
    
    // Load particle texture
    bool setParticleTexture(const std::string& texturePath);
    
    // Main render method
    void render(const ParticleSystem* particleSystem);
    
private:
    void setupBuffers();
};

struct ParticleVertex {
    glm::vec2 position;
    glm::vec2 size;
    glm::vec4 color;
    float rotation;
    glm::vec2 texCoord;
};

void ParticleRenderer::render(const ParticleSystem* particleSystem) {
    if (!particleSystem) return;
    
    const auto& particles = particleSystem->getParticles();
    size_t activeCount = particleSystem->getActiveParticleCount();
    
    if (activeCount == 0) return;
    
    // Prepare vertex data
    vertices.clear();
    vertices.reserve(activeCount * 4); // 4 vertices per particle (quad)
    
    // For each active particle, generate quad vertices
    for (size_t i = 0; i < activeCount; i++) {
        const Particle& p = particles[i];
        
        // Skip particles with zero size
        if (p.size.x <= 0.0f || p.size.y <= 0.0f) continue;
        
        // Alpha based on lifetime
        float alpha = p.lifetime / p.maxLifetime;
        glm::vec4 color = p.color;
        color.a *= alpha;
        
        // Create vertex for instanced rendering
        ParticleVertex vertex;
        vertex.position = p.position;
        vertex.size = p.size;
        vertex.color = color;
        vertex.rotation = p.rotation;
        vertex.texCoord = glm::vec2(0.0f); // This will be set in the shader
        
        vertices.push_back(vertex);
    }
    
    // If no visible particles, exit
    if (vertices.empty()) return;
    
    // Upload vertices to GPU
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(ParticleVertex), vertices.data(), GL_STREAM_DRAW);
    
    // Bind shader
    particleShader->use();
    
    // Set uniforms
    particleShader->setUniform("uView", camera->getViewMatrix());
    particleShader->setUniform("uProjection", camera->getProjectionMatrix());
    
    // Bind texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, particleTexture);
    particleShader->setUniform("uTexture", 0);
    particleShader->setUniform("uUseTexture", true);
    
    // Enable blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Render particles
    glBindVertexArray(VAO);
    glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, vertices.size());
    glBindVertexArray(0);
    
    // Restore state
    particleShader->unuse();
}
```

## Camera System

```cpp
class Camera {
private:
    glm::vec2 position;
    float zoom;
    
    // Viewport dimensions
    int viewportWidth;
    int viewportHeight;
    
    // Matrices
    glm::mat4 viewMatrix;
    glm::mat4 projectionMatrix;
    
    // Bounds
    bool boundingEnabled;
    glm::vec2 minBounds;
    glm::vec2 maxBounds;
    
    // Shake effect
    float shakeIntensity;
    float shakeDuration;
    float shakeTimer;
    
    // For smooth motion
    glm::vec2 targetPosition;
    float targetZoom;
    float smoothingFactor;
    
public:
    Camera();
    
    // Set properties
    void setPosition(const glm::vec2& pos);
    void setZoom(float zoom);
    void setViewport(int width, int height);
    
    // Get properties
    const glm::vec2& getPosition() const { return position; }
    float getZoom() const { return zoom; }
    
    // Set bounds
    void enableBounding(bool enable);
    void setBounds(const glm::vec2& min, const glm::vec2& max);
    
    // Camera effects
    void shake(float intensity, float duration);
    
    // Smooth motion
    void setTargetPosition(const glm::vec2& target);
    void setTargetZoom(float target);
    void setSmoothingFactor(float factor);
    
    // Update camera (smooth motion, shake)
    void update(float deltaTime);
    
    // Get matrices
    const glm::mat4& getViewMatrix() const { return viewMatrix; }
    const glm::mat4& getProjectionMatrix() const { return projectionMatrix; }
    
    // Convert between screen and world coordinates
    glm::vec2 screenToWorld(const glm::vec2& screenPos) const;
    glm::vec2 worldToScreen(const glm::vec2& worldPos) const;
    
    // Get visible area in world coordinates
    WorldRect getVisibleWorldRect() const;
    
private:
    void updateMatrices();
    void applyBounds();
};

void Camera::update(float deltaTime) {
    // Smooth position movement
    if (smoothingFactor > 0.0f) {
        position = glm::mix(position, targetPosition, smoothingFactor * deltaTime * 10.0f);
        zoom = glm::mix(zoom, targetZoom, smoothingFactor * deltaTime * 5.0f);
    }
    
    // Apply camera shake
    if (shakeTimer > 0.0f) {
        shakeTimer -= deltaTime;
        
        if (shakeTimer <= 0.0f) {
            // Shake finished
            shakeTimer = 0.0f;
            shakeIntensity = 0.0f;
        } else {
            // Apply random offset
            float intensity = shakeIntensity * (shakeTimer / shakeDuration);
            float offsetX = (rand() / (float)RAND_MAX * 2.0f - 1.0f) * intensity;
            float offsetY = (rand() / (float)RAND_MAX * 2.0f - 1.0f) * intensity;
            
            position.x += offsetX;
            position.y += offsetY;
        }
    }
    
    // Apply bounds
    if (boundingEnabled) {
        applyBounds();
    }
    
    // Update matrices
    updateMatrices();
}

void Camera::updateMatrices() {
    // Create view matrix
    viewMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(-position.x, -position.y, 0.0f));
    
    // Create projection matrix (orthographic)
    float aspect = (float)viewportWidth / (float)viewportHeight;
    float width = zoom * aspect;
    float height = zoom;
    
    projectionMatrix = glm::ortho(-width / 2.0f, width / 2.0f, height / 2.0f, -height / 2.0f, -1.0f, 1.0f);
}
```

## Post-Processing System

For visual effects:

```cpp
class PostProcessor {
private:
    // Framebuffers
    std::unique_ptr<Framebuffer> primaryBuffer;
    std::unique_ptr<Framebuffer> secondaryBuffer;
    std::unique_ptr<Framebuffer> bloomBuffer;
    
    // Shaders
    ShaderProgram* postShader;
    ShaderProgram* blurShader;
    ShaderProgram* thresholdShader;
    
    // Screen quad
    GLuint quadVAO;
    GLuint quadVBO;
    
    // Effects settings
    bool bloomEnabled;
    float bloomThreshold;
    float bloomIntensity;
    
    bool vignetteEnabled;
    float vignetteIntensity;
    float vignetteSize;
    
    bool chromaticAberrationEnabled;
    float chromaticAberrationIntensity;
    
    bool filmGrainEnabled;
    float filmGrainIntensity;
    
    // Dimensions
    int width;
    int height;
    
public:
    PostProcessor(ShaderProgram* postShader, ShaderProgram* blurShader, ShaderProgram* thresholdShader, int width, int height);
    ~PostProcessor();
    
    bool initialize();
    void resize(int width, int height);
    
    // Set effect parameters
    void setBloom(bool enabled, float threshold = 0.8f, float intensity = 1.0f);
    void setVignette(bool enabled, float intensity = 0.5f, float size = 0.8f);
    void setChromaticAberration(bool enabled, float intensity = 0.5f);
    void setFilmGrain(bool enabled, float intensity = 0.1f);
    
    // Main render method
    void process(const Framebuffer* inputBuffer);
    
    // Get the result of post-processing
    const Framebuffer* getResult() const { return primaryBuffer.get(); }
    
private:
    void setupQuad();
    void bloomPass(const Framebuffer* inputBuffer);
    void finalPass(const Framebuffer* inputBuffer);
};

void PostProcessor::process(const Framebuffer* inputBuffer) {
    if (!inputBuffer) return;
    
    // Bloom pass
    if (bloomEnabled) {
        bloomPass(inputBuffer);
    }
    
    // Final composite pass
    finalPass(inputBuffer);
}

void PostProcessor::bloomPass(const Framebuffer* inputBuffer) {
    // Extract bright areas
    bloomBuffer->bind();
    thresholdShader->use();
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, inputBuffer->getColorTexture());
    thresholdShader->setUniform("uTexture", 0);
    thresholdShader->setUniform("uThreshold", bloomThreshold);
    
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    thresholdShader->unuse();
    bloomBuffer->unbind();
    
    // Horizontal blur
    secondaryBuffer->bind();
    blurShader->use();
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, bloomBuffer->getColorTexture());
    blurShader->setUniform("uTexture", 0);
    blurShader->setUniform("uDirection", glm::vec2(1.0f, 0.0f));
    blurShader->setUniform("uResolution", glm::vec2(width, height));
    
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    secondaryBuffer->unbind();
    
    // Vertical blur
    bloomBuffer->bind();
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, secondaryBuffer->getColorTexture());
    blurShader->setUniform("uDirection", glm::vec2(0.0f, 1.0f));
    
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    blurShader->unuse();
    bloomBuffer->unbind();
}

void PostProcessor::finalPass(const Framebuffer* inputBuffer) {
    primaryBuffer->bind();
    postShader->use();
    
    // Bind input texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, inputBuffer->getColorTexture());
    postShader->setUniform("uTexture", 0);
    
    // Bind bloom texture if enabled
    if (bloomEnabled) {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, bloomBuffer->getColorTexture());
        postShader->setUniform("uBloomTex", 1);
    }
    
    // Set effect parameters
    postShader->setUniform("uTime", static_cast<float>(glfwGetTime()));
    postShader->setUniform("uEnableBloom", bloomEnabled);
    postShader->setUniform("uBloomStrength", bloomIntensity);
    
    postShader->setUniform("uEnableVignette", vignetteEnabled);
    postShader->setUniform("uVignetteStrength", vignetteIntensity);
    postShader->setUniform("uVignetteSize", vignetteSize);
    
    postShader->setUniform("uEnableChromatic", chromaticAberrationEnabled);
    postShader->setUniform("uChromaticStrength", chromaticAberrationIntensity);
    
    postShader->setUniform("uEnableFilmGrain", filmGrainEnabled);
    postShader->setUniform("uGrainStrength", filmGrainIntensity);
    
    // Draw fullscreen quad
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    postShader->unuse();
    primaryBuffer->unbind();
}
```

## Framebuffer Management

```cpp
class Framebuffer {
private:
    GLuint fbo;
    GLuint colorTexture;
    GLuint depthRenderbuffer;
    
    int width;
    int height;
    
public:
    Framebuffer(int width, int height);
    ~Framebuffer();
    
    bool initialize();
    void resize(int width, int height);
    
    void bind() const;
    void unbind() const;
    
    GLuint getColorTexture() const { return colorTexture; }
    int getWidth() const { return width; }
    int getHeight() const { return height; }
};

Framebuffer::Framebuffer(int width, int height) : width(width), height(height), fbo(0), colorTexture(0), depthRenderbuffer(0) {
}

Framebuffer::~Framebuffer() {
    if (fbo != 0) {
        glDeleteFramebuffers(1, &fbo);
    }
    if (colorTexture != 0) {
        glDeleteTextures(1, &colorTexture);
    }
    if (depthRenderbuffer != 0) {
        glDeleteRenderbuffers(1, &depthRenderbuffer);
    }
}

bool Framebuffer::initialize() {
    // Create framebuffer
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    
    // Create color attachment
    glGenTextures(1, &colorTexture);
    glBindTexture(GL_TEXTURE_2D, colorTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0);
    
    // Create depth attachment
    glGenRenderbuffers(1, &depthRenderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, depthRenderbuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRenderbuffer);
    
    // Check if framebuffer is complete
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Failed to create framebuffer!" << std::endl;
        return false;
    }
    
    // Unbind
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    return true;
}

void Framebuffer::resize(int newWidth, int newHeight) {
    if (newWidth == width && newHeight == height) {
        return;
    }
    
    width = newWidth;
    height = newHeight;
    
    // Recreate textures and buffers
    glBindTexture(GL_TEXTURE_2D, colorTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    
    glBindRenderbuffer(GL_RENDERBUFFER, depthRenderbuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
}
```

## Debug Visualization

Debug tools for rendering:

```cpp
class DebugRenderer {
private:
    // Shaders
    ShaderProgram* lineShader;
    
    // Buffers
    GLuint lineVAO;
    GLuint lineVBO;
    
    // Debug shapes
    std::vector<DebugLine> lines;
    std::vector<DebugRect> rects;
    std::vector<DebugCircle> circles;
    std::vector<DebugText> texts;
    
    // Text rendering
    std::unique_ptr<TextRenderer> textRenderer;
    
    // Camera reference
    Camera* camera;
    
public:
    DebugRenderer(ShaderProgram* lineShader, Camera* camera);
    ~DebugRenderer();
    
    bool initialize();
    
    // Drawing methods
    void drawLine(const glm::vec2& start, const glm::vec2& end, const glm::vec4& color, float duration = 0.0f);
    void drawRect(const glm::vec2& pos, const glm::vec2& size, const glm::vec4& color, float duration = 0.0f);
    void drawCircle(const glm::vec2& center, float radius, const glm::vec4& color, int segments = 16, float duration = 0.0f);
    void drawText(const glm::vec2& pos, const std::string& text, const glm::vec4& color, float scale = 1.0f, float duration = 0.0f);
    
    // Grid
    void drawGrid(const glm::vec2& origin, const glm::vec2& size, float cellSize, const glm::vec4& color);
    
    // Physics debug visualization
    void drawActiveChunks(const ChunkManager* chunkManager, const glm::vec4& color);
    void drawChunkBoundaries(const ChunkManager* chunkManager, const glm::vec4& color);
    void drawVelocityField(const ChunkManager* chunkManager, const MaterialRegistry* materials, float scale = 1.0f);
    
    // Clear all debug shapes
    void clear();
    
    // Update (removes expired shapes)
    void update(float deltaTime);
    
    // Render all debug shapes
    void render();
    
private:
    void setupBuffers();
    void renderLines();
    void renderRects();
    void renderCircles();
    void renderTexts();
};

struct DebugLine {
    glm::vec2 start;
    glm::vec2 end;
    glm::vec4 color;
    float timeRemaining;
};

struct DebugRect {
    glm::vec2 position;
    glm::vec2 size;
    glm::vec4 color;
    float timeRemaining;
};

struct DebugCircle {
    glm::vec2 center;
    float radius;
    glm::vec4 color;
    int segments;
    float timeRemaining;
};

struct DebugText {
    glm::vec2 position;
    std::string text;
    glm::vec4 color;
    float scale;
    float timeRemaining;
};

void DebugRenderer::drawActiveChunks(const ChunkManager* chunkManager, const glm::vec4& color) {
    if (!chunkManager) return;
    
    const auto& activeChunks = chunkManager->getActiveChunks();
    
    for (const auto& chunkCoord : activeChunks) {
        glm::vec2 chunkPos(chunkCoord.x * CHUNK_SIZE, chunkCoord.y * CHUNK_SIZE);
        glm::vec2 chunkSize(CHUNK_SIZE, CHUNK_SIZE);
        
        drawRect(chunkPos, chunkSize, color);
    }
}

void DebugRenderer::drawChunkBoundaries(const ChunkManager* chunkManager, const glm::vec4& color) {
    if (!chunkManager) return;
    
    // Get visible area from camera
    WorldRect visibleArea = camera->getVisibleWorldRect();
    
    // Calculate chunk range
    int startChunkX = visibleArea.x / CHUNK_SIZE;
    int startChunkY = visibleArea.y / CHUNK_SIZE;
    int endChunkX = (visibleArea.x + visibleArea.width) / CHUNK_SIZE + 1;
    int endChunkY = (visibleArea.y + visibleArea.height) / CHUNK_SIZE + 1;
    
    // Draw horizontal lines
    for (int y = startChunkY; y <= endChunkY; y++) {
        float worldY = y * CHUNK_SIZE;
        drawLine(glm::vec2(startChunkX * CHUNK_SIZE, worldY), 
                glm::vec2(endChunkX * CHUNK_SIZE, worldY), 
                color);
    }
    
    // Draw vertical lines
    for (int x = startChunkX; x <= endChunkX; x++) {
        float worldX = x * CHUNK_SIZE;
        drawLine(glm::vec2(worldX, startChunkY * CHUNK_SIZE), 
                glm::vec2(worldX, endChunkY * CHUNK_SIZE), 
                color);
    }
}

void DebugRenderer::drawVelocityField(const ChunkManager* chunkManager, const MaterialRegistry* materials, float scale) {
    if (!chunkManager || !materials) return;
    
    // Get visible area from camera
    WorldRect visibleArea = camera->getVisibleWorldRect();
    
    // Sample interval (don't draw for every cell)
    int sampleInterval = 4;
    
    // For each visible cell (with interval)
    for (int y = visibleArea.y; y < visibleArea.y + visibleArea.height; y += sampleInterval) {
        for (int x = visibleArea.x; x < visibleArea.x + visibleArea.width; x += sampleInterval) {
            if (!chunkManager->isValidCoord({x, y})) continue;
            
            const Cell& cell = chunkManager->getCell(x, y);
            
            if (cell.material == materials->air()) continue;
            
            const MaterialProperties& props = materials->getMaterial(cell.material);
            
            // Only show for dynamic materials
            if (!props.movable) continue;
            
            // Get velocity (this would come from your physics system)
            // This is just an example - actual velocity would come from your simulation
            glm::vec2 velocity = cell.velocity * scale;
            
            // Skip very small velocities
            if (glm::length(velocity) < 0.1f) continue;
            
            // Draw a line representing the velocity
            glm::vec2 start(x + 0.5f, y + 0.5f);
            glm::vec2 end = start + velocity;
            
            // Color based on material and velocity magnitude
            float speed = glm::length(velocity);
            glm::vec4 color = glm::vec4(props.color.r, props.color.g, props.color.b, 0.8f);
            
            drawLine(start, end, color);
        }
    }
}
```

## ImGui Integration for Debug UI

```cpp
class ImGuiRenderer {
private:
    GLFWwindow* window;
    bool initialized;
    
public:
    ImGuiRenderer(GLFWwindow* window);
    ~ImGuiRenderer();
    
    bool initialize();
    void shutdown();
    
    void beginFrame();
    void endFrame();
    
    // Debug UI components
    void renderPerformanceWindow(const PerformanceMetrics& metrics);
    void renderMaterialDebugWindow(const MaterialRegistry* materials);
    void renderPhysicsDebugWindow(const PhysicsSystem* physics);
    void renderRenderingDebugWindow(RenderingSystem* renderer);
};

ImGuiRenderer::ImGuiRenderer(GLFWwindow* window) : window(window), initialized(false) {
}

ImGuiRenderer::~ImGuiRenderer() {
    shutdown();
}

bool ImGuiRenderer::initialize() {
    // Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    
    // Configure ImGui
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    
    // Set up style
    ImGui::StyleColorsDark();
    
    // Set up platform/renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");
    
    initialized = true;
    return true;
}

void ImGuiRenderer::shutdown() {
    if (initialized) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        initialized = false;
    }
}

void ImGuiRenderer::beginFrame() {
    if (!initialized) return;
    
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void ImGuiRenderer::endFrame() {
    if (!initialized) return;
    
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void ImGuiRenderer::renderPerformanceWindow(const PerformanceMetrics& metrics) {
    if (!initialized) return;
    
    ImGui::Begin("Performance");
    
    ImGui::Text("FPS: %.1f", metrics.fps);
    ImGui::Text("Frame Time: %.3f ms", metrics.frameTime * 1000.0f);
    ImGui::Text("Physics Time: %.3f ms", metrics.physicsTime * 1000.0f);
    ImGui::Text("Render Time: %.3f ms", metrics.renderTime * 1000.0f);
    
    ImGui::Separator();
    
    ImGui::Text("Cells");
    ImGui::Text("  Rendered: %d", metrics.renderedCells);
    ImGui::Text("  Updated: %d", metrics.updatedCells);
    ImGui::Text("  Active Chunks: %d", metrics.activeChunks);
    
    ImGui::Separator();
    
    ImGui::Text("Memory");
    ImGui::Text("  Used: %.2f MB", metrics.memoryUsage / (1024.0f * 1024.0f));
    
    ImGui::End();
}
```

## Rendering System Optimization

The renderer employs several optimization techniques:

### Texture Atlasing

All materials are packed into a single texture atlas to minimize texture switches.

### Instanced Rendering

The grid renderer uses instanced rendering to draw thousands of cells efficiently.

### Frustum Culling

Only cells within the camera's view are rendered.

### Deferred Lighting

The lighting system uses a deferred approach, calculating lighting in a separate pass.

### Chunk-Based Rendering

Chunks outside the camera view are not processed or rendered.

### Visibility Optimization

Empty (air) cells are skipped during rendering.

### Render Batching

Similar materials are batched together to minimize state changes.

### GPU Memory Management

Vertex data is streamed to the GPU efficiently using dynamic buffers.

## Conclusion

The rendering system for Astral provides high-performance visualization of the cellular simulation. Key features include:

1. **Efficient Grid Rendering**: Using instanced rendering and frustum culling
2. **Dynamic Lighting**: Emissive materials create light sources
3. **Post-Processing Effects**: Bloom, chromatic aberration, vignette, and film grain
4. **Particle System**: For additional visual effects
5. **Debug Visualization**: Tools for debugging physics and rendering
6. **Camera System**: Smooth camera movement and effects

The system is designed to be modular and extensible, allowing for new visual effects and rendering techniques to be added as needed.