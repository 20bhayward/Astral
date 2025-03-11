# Astral Engine Architecture

## Core Architecture Overview

Astral is a high-performance 2D physics-based voxel simulation engine inspired by games like Noita, with a focus on realistic particle physics, fluid dynamics, and material interactions. This document outlines the detailed architecture of the engine.

## System Components

The Astral engine is structured into several interconnected systems that work together to create a cohesive simulation:

```
+---------------------+     +----------------------+     +----------------------+
| Rendering System    |     | Physics System       |     | World Generation     |
|---------------------|     |----------------------|     |----------------------|
| - OpenGL Renderer   |<--->| - Material System    |<--->| - Terrain Generator  |
| - Particle Renderer |     | - Fluid Dynamics     |     | - Biome System       |
| - Shader Management |     | - Cellular Automata  |     | - Cave Generator     |
| - Camera System     |     | - Rigid Body Physics |     | - Structure Placer   |
+---------------------+     +----------------------+     +----------------------+
           ^                          ^                           ^
           |                          |                           |
           v                          v                           v
+---------------------+     +----------------------+     +----------------------+
| Core Engine         |     | Optimization Systems |     | Utility Systems      |
|---------------------|     |----------------------|     |----------------------|
| - Game Loop         |<--->| - Chunk Manager      |<--->| - Resource Manager   |
| - Event System      |     | - Thread Pool        |     | - Input Handler      |
| - Entity Component  |     | - Spatial Partioning |     | - Debug Tools        |
| - Memory Management |     | - GPU Acceleration   |     | - Serialization      |
+---------------------+     +----------------------+     +----------------------+
```

## Core Engine Design

### Engine Core

The engine core manages the central game loop, memory allocation, and coordination between systems. It follows a data-oriented design approach to maximize performance.

```cpp
class AstralEngine {
private:
    // Subsystems
    std::unique_ptr<RenderingSystem> renderer;
    std::unique_ptr<PhysicsSystem> physics;
    std::unique_ptr<WorldGenerator> worldGen;
    std::unique_ptr<ChunkManager> chunkManager;
    std::unique_ptr<ThreadPool> threadPool;
    std::unique_ptr<ResourceManager> resourceManager;
    std::unique_ptr<EventSystem> eventSystem;
    
    // Engine state
    bool isRunning;
    double deltaTime;
    uint64_t frameCount;
    
    // Performance metrics
    PerformanceMetrics metrics;
    
public:
    AstralEngine();
    ~AstralEngine();
    
    // Initialization
    bool initialize(const EngineConfig& config);
    void shutdown();
    
    // Main loop control
    void run();
    void pause();
    void resume();
    void stop();
    
    // Frame timing
    double getDeltaTime() const { return deltaTime; }
    uint64_t getFrameCount() const { return frameCount; }
    
    // System access
    RenderingSystem* getRenderer() { return renderer.get(); }
    PhysicsSystem* getPhysics() { return physics.get(); }
    WorldGenerator* getWorldGenerator() { return worldGen.get(); }
    ChunkManager* getChunkManager() { return chunkManager.get(); }
    ThreadPool* getThreadPool() { return threadPool.get(); }
    ResourceManager* getResourceManager() { return resourceManager.get(); }
    EventSystem* getEventSystem() { return eventSystem.get(); }
    
    // Event handling
    void processEvents();
    
    // Update and render
    void update();
    void render();
    
    // Utility
    void captureScreenshot(const std::string& filename);
    PerformanceMetrics getPerformanceMetrics() const { return metrics; }
};
```

### Game Loop

The engine uses a fixed timestep game loop for physics simulation with variable rendering:

```cpp
void AstralEngine::run() {
    const double fixedTimeStep = 1.0 / 60.0; // 60 updates per second
    double accumulator = 0.0;
    double currentTime = Timer::getTimeInSeconds();
    
    isRunning = true;
    
    while (isRunning) {
        double newTime = Timer::getTimeInSeconds();
        double frameTime = newTime - currentTime;
        currentTime = newTime;
        
        // Cap max frame time to avoid spiral of death
        frameTime = std::min(frameTime, 0.25);
        
        // Track performance
        metrics.frameTime = frameTime;
        metrics.fps = 1.0 / frameTime;
        
        // Process input
        processEvents();
        
        // Fixed update for physics
        accumulator += frameTime;
        while (accumulator >= fixedTimeStep) {
            metrics.startPhysicsTime();
            physics->update(fixedTimeStep);
            metrics.endPhysicsTime();
            
            accumulator -= fixedTimeStep;
        }
        
        // Variable update for everything else
        metrics.startUpdateTime();
        update();
        metrics.endUpdateTime();
        
        // Render with interpolation
        metrics.startRenderTime();
        renderer->setInterpolation(accumulator / fixedTimeStep);
        render();
        metrics.endRenderTime();
        
        frameCount++;
        
        // Yield to OS if we're running too fast
        if (frameTime < 0.001) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
}
```

