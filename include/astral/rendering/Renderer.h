#pragma once

#include <string>
#include <memory>
#include <glm/glm.hpp>

namespace astral {

class Camera;
class ShaderProgram;
class GridRenderer;
class ChunkManager;
class MaterialRegistry;

class Renderer {
public:
    Renderer();
    virtual ~Renderer();
    
    bool initialize(int width, int height, const std::string& title);
    void shutdown();
    
    void beginFrame();
    void endFrame();
    
    void setViewport(int x, int y, int width, int height);
    void setClearColor(float r, float g, float b, float a);
    
    void renderWorld(const ChunkManager* chunkManager, const MaterialRegistry* materials);
    
    bool shouldClose() const;
    
    int getWidth() const { return width; }
    int getHeight() const { return height; }
    
private:
    int width;
    int height;
    std::string title;
    
    std::unique_ptr<Camera> camera;
    std::unique_ptr<ShaderProgram> gridShader;
    std::unique_ptr<GridRenderer> gridRenderer;
    
    void* window; // GLFW window, void* to avoid including GLFW here
};

} // namespace astral