### Event System

The event system uses a publisher-subscriber pattern to decouple systems:

```cpp
class EventSystem {
private:
    // Map from event type to list of subscribers
    std::unordered_map<EventType, std::vector<EventCallback>> subscribers;
    
    // Thread-safety
    std::mutex eventQueueMutex;
    std::queue<Event> eventQueue;
    
public:
    void subscribe(EventType type, EventCallback callback);
    void unsubscribe(EventType type, void* subscriber);
    
    // Queue an event for later processing
    void queueEvent(const Event& event);
    
    // Immediately dispatch an event
    void dispatchEvent(const Event& event);
    
    // Process all queued events
    void processEventQueue();
};
```

### Entity Component System

The engine uses an Entity Component System (ECS) architecture for game objects:

```cpp
// Entity is just an ID
using EntityID = uint32_t;

// Component interface
class Component {
public:
    virtual ~Component() = default;
    virtual void initialize() {}
    virtual ComponentType getType() const = 0;
};

// System that operates on components
class System {
public:
    virtual ~System() = default;
    virtual void update(double deltaTime) = 0;
};

// Entity manager
class EntityManager {
private:
    EntityID nextEntityID = 0;
    std::unordered_map<EntityID, std::unordered_map<ComponentType, std::shared_ptr<Component>>> entities;
    std::vector<std::unique_ptr<System>> systems;
    
public:
    EntityID createEntity();
    void destroyEntity(EntityID entity);
    
    template<typename T, typename... Args>
    T* addComponent(EntityID entity, Args&&... args);
    
    template<typename T>
    T* getComponent(EntityID entity);
    
    template<typename T>
    void removeComponent(EntityID entity);
    
    template<typename T, typename... Args>
    void addSystem(Args&&... args);
    
    void updateSystems(double deltaTime);
};
```

## World Representation

The world is represented as a grid of cells, organized into chunks for efficient memory management and parallelization:

```cpp
// A single cell in the world
struct Cell {
    MaterialID material;
    uint8_t health;
    uint8_t temperature;
    uint8_t metadata;
    // Other properties...
};

// A chunk of the world
class Chunk {
private:
    static constexpr int CHUNK_SIZE = 64; // 64x64 cells per chunk
    ChunkCoord coord;
    Cell cells[CHUNK_SIZE][CHUNK_SIZE];
    bool isDirty;
    bool isActive;
    
public:
    Chunk(ChunkCoord coord);
    
    Cell& getCell(int x, int y);
    const Cell& getCell(int x, int y) const;
    void setCell(int x, int y, const Cell& cell);
    
    bool isDirty() const { return isDirty; }
    void markDirty() { isDirty = true; }
    void clearDirty() { isDirty = false; }
    
    bool isActive() const { return isActive; }
    void setActive(bool active) { isActive = active; }
    
    // Update physics in this chunk
    void update(double deltaTime);
    
    // Render this chunk
    void render(Renderer* renderer);
};

// Manages chunks in the world
class ChunkManager {
private:
    std::unordered_map<ChunkCoord, std::unique_ptr<Chunk>, ChunkCoordHash> chunks;
    std::set<ChunkCoord> activeChunks;
    ThreadPool* threadPool;
    
public:
    ChunkManager(ThreadPool* threadPool);
    
    Chunk* getChunk(ChunkCoord coord);
    Chunk* getOrCreateChunk(ChunkCoord coord);
    void removeChunk(ChunkCoord coord);
    
    Cell* getCell(WorldCoord coord);
    void setCell(WorldCoord coord, const Cell& cell);
    
    // Convert between world and chunk coordinates
    static ChunkCoord worldToChunkCoord(WorldCoord coord);
    static LocalCoord worldToLocalCoord(WorldCoord coord);
    static WorldCoord chunkToWorldCoord(ChunkCoord chunkCoord, LocalCoord localCoord);
    
    // Update active chunks
    void updateActiveChunks(const WorldRect& activeArea);
    void updateChunks(double deltaTime);
    
    // Save/load chunks
    void saveChunk(ChunkCoord coord, const std::string& filename);
    void loadChunk(ChunkCoord coord, const std::string& filename);
};
```

## Memory Management

The engine employs custom memory management to reduce fragmentation and improve cache coherency:

```cpp
// Object pool for frequently allocated objects
template<typename T>
class ObjectPool {
private:
    static constexpr size_t BLOCK_SIZE = 1024;
    
    struct Block {
        std::array<T, BLOCK_SIZE> objects;
        std::bitset<BLOCK_SIZE> used;
        size_t firstFree;
    };
    
    std::vector<std::unique_ptr<Block>> blocks;
    
public:
    T* allocate();
    void deallocate(T* object);
    size_t getCapacity() const;
    size_t getUsed() const;
};

// Custom allocator for systems with specific memory requirements
template<size_t BlockSize>
class BlockAllocator {
private:
    struct MemoryBlock {
        uint8_t data[BlockSize];
        MemoryBlock* next;
    };
    
    MemoryBlock* freeList = nullptr;
    std::vector<std::unique_ptr<MemoryBlock>> blocks;
    
public:
    void* allocate(size_t size);
    void deallocate(void* ptr);
    
    template<typename T, typename... Args>
    T* create(Args&&... args);
    
    template<typename T>
    void destroy(T* object);
};
```

## Threading Model

The engine uses a job-based threading model to parallelize work:

```cpp
class ThreadPool {
private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    
    std::mutex queueMutex;
    std::condition_variable condition;
    bool stop;
    
public:
    ThreadPool(size_t numThreads);
    ~ThreadPool();
    
    // Add task to the pool
    template<typename F, typename... Args>
    auto enqueue(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type>;
    
    // Get number of active threads
    size_t getNumThreads() const { return workers.size(); }
    
    // Wait for all tasks to complete
    void waitForAll();
};
```

## Configuration System

A robust configuration system allows tweaking engine parameters:

```cpp
class ConfigManager {
private:
    std::unordered_map<std::string, std::variant<int, float, double, bool, std::string>> configValues;
    std::string configFilePath;
    
public:
    ConfigManager();
    ~ConfigManager();
    
    bool loadFromFile(const std::string& filepath);
    bool saveToFile(const std::string& filepath = "");
    
    // Getters
    template<typename T>
    T get(const std::string& key, const T& defaultValue = T()) const;
    
    // Setters
    template<typename T>
    void set(const std::string& key, const T& value);
    
    // Check if key exists
    bool hasKey(const std::string& key) const;
    
    // Remove a key
    void removeKey(const std::string& key);
    
    // Clear all
    void clear();
};
```

## Serialization System

For saving and loading world state:

```cpp
// Interface for serializable objects
class ISerializable {
public:
    virtual ~ISerializable() = default;
    virtual void serialize(Serializer& serializer) const = 0;
    virtual void deserialize(Deserializer& deserializer) = 0;
};

// Binary serializer
class BinarySerializer {
private:
    std::vector<uint8_t> buffer;
    
public:
    // Primitive types
    void write(bool value);
    void write(int8_t value);
    void write(uint8_t value);
    void write(int16_t value);
    void write(uint16_t value);
    void write(int32_t value);
    void write(uint32_t value);
    void write(int64_t value);
    void write(uint64_t value);
    void write(float value);
    void write(double value);
    void write(const std::string& value);
    
    // Arrays
    template<typename T, size_t N>
    void write(const std::array<T, N>& array);
    
    template<typename T>
    void write(const std::vector<T>& vec);
    
    // Custom objects
    void write(const ISerializable& object);
    
    // Get serialized data
    const std::vector<uint8_t>& getData() const { return buffer; }
    
    // Save to file
    bool saveToFile(const std::string& filepath) const;
};

// Binary deserializer
class BinaryDeserializer {
private:
    const uint8_t* buffer;
    size_t bufferSize;
    size_t position;
    
public:
    BinaryDeserializer(const uint8_t* data, size_t size);
    
    // Primitive types
    bool readBool();
    int8_t readInt8();
    uint8_t readUInt8();
    int16_t readInt16();
    uint16_t readUInt16();
    int32_t readInt32();
    uint32_t readUInt32();
    int64_t readInt64();
    uint64_t readUInt64();
    float readFloat();
    double readDouble();
    std::string readString();
    
    // Arrays
    template<typename T, size_t N>
    void read(std::array<T, N>& array);
    
    template<typename T>
    void read(std::vector<T>& vec);
    
    // Custom objects
    void read(ISerializable& object);
    
    // Check if we're at the end
    bool isEnd() const { return position >= bufferSize; }
    
    // Load from file
    static std::unique_ptr<BinaryDeserializer> loadFromFile(const std::string& filepath);
};
```

## Resource Management

The resource management system handles loading and caching assets:

```cpp
// Resource handle
template<typename T>
class ResourceHandle {
private:
    std::shared_ptr<T> resource;
    std::string id;
    
public:
    ResourceHandle(const std::string& id, std::shared_ptr<T> resource);
    
    T* get() { return resource.get(); }
    const T* get() const { return resource.get(); }
    
    const std::string& getID() const { return id; }
    
    operator bool() const { return resource != nullptr; }
    T* operator->() { return resource.get(); }
    const T* operator->() const { return resource.get(); }
};

// Resource cache
template<typename T>
class ResourceCache {
private:
    std::unordered_map<std::string, std::weak_ptr<T>> resources;
    std::mutex cacheMutex;
    
public:
    ResourceHandle<T> getResource(const std::string& id);
    void addResource(const std::string& id, std::shared_ptr<T> resource);
    void removeResource(const std::string& id);
    void clearUnused();
    void clear();
};

// Resource manager
class ResourceManager {
private:
    ResourceCache<Texture> textures;
    ResourceCache<Shader> shaders;
    ResourceCache<Sound> sounds;
    ResourceCache<MaterialDefinition> materials;
    
    std::string resourceBasePath;
    
public:
    ResourceManager(const std::string& basePath = "resources");
    
    // Texture loading
    ResourceHandle<Texture> loadTexture(const std::string& id, const std::string& filepath);
    ResourceHandle<Texture> getTexture(const std::string& id);
    
    // Shader loading
    ResourceHandle<Shader> loadShader(const std::string& id, const std::string& vertexPath, const std::string& fragmentPath);
    ResourceHandle<Shader> getShader(const std::string& id);
    
    // Sound loading
    ResourceHandle<Sound> loadSound(const std::string& id, const std::string& filepath);
    ResourceHandle<Sound> getSound(const std::string& id);
    
    // Material loading
    ResourceHandle<MaterialDefinition> loadMaterial(const std::string& id, const std::string& filepath);
    ResourceHandle<MaterialDefinition> getMaterial(const std::string& id);
    
    // Clean up unused resources
    void clearUnused();
    
    // Get base path
    const std::string& getBasePath() const { return resourceBasePath; }
};
```

## Integration with External Libraries

The engine is designed to integrate with various external libraries:

- **OpenGL**: For rendering
- **GLFW**: For window management and input
- **GLM**: For mathematics
- **ImGui**: For debugging UI
- **stb_image**: For image loading
- **EnTT**: For entity component system
- **spdlog**: For logging
- **nlohmann/json**: For configuration and serialization
- **OpenAL**: For audio
- **Box2D**: For rigid body physics

```cpp
// Example of OpenGL integration
class GLRenderer : public RenderingSystem {
private:
    GLFWwindow* window;
    ShaderManager shaderManager;
    TextureManager textureManager;
    Camera camera;
    
    // Rendering state
    GLuint particleVAO;
    GLuint particleVBO;
    
public:
    GLRenderer();
    ~GLRenderer();
    
    bool initialize(int width, int height, const std::string& title) override;
    void shutdown() override;
    
    void beginFrame() override;
    void endFrame() override;
    
    void renderParticles(const ParticleCollection& particles) override;
    void renderUI() override;
    
    void setViewport(int x, int y, int width, int height) override;
    void setClearColor(float r, float g, float b, float a) override;
    
    bool isWindowClosed() const override;
    void pollEvents() override;
};
```

## Debug Systems

The engine includes extensive debug functionality:

```cpp
class DebugSystem {
private:
    bool enabled;
    std::unordered_map<std::string, bool> debugFlags;
    std::vector<DebugDrawCommand> drawCommands;
    
public:
    DebugSystem();
    
    void setEnabled(bool enabled);
    bool isEnabled() const { return enabled; }
    
    void setFlag(const std::string& flag, bool value);
    bool getFlag(const std::string& flag) const;
    
    void drawLine(const glm::vec2& start, const glm::vec2& end, const glm::vec4& color);
    void drawRect(const glm::vec2& pos, const glm::vec2& size, const glm::vec4& color);
    void drawCircle(const glm::vec2& center, float radius, const glm::vec4& color);
    void drawText(const glm::vec2& pos, const std::string& text, const glm::vec4& color);
    
    void clearDrawCommands();
    const std::vector<DebugDrawCommand>& getDrawCommands() const { return drawCommands; }
    
    void renderImGui();
    void renderDrawCommands(Renderer* renderer);
};
```

## Conclusion

The Astral engine architecture is designed to be:

1. **Modular**: Systems are decoupled and communicate through well-defined interfaces
2. **High-performance**: Using data-oriented design, custom memory management, and multithreading
3. **Flexible**: Configurable through external files and adaptive at runtime
4. **Extendable**: New systems and components can be added easily

The design draws inspiration from modern game engines while focusing specifically on the requirements of a 2D falling-sand physics simulation. By following this architecture, we create a foundation that can scale to large worlds with complex physics interactions while maintaining high performance